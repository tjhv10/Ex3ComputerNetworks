#include <stdio.h> 
#include <stdlib.h> 
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h> 
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>



#define SERVER_PORT 9998  // The port to connect server 
#define SERVER_IP_ADDRESS "127.0.0.1" // Te server IP adderss
#define BUFFER_SIZE 4096 // Buffer size for sending the text 
#define filename "text.txt"

int authentication(int sockfd); // function for performing autentication

int main(){

  

    FILE *fp; 
    
    // create a socket for communicating with receiver using TCP protocol 

    int sock;  // socket descriptor
    if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
        printf("Could not create socket : %d", errno);
        return -1; 
    }

    struct sockaddr_in server_address; // struct of the socket
    memset(&server_address, 0, sizeof(server_address)); 

    server_address.sin_family = AF_INET; // Ipv4 
    server_address.sin_port = htons(SERVER_PORT);  

    int rval = inet_pton(AF_INET, (const char *)SERVER_IP_ADDRESS, &server_address.sin_addr);

    if(rval<=0){
        printf("inet_pton() faild"); 
        return -1; 
    }

    
    // Make a connnection to the Receiver server 
    int connectResult = connect(sock, (struct sockaddr*)&server_address, sizeof(server_address));
    if(connectResult == -1){  // connection faild - print a message and close socket
        printf("connect() failed with error code %d\n", errno);  
        close(sock); 
        return -1; 
    }

    // If we are here - connection with receiver has established!!! 

    printf("Connected to server!!!\n");



    /*************** while loop for sending the file and performing authentication start sending the file  *********************/

    int sendfile=1; // While send file is diffrent than 0 - ccontinue; 


    while(sendfile){
        
        if(setsockopt(sock,IPPROTO_TCP,TCP_CONGESTION,"cubic",5)<0){  // send the file with "cubic" cc algorithm 
            return -1; 
        }                       
    

        fp = fopen(filename, "r"); // open the file for reading
        if(fp == NULL){
            perror("ERROR in reading file\n");
            exit(1);
        }

        // calculate the length of the file 
        int textLength;
        fseek(fp, 0L, SEEK_END); 
        textLength = ftell(fp);
        fseek(fp, 0L, SEEK_SET);


        int n; // how many bytes are sent on send()
        int size; // how many bytes are read on fread()
        char data[BUFFER_SIZE] = {0}; // buffer for reading the data from the file
        int totalByteSent=0; // total amount of the bytes that were sent

       
       
        // byteToSend contains the number of bytes that is a multiplication of the buffer size and is more than half of the size of the file. 
        int byteToSend = textLength/2 + (BUFFER_SIZE - (textLength/2)%BUFFER_SIZE); 

        
        if((n = send(sock, &byteToSend, sizeof(byteToSend), 0)==-1)){
            perror("Error in sending size of file");
            return -1; 
        }  // inform the receiver how many bytes we are sending

        //*****while looop for reading the data from file and sending it to receiver with sent(). ******// 
      
        while ((size = fread(data, 1,BUFFER_SIZE, fp))){ // read the data 
            if((n = send(sock, data, size,0))==-1){
                perror("Error in sending file"); 
                close(sock); 
                return -1; 
            } else if(n == 0){
                printf("peer has closed the TCP connection prior to send(). \n");  
                close(sock);
                return -1;   
            }

            totalByteSent += n; // sum the total byte that are sent.
            if(totalByteSent >= byteToSend){ // sended the first half. break and continue for authentication 
                printf("Sent %d bytes\n", totalByteSent);
                break; 
            }
        }
                
     
        //  Authentication - get a authentication message from receiver. 

        authentication(sock); // perfrom authentication 
        




        // Change the CC algorithm to "reno"

        if(setsockopt(sock,IPPROTO_TCP,TCP_CONGESTION,"reno",sizeof("reno"))<0){
            return -1; 
        }

       
       
        //******* send the second part of the file ********// 
        //**same procces like frist half ** // 
        
        byteToSend = textLength - byteToSend; // The number of bytes that are left to send

        totalByteSent = 0, n=0;
        

        send(sock, &byteToSend, sizeof(byteToSend), 0); // send the num of bytes that are left to sent
        

        // ** loop for reading the file and sending it **// 
       
        while ((size = fread(data, 1,BUFFER_SIZE, fp))){

            if((n = send(sock, data, size,0))==-1){
               perror("Error in sending file"); 
               return -1;
            }
            totalByteSent += n;
             
            if(totalByteSent >= byteToSend){
                printf("Sent %d bytes\n", totalByteSent);
                break; 
            }     
        }

        


        //****** User dicision if to send the file again or to exit. ******//  

        printf("Enter '0' for exiting, or enter anything else for sending againg the file\n"); 
        scanf("%d", &sendfile); // The while condition is "sendfile == 1"

        fclose(fp); // close the file. 

    }

    // send 0 to notify the receiver that we finished sending the file
    int sizeOfFile = 0; 
    int totalByteSent = send(sock, &sizeOfFile, sizeof(sizeOfFile), 0);  

    if(totalByteSent == -1){
        printf("send() faild %d", errno); 
    } else if(totalByteSent == 0){
        printf("peer has closed the TCP connection prior to send(). \n"); 
    } else if(totalByteSent < (int)sizeof(sizeof(sizeOfFile))){
        printf("sent only %d bytes from the requierd %ld\n", totalByteSent, sizeof(totalByteSent)); 
    } else{
        printf(" 0 was succussfuly sent\n"); 
    }   

	close(sock); // finshed all. close socket. 

	return 0; 
}


 // the fiction is used to verify the authentication  
 // parm sockfd the socket of the sender

    int authentication(int sockfd){
        int authenticationReply; 
        int byteRecieved  = recv(sockfd, &authenticationReply, 10, 0); // recv the authentication 

        if(byteRecieved == -1){
            printf("recv() failed withe error code : %d" , errno); 
            return -1; 
        } 
        
        printf("recieved %d bytes from server :\n", byteRecieved); 
        
        
        if(authenticationReply != (197^4599)){ 
            printf("authentication faild\n");
            close(sockfd);
            return -1; 
        }
   
        printf("Authentication succeeded\n");

        return 1; 
    }