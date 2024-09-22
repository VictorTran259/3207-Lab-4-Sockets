consumerserver: getandput.o queue.o
	gcc getandput.o queue.o -pthread -o consumerserver consumerserver.c -Wall -Werror
getandput: getandput.c
	gcc -c getandput.c
queue: queue.c
	gcc -c queue.c
clean: 
	rm  *.o consumerserver
