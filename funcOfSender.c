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

#define SERVER_PORT 9999
#define SERVER_IP_ADDRESS "127.0.0.1"
#define BUFFER_SIZE 1024



char* readFile(){
    	FILE *ptr;
    long textLength, readLength;
    char *text = NULL; 
	
	ptr = fopen("/home/daniel/Desktop/Computer_Networking/assignment3/text.txt","r");
	if(ptr == NULL)
        return NULL; 
    printf("opend file successfully\n");

    // get the length of the file using fseek() and ftell() 
    fseek(ptr, 0L, SEEK_END); 
    textLength = ftell(ptr);
    fseek(ptr, 0L, SEEK_SET);


    text = (char*)malloc((textLength + 1) * sizeof(char) ); 

    if(text == NULL)
        return NULL; 

    readLength =fread(text, sizeof(char), textLength, ptr);
    text[textLength] = '\0';

    if(textLength != readLength){
        printf("something went worng while reading\n");
        free(text);
        text = NULL;
    }
	fclose(ptr);
    printf("closed file successfully\n");
    printf("%ld\n", textLength);
    
    //printf("%s\n", text);
	return text;
}