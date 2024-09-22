struct producerStruct {
    int productType;
    int socketDescriptor;
};

//Struct that contains information that can be initiallized and sent to distributor thread.
struct distributorStruct {
    struct Queue *queue1;
    struct Queue *queue2;
    int signalCount;
    int socketDescriptor;
};

//Struct that contains information that can be initiallized and sent to consumer threads.
struct consumerStruct {
    struct Queue *queue;
    int productType;
    int *totalConsumeCount;
    int *consumeSequence;
};
