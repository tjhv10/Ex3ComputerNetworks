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

#include <time.h>

#define SERVER_PORT 9998  // port for making connection
#define SERVER_IP_ADDRESS "127.0.0.1" // local IP 
#define BUFFER_SIZE 4096 
#define TEXT_LENGTH 8640185

uint64_t timeOfReceivingFiles[1000];  // buffer of longs that every i index holds how much time took the i sending part. 
int indexOfReveivingFiles = 0;  // index to keep track which time we are taking now


int main(){

    // open a socket for the receiver
    int listeningSocket = -1; 
    listeningSocket = socket(AF_INET, SOCK_STREAM, 0); // The listining socket descriptor
    if(listeningSocket == -1){
        printf("Could not create socket : %d", errno);
        return -1; 
    }

    int enableReuse = 1; 
    int ret = setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)); // enable reuse of the socket on its port
    if(ret<0){
        printf("setsocketpot() failed with error code : %d" , errno); 
        return -1; 
    }

    struct sockaddr_in server_address; // struct of the socket
    memset(&server_address, 0, sizeof(server_address)); 

    server_address.sin_family = AF_INET;  // use IPv4 address
    server_address.sin_port = htons(SERVER_PORT); // 
    server_address.sin_addr.s_addr = INADDR_ANY; // any IP at this port 
    
    
    int bindResult = bind(listeningSocket, (struct sockaddr *)&server_address, sizeof(server_address));  // iund the port with address and port
    if(bindResult == -1){
        printf("bind() failed with error code : %d\n" , errno); 
        //close socket
        close(listeningSocket); 
        return -1; 
    }

    

    int listenResult = listen(listeningSocket, 4);  // start listining for incoming sockets 
    if(listenResult == -1){
        printf("listen() failed with error code : %d", errno);
        //close the socket
        close(listeningSocket); 
    }

    printf("Waiting for incoming TCP-connection...\n"); 

    struct sockaddr_in clientAddress; // A new socket for the tcp connection with client 
    socklen_t clientAddressLen = sizeof(clientAddress); 

    memset(&clientAddress, 0, sizeof(clientAddress)); 
    clientAddressLen = sizeof(clientAddress); 

    int clientSocket = accept(listeningSocket, (struct sockaddr *)&clientAddress, &clientAddressLen); // accept a new connection from the sender
    if(clientSocket == -1){
        printf("listen() failed with error code : %d" , errno);
        close(listeningSocket); 
        return -1;
    }

    printf("A new client connection accepted\n"); 
    
    // *** main while loop for receiving to parts of the file and perform authentication ***//
    while(1){
        

       
        char buffer[BUFFER_SIZE];  // buffer for receiving message
        int totalByteReceive = 0; // Total sum of received bytes, so we know wen to stop to recv() 
        int byteRecieved; // byte received in each recv() call. 
        int sizeOfFile; // The size of coming message from sender 







        //************receive the first file part from client*****************//
        

        
        recv(clientSocket,&sizeOfFile, sizeof(sizeOfFile), 0); // recv the size of the first part of file 

        if(sizeOfFile == 0){ // The sender has finised sending the file. break the while loop. 
            break;
        }

        printf("expecting size of file to be: %d\n", sizeOfFile); 
        
        if(setsockopt(clientSocket,IPPROTO_TCP,TCP_CONGESTION,"cubic",strlen("cubic"))<0){
            return -1; 
        }
        struct timespec before, after;
        clock_gettime(CLOCK_MONOTONIC, &before);
        uint64_t before_ns = (before.tv_sec * 1000000000) + before.tv_nsec;   

        //******recv first part while loop******//
        while(1){  // loop to get the message in chuncks of BUFFER_SIZE bytes(4096); 

        
            memset(buffer, 0, BUFFER_SIZE); // clear the buffer
                
             // struct for saving the currnt time; 
             // save current time before recv the file 
            if((byteRecieved = recv(clientSocket, buffer, BUFFER_SIZE, 0)) < 0){   // recv the chunks of message
                printf("recv failed with error code : %d", errno);
                close(listeningSocket); 
                close(clientSocket);
                return -1;
            }

            totalByteReceive += byteRecieved; // sum up the number of bytes that already received until recv all message
            
            if(totalByteReceive >= sizeOfFile){ // means that all the first part of the message has arrived
                clock_gettime(CLOCK_MONOTONIC, &after); // save she current time after recv the file
                uint64_t after_ns = (after.tv_sec * 1000000000) + after.tv_nsec;   
                timeOfReceivingFiles[indexOfReveivingFiles++] = after_ns - before_ns; // calculate the time it took to recv the file
                printf("Received succussfully part 1\n");
                printf("%d total\n ", totalByteReceive);
  
                break; // got all the message from the sender. break the recv while loop 
            }
        }
            
           
        // Autentication with the sender. send the xor of 4599 and 0197 to sender. 

        int authenticationMessage = 4599 ^ 197;

        int byteSent = send(clientSocket, &authenticationMessage, sizeof(authenticationMessage), 0);
        if(byteSent == -1){
            printf("send() failed with error code : %d ", errno);
            close(listeningSocket);
            close(clientSocket);
            return -1; 
        } else if(byteSent == 0){
            printf("peer has closed the TCP connection prior to send(). \n"); 
        } else if(byteSent < sizeof(authenticationMessage)){
            printf("sent only %d bytes from the requierd %ld", byteSent, sizeof(authenticationMessage)); 
        } else{
            printf("authentication message was succussfuly sent\n"); 
        }
       

        if(setsockopt(clientSocket,IPPROTO_TCP,TCP_CONGESTION,"reno",4) < 0){ // change the cc algorithm for receving the second part of file
            printf("setsocketpot() failed with error code : %d" , errno); 
            return -1; 
        }




        //**************** receive the second part from the client *****************// 

        totalByteReceive = 0;
        recv(clientSocket,&sizeOfFile, sizeof(sizeOfFile), 0);  // receive the length of the second part of the file
        printf("size of file to get is %d\n", sizeOfFile); 

        clock_gettime(CLOCK_MONOTONIC, &before);
        before_ns = (before.tv_sec * 1000000000) + before.tv_nsec;
        //********** recv second part while loop********// 
        // ** The same process like recv first part ** // 
        //struct timespec before, after; 
        while(1){
            memset(buffer, 0, BUFFER_SIZE); 

            
             
            
            
            if((byteRecieved = recv(clientSocket, buffer, BUFFER_SIZE, 0)) < 0){ 
                printf("recv failed with error code : %d", errno);
                close(listeningSocket); 
                close(clientSocket);
                return -1;
            }
        
            totalByteReceive += byteRecieved; 
            
            if(totalByteReceive >= sizeOfFile){
                clock_gettime(CLOCK_MONOTONIC, &after);
                uint64_t after_ns = (after.tv_sec * 1000000000) + after.tv_nsec;
                timeOfReceivingFiles[indexOfReveivingFiles++] = after_ns - before_ns; 
                printf("%d total\n ", totalByteReceive);
                printf("Received succussfully part 2\n");
                break;
            }
            
        }
         
       

    } // End of the main while loop 



    long sumOfTimes=0;
    //long avarageOftimes;
    // loop the timeOfreceivingFiles buffer, and print and sum up all the indexes (print the time of every part, and sum up all the times)
    for(int i=0;i<indexOfReveivingFiles;i++){
        if(i%2==0){ // every 2 part, it's a new sending of the file.
            if(i>0)
                printf("\n avarage times recv of file num %d nano seconds %ld\n", (i/2)+1, sumOfTimes/2);
            printf("\n\nsend number %d:\n",i/2+1);
            sumOfTimes = 0; 
        }
        printf("\ntime %d: %ld nano seconds", i, timeOfReceivingFiles[i]); // print the time in nano seconds 
        sumOfTimes +=timeOfReceivingFiles[i];
    }

    printf("\n avarage times recv of last file is nano seconds %ld\n", sumOfTimes/2); // avarge time of last file
    printf("\n");
    //long avarageOftimes = sumOfTimes/indexOfReveivingFiles; // calculate th avarge time of all sending 

    //printf("\nThe Avarage time for receving all packets is %ld\n", avarageOftimes); // print the avarage time

    close(clientSocket); // close client socket 
    close(listeningSocket); // close server socket
}

