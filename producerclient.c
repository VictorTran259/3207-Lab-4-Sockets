#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "structs.h"

void *producer(void *arg);

pthread_mutex_t mutex;

int main(int argc, char **argv) {
    //Check to see if the user entered a port number
    if(argv[1] == NULL) {
    	printf("Error: no port number was entered\n");
    	exit(-1);
    }
    
    //I COPIED THIS SOCKET CODE FROM BINARYTIDES
    //LINK: https://www.binarytides.com/socket-programming-c-linux-tutorial
    int portNumber = atoi(argv[1]);
    int socket_desc;
    struct sockaddr_in server;
    
    //Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_desc == -1) {
    	printf("Could not create socket");
    	exit(-1);
    }
    
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(portNumber);
    
    //Connect to remote server
    if(connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0 ){
    	printf("Error: failed to connect to server\n");
    	exit(-1);
    }
    
    //Initialize producerClient structs to be passed to the producer threads.
    struct producerStruct producerStruct1;
    struct producerStruct producerStruct2;
    
    producerStruct1.productType = 1;
    producerStruct2.productType = 2;
    
    producerStruct1.socketDescriptor = socket_desc;
    producerStruct2.socketDescriptor = socket_desc;

    //Initialize mutex variables.
    pthread_mutex_init(&mutex, NULL);

    //Arrays containing the disributor and consumer threads.
    pthread_t producer_tid[2];

    //Create producer thread 1.
    if(pthread_create(&producer_tid[0], NULL, producer, &producerStruct1) != 0) {
        printf("Error: failed to create producer thread.\n");
        exit(-1);
    }
    //Create producer thread 2.
    if(pthread_create(&producer_tid[1], NULL, producer, &producerStruct2) != 0) {
        printf("Error: failed to create producer thread.\n");
        exit(-1);
    }
    
    //Join the producer threads.
    if(pthread_join(producer_tid[0], NULL) != 0) {
        printf("Error: failed to join producer thread 1\n");
        exit(-1);
    }
    if(pthread_join(producer_tid[1], NULL) != 0) {
        printf("Error: failed to join producer thread 2\n");
        exit(-1);
    }

    //Destroy the mutex variables after the program finishes its job and before the program exits.
    pthread_mutex_destroy(&mutex);
}

//Producer function to be used by threads.
void *producer(void *arg) {
    //Cast the arg structure from void* back to productStruct* so the producers have access to the data in the struct.
    struct producerStruct *producerStruct = (struct producerStruct*)arg;

    //Producers will each produce 150 values plus the -1 that signals that the producer is done producing.
    for(int i = 1; i <= 151; i++) {
        //Lock over the critical section.
        pthread_mutex_lock(&mutex);

        int productID;
        int productType = producerStruct->productType;
	
        //The producer will send -1 to the socket signalling that the producer is done producing if i = 151, the last value and also the product type.
        if(i == 151) {
            productID = -1;
            if(send(producerStruct->socketDescriptor, &productID, sizeof(int), 0) < 0) {
            	printf("Failed to send product ID to the socket\n");
            }
            if(send(producerStruct->socketDescriptor, &productType, sizeof(int), 0) < 0) {
            	printf("Failed to send product type to the socket\n");
            }
        }
        //The producer will send the product id to the socket for all other values and also the product type.
        else {
            productID = i;
            if(send(producerStruct->socketDescriptor, &productID, sizeof(int), 0) < 0) {
            	printf("Failed to send product ID to the socket\n");
            }
            if(send(producerStruct->socketDescriptor, &productType, sizeof(int), 0) < 0) {
            	printf("Failed to send product type to the socket\n");
            }
        }

        //End of critical section.
        pthread_mutex_unlock(&mutex);

        //Sleep for 0.2 seconds.
        usleep(200000);
    }
    pthread_exit(NULL);
}
