#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "queue.h"
#include "getandput.h"
#include "structs.h"

void *distributor(void *arg);
void *consumer(void *arg);

pthread_mutex_t mutex1, mutex2;
pthread_cond_t empty, fill;

int main(int argc, char **argv) {
    //Check to see if the user entered a port number
    if(argv[1] == NULL) {
    	printf("Error: no port number was entered\n");
    	exit(-1);
    }
    
    //I COPIED THIS SOCKET CODE FROM BINARYTIDES
    //LINK: https://www.binarytides.com/socket-programming-c-linux-tutorial
    int portNumber = atoi(argv[1]);
    int socket_desc, new_socket, c;
    struct sockaddr_in server, client;
    
    //Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_desc == -1) {
    	printf("Error: failed to create socket\n");
    	exit(-1);
    }
    
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(portNumber);
    
    //Bind
    if(bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
    	puts("Error: bind failed\n");
    	exit(-1);
    }
    else {
    	puts("Binded successfully\n");
    }
    
    //Listen
    listen(socket_desc, 3);
    
    //Accept an incoming connection
    puts("Waiting for incoming connections...\n");
    c = sizeof(struct sockaddr_in);
    new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
    if(new_socket < 0) {
    	perror("accept failed\n");
    	exit(-1);
    }
    
    puts("Connection accepted\n");

    //Remove any prior instances of "log.txt" before continuting on in the program.
    remove("log.txt");

    //Create the buffers
    struct Queue *buf1 = createQueue();
    struct Queue *buf2 = createQueue();

    //Initialize all of the counts to 0.
    int totalConsumeCount = 0;
    int consumeSequence1 = 0;
    int consumeSequence2 = 0;

    //Initialize the two structs that contain the arguments to be passed to the consumers.
    struct consumerStruct consumerStruct1;
    struct consumerStruct consumerStruct2;

    //Initialize the struct that contains the arguments to be passed to the distributor.
    struct distributorStruct distributorStruct;

    //Initialize the data in each of the consumer structs.
    consumerStruct1.productType = 1;
    consumerStruct2.productType = 2;

    consumerStruct1.consumeSequence = &consumeSequence1;
    consumerStruct2.consumeSequence = &consumeSequence2;

    consumerStruct1.totalConsumeCount = &totalConsumeCount;
    consumerStruct2.totalConsumeCount = &totalConsumeCount;

    consumerStruct1.queue = buf1;
    consumerStruct2.queue = buf2;

    //Initialize the data in the distributor struct.
    distributorStruct.queue1 = buf1;
    distributorStruct.queue2 = buf2;
    distributorStruct.signalCount = 0;
    distributorStruct.socketDescriptor = new_socket;

    pthread_t distributor_tid[1];
    pthread_t consumer_tid[4];

    //Initialize mutex and condition variables.
    pthread_mutex_init(&mutex1, NULL);
    pthread_mutex_init(&mutex2, NULL);
    pthread_cond_init(&empty, NULL);
    pthread_cond_init(&fill, NULL);

    //Create distributor thread.
    if(pthread_create(&distributor_tid[0], NULL, distributor, &distributorStruct) != 0) {
        printf("Error: failed to create distributor thread.\n");
        exit(-1);
    }

    //Create consumer thread 1.
    if(pthread_create(&consumer_tid[0], NULL, consumer, &consumerStruct1) != 0) {
        printf("Error: failed to create consumer thread 1.\n");
        exit(-1);
    }
    //Create consumer thread 2.
    if(pthread_create(&consumer_tid[1], NULL, consumer, &consumerStruct1) != 0) {
        printf("Error: failed to create consumer thread 2.\n");
        exit(-1);
    }

    //Create consumer thread 3.
    if(pthread_create(&consumer_tid[2], NULL, consumer, &consumerStruct2) != 0) {
        printf("Error: failed to create consumer thread 3.\n");
        exit(-1);
    }
    //Create consumer thread 4.
    if(pthread_create(&consumer_tid[3], NULL, consumer, &consumerStruct2) != 0) {
        printf("Error: failed to create consumer thread 4.\n");
        exit(-1);
    }
    
    //Join the distributor thread.
    if(pthread_join(distributor_tid[0], NULL) != 0) {
        printf("Error: failed to join distributor thread\n");
        exit(-1);
    }
    
    //Join the consumer threads.
    if(pthread_join(consumer_tid[0], NULL) != 0) {
        printf("Error: failed to join consumer thread 1\n");
        exit(-1);
    }
    if(pthread_join(consumer_tid[1], NULL) != 0) {
        printf("Error: failed to join consumer thread 2\n");
        exit(-1);
    }
    if(pthread_join(consumer_tid[2], NULL) != 0) {
        printf("Error: failed to join consumer thread 3\n");
        exit(-1);
    }
    if(pthread_join(consumer_tid[3], NULL) != 0) {
        printf("Error: failed to join consumer thread 4\n");
        exit(-1);
    }

    //Destroy the mutex and condition variables after the program finishes its job and before the program exits.
    pthread_mutex_destroy(&mutex1);
    pthread_mutex_destroy(&mutex2);
    pthread_cond_destroy(&empty);
    pthread_cond_destroy(&fill);
}

//Distributor function to be used by the distributor thread.
void *distributor(void *arg) {
    //Cast the arg structure from void* back to distributorStruct* so the consumers have access to the data in the struct.
    struct distributorStruct *distributorStruct = (struct distributorStruct*)arg;

    //Will continue distributing until producers send -1 into the pipe.
    //Will enqueue -1 into the queue to tell the consumers to stop.
    while(distributorStruct->signalCount != 2) {
        //Beginning of critical section 1.
        pthread_mutex_lock(&mutex1);

        int productID;
        int productType;
	
	//Read the product ID from the socket which will be the first value in the 2 value sequence.
	//Throw an error if the read failed for some reason.
        if(recv(distributorStruct->socketDescriptor, &productID, sizeof(int), 0) == -1) {
            printf("Failed to read product ID from the socket\n");
        }
        //Read the product type from the socket which will be the second value in the 2 value sequence.
	//Throw an error if the read failed for some reason.
        if(recv(distributorStruct->socketDescriptor, &productType, sizeof(int), 0) == -1) {
            printf("Failed to read product type from the socket\n");
        }

        //distributorStruct->signalCount will equal 2 when both producers have sent -1 into the pipe.
        //When the distributor encounters a -1 in the pipe, increment the signal count.
        if(productID == -1 && distributorStruct->signalCount != 2) {
            distributorStruct->signalCount += 1;
        }
	
	//Beginning of critical section 2.
	pthread_mutex_lock(&mutex2);
	
        //Enqueues the product id into queue 1 if it's of product type 1.
        if(productType == 1) {
            while(getSize(distributorStruct->queue1) == MAX_SIZE) {
                pthread_cond_wait(&empty, &mutex1);
            }
            put(distributorStruct->queue1, productID);
            pthread_cond_signal(&fill);
        }
        //Enqueues the product id into queue 2 if it's of product type 2.
        if(productType == 2) {
            while(getSize(distributorStruct->queue2) == MAX_SIZE) {
                pthread_cond_wait(&empty, &mutex1);
            }
            put(distributorStruct->queue2, productID);
            pthread_cond_signal(&fill);
        }
        
        //End of critical section 2.
        pthread_mutex_unlock(&mutex2);

        //End of critical section 1.
        pthread_mutex_unlock(&mutex1);
        //The distributor has to sleep for a little bit or the program bugs out.
        usleep(100000);
    }
    pthread_exit(NULL);
}

//Consumer function to be used by threads.
//***BASED ON CODE FROM THREE EASY PIECES CHAPTER 30 PAGE 13***
void *consumer(void *arg) {
    //Cast the arg structure from void* back to consumerStruct* so the consumers have access to the data in the struct.
    struct consumerStruct *consumerStruct = (struct consumerStruct*)arg;

    //Consumers will keep going until the producer sends a -1 into the queue signalling that it's done producing.
    while(getFrontVal(consumerStruct->queue) != -1) {
        //Beginning of critical section 1.
        pthread_mutex_lock(&mutex1);

        //Consumer waits until the queue isn't empty before continuing.
        while(getSize(consumerStruct->queue) == 0) {
            pthread_cond_wait(&fill, &mutex1);
        }
	
	//Beginning of critical section 2.
	pthread_mutex_lock(&mutex2);
	
        //Buffer that holds the value that was removed from the queue.
        int temp = get(consumerStruct->queue);
        
        //End of critical section 2.
        pthread_mutex_unlock(&mutex2);

        //Increment the consumer sequence and total consumer count by 1.
        *(consumerStruct->consumeSequence) += 1;
        *(consumerStruct->totalConsumeCount) += 1;

        //Print the information to standard output.
        printf("Product ID: %5d | Product Type: %5d | Thread ID: %5lu | Prod SEQ #: %5d | Consume SEQ #: %5d | Total Consume Count: %5d\n", 
        temp, consumerStruct->productType, pthread_self(), temp, *(consumerStruct->consumeSequence), *(consumerStruct->totalConsumeCount));

        //Code I borrowed from my project 2 that redirects the standard output to a file which will be the "log.txt" file in this case.
        //Buffers to hold the original file descriptors so the file descriptors can be reset later.
        int originalSTDOUT = dup(STDOUT_FILENO);
        int originalSTDIN = dup(STDIN_FILENO);

        //Initialize the output file variable.
        int outputfile;

        //Open the output file. File is created if it doesn't exist and is opened in append mode so no previous information is lost.
        outputfile = open("log.txt", O_WRONLY | O_APPEND | O_CREAT,  0777);

        //Throw an error if "log.txt" failed to open.
        if(outputfile == -1) {
            printf("failed to open file.\n");
        }
        else {
            //Change STDOUT to the output file.
            dup2(outputfile, STDOUT_FILENO);
            close(outputfile);
        }

        //Print the information to the "log.txt" file.
        printf("Product ID: %5d | Product Type: %5d | Thread ID: %5lu | Prod SEQ #: %5d | Consume SEQ #: %5d | Total Consume Count: %5d\n", 
        temp, consumerStruct->productType, pthread_self(), temp, *(consumerStruct->consumeSequence), *(consumerStruct->totalConsumeCount));

        //Reset the file descriptors to their original values.
        dup2(originalSTDOUT, STDOUT_FILENO);
        dup2(originalSTDIN, STDIN_FILENO);
        close(originalSTDIN);
        close(originalSTDOUT);
        
        //End of critical section 1.
        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&mutex1);

        //The program bugs out if the consumers don't sleep so this needs to be here and the sleep time needs to be set to at least 400000 (0.4 seconds).
        //Generally, this value probably has to be a little over double the value of the sleep time for the producer process in order to consistently work.
        usleep(450000);
    }
    pthread_exit(NULL);
}
