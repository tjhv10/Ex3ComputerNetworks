all: sender receiver

sender: Sender.o
	gcc -Wall Sender.o -o sender
Sender.o: Sender.c 
	gcc -c -Wall Sender.c -o Sender.o	
	
receiver: Receiver.o
	gcc -Wall Receiver.o -o receiver
Receiver.o: Receiver.c
	gcc -c -Wall Receiver.c -o Receiver.o

clean:
	-rm  *.o sender receiver
