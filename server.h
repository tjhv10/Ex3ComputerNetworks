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

#define SERVER_PORT 9998
#define SERVER_IP_ADDRESS "127.0.0.1"




char* readFile();