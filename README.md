# CIS 3207 Lab 4 - Sockets

## Project files

    producerclient.c
    consumerserver.c
    getandput.c
    getandput.h
    queue.c
    queue.h
    structs.h
    makefile

## FUNCTIONS AND STRUCTS

### In producerclient.c
    main(int argc, char **argv);
        The main function of the client. First it checks to see if the user entered a port number as an argument. If the user didn't enter a port number, the client won't run.
        Then it creates the socket and tries to connect to the server using the socket. If the client fails to connect to the server, an error message will be printed and
        the client will stop executing. If the client connected successfully, it initializes the producerStructs to be passed to the threads, the lock, and the producer threads.
        Finally, it joins the producer threads and destroys the lock after the client is finished sending products. The socket code is copied directly from binarytides and 
        I put a link to it in a comment in "consumerserver.c".
        
    void *producer(void *arg);
        Takes in the argument passed to it by the initialization of the thread which in this case, is going to be one of the producerStruct structs initialized in main.
        First casts the struct back to a struct pointer from the void pointer. Then it enters a for loop that executes 150 times producing 150 products + 1 more time for
        the -1 signal for a total of 151 times. Each time the for loop executes, it writes the product number it produced which is just the i variable and also the product
        number which came from the struct that was passed to the thread to the socket. There are locks over the critical section in this section of code. This code is based
        on the code on page 13 of chapter 30 of Three Easy Pieces but I made quite a few modifications to it.

### In consumerserver.c
    main(int argc, char **argv);
        The main function of the server. First it checks to see if the user entered a port number as an argument. If the user didn't enter a port number, the server won't run.
        Then it creates the socket, binds it, starts to listen for a connection, and then accepts it. Then it removes any instances of "log.txt" in the current working directory
        so there aren't any overlapping entries between each run of this program. Then it initializes the buffers, initializes the structs consumerStruct1,
        consumerStruct2 to be passed to the consumer threads later on and the struct distributorStruct which will be passed to the distributor thread later, and initializes
        the locks and condition variables. It then creates the distributor, and all the consumer threads and then joins the distributor and consumer threads. Finally, it
        destroys the lock and condition variables after the program is finished executing. The socket code is copied directly from binarytides and I put a link to it in a
        comment in "consumerserver.c".

    void *distributor(void *arg);
        Takes in the argument passed to it by the initialization of the thread which in this case, is going to be the distributorStruct struct initialized in main. First 
        casts the struct back to a struct pointer from the void pointer. Then it enters a while loop that only terminates once the two -1 signal variables have been sent
        to the socket by the producers. It keeps track of this using a counter variable which is in the struct distributorStruct passed to the distributor that it increments
        whenever -1 is read from the socket until two -1 signal variables have been read from the socket. The distributor reads from the socket twice each time to get the
        product id and the corresponding product type. It then enqueues the product id in its corresponding queue using two if statements to check the product type and the
        put() function. Locks and condition variables are used over the critical sections of the code and to stop the thread while a queue is full.

    void *consumer(void *arg);
        Takes in the argument passed to it by the initialization of the thread which in this case, is going to be one of the consumerStruct structs initialized in main. First
        casts the struct back to a struct pointer from the void pointer. Then it enters a while loop that only terminates once the front node of the consumer's corresponding
        queue has the -1 signal variable in it. It then consumes the product at the front of the queue using the get() function and stores the product id in a temp variable for
        use later in printing out the information. Then the consumeSequence and totalConsumeCount variables in the consumerStruct struct that was passed to the thread are
        incremented for use later in printing out the information. Finally, the consumer will print out the product id using the temp variable from earlier, the product type 
        stored in the consumerStruct struct passed to the consumer, the thread id from pthread_self(), the prod sequence # which is the same as the product id stored in the
        temp variable from earlier, and the consume sequence # and total consume count which are in the arg struct passed to the consumer. The information is also redirected
        to a "log.txt" file which I accomplished using code for redirecting standard output to a file from my project 2. That code uses dup2 to redirect the output to a file
        and saves the original file descriptors before doing this so the consumer can reset them to their original values each time the consumer is done redirecting the
        information to the "log.txt" file. Locks and condition variables are used over the critical sections of the code and to stop the thread while the queue passed to the
        consumer is empty. This code is based on the code on page 13 of chapter 30 of Three Easy Pieces but I made quite a few modifications to it.

### In structs.h
    struct producerStruct {
        int productType; (Initialized to either 1 or 2 for the producer threads to reference.)
        int socketDescriptor; (The socket descriptor so the producer threads can access it.)
    }

    struct distributorStruct {
        struct Queue *queue1; (A pointer to the first queue so the distributor thread can access it.)
        struct Queue *queue2; (A pointer to the second queue so the distributor thread can access it.)
        int signalCount; (A counter variable that is used by the distributor thread to count how many times -1 was read from the socket.)
        int socketDescriptor; (The socket descriptor so the distributor thread can access it.)
    };

    struct consumerStruct {
        struct Queue *queue; (A pointer to the queue so the consumer threads can access it.)
        int productType; (Initialized to either 1 or 2 for the producer and consumer threads to reference.)
        int *totalConsumeCount; (Initialized to 0 for the consumer threads to reference and also increment for printing out the total consume count.)
        int *consumeSequence; (Initialized to 0 for the consumer threads to reference and also increment for printing out the consume squ #.)
    };

### In getandput.c
    void put(struct Queue* q, int value);
        Takes in a queue and a value and then enqueues the value into the queue.

    int get(struct Queue* q);
        Takes in a queue and then dequeues the value into the queue. Stores the value to be dequeued gotten from getFrontVal() in a temp variable and returns the value that
        was dequeued from the queue after dequeueing the value from the queue.

### In queue.c (Code from geeksforgeeks. Link: https://www.geeksforgeeks.org/queue-linked-list-implementation/?ref=lbp#)
    struct QNode* newNode(int k)
        "A utility function to create a new linked list node". I didn't make any modifications to it.

    struct Queue* createQueue();
        "A utility function to create an empty queue". I didn't make any modifications to it.

    void enQueue(struct Queue* q, int k);
        "The function to add a key k to q". The only modifications I made to it was to make it throw an error if the queue size was already the max size which I used getSize()
        to check and to increment the size of the queue in the Queue struct every time it enqueues something.

    void deQueue(struct Queue* q);
        "Function to remove a key from given queue q". The only modification I made to it was to decrement the size of the queue in the Queue struct every time it dequeues
        something.

    int getSize(struct Queue* q);
        One of the functions I added to queue.c. Takes in a queue and returns the size of the queue by checking its size in its Queue struct.

    int getFrontVal(struct Queue* q);
        One of the functions I added to queue.c. Takes in a queue and returns the front value of the queue if the queue is not empty or 0 if the queue is empty which signals
        that the queue is empty.

    "A linked list (LL) node to store a queue entry" (I didn't make any modifications to it.)
    struct QNode {
        int key;
        struct QNode* next;
    };

    "The queue, front stores the front node of LL and rear stores the last node of LL"
    struct Queue {
        struct QNode *front, *rear;
        int size; (The only modification I made to it for the purpose of bounding the queue)
    };

I couldn't figure out how to make the makefile also compile the producer client so you have to compile that yourself using the command
"gcc -pthread -o producerclient producerclient.c -Wall -Werror".
I have created and included a makefile that compiles the consumer server for quality of life purposes since it's more tedious to compile than the client.

All of the header files contain function prototypes except for struct.h which contains structures to be passed to the threads.

There are many comments included all over producerclient.c, consumerserver.c, getandput.c, and queue.c that go into detail about
the purpose of a certain line of lines of code.

## TESTING
To test the program, simply just run the server and then run the client in that order and check to make sure that the information that it's printing out matches up
with the information below

    Product ID: Numbers ranging from 1-150 in ascending order. There should be two sets of numbers from 1-150 in the output.
    Product Type: Can only be either 1 or 2.
    Thread ID: 4 different numbers for thread IDs should be present in the output.
    Prod SEQ #: Numbers ranging from 1-150 in ascending order. There should be two sets of numbers from 1-150 in the output.
    Consume SEQ #: Numbers ranging from 1-150 in ascending order. There should be two sets of numbers from 1-150 in the output.
    Total Consume Count: Numbers ranging from 1-300 in ascending order.

Make sure to check the log.txt file that gets generated to see if the information printed out to the terminal matches the information inside the
log.txt file. Also make sure that you enter a port number as an argument when executing the server and then enter that same port number when executing the client.
Lastly, make sure that you run the server first and then the client. Otherwise this program won't work.
NOTE: The consumers and distributor have to have some delay otherwise the program will bug out.
