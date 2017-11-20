# makefile
all: dataserver client

bounded_buffer.o: bounded_buffer.H bounded_buffer.C 
	g++ -std=c++11 -c -g bounded_buffer.C 

semaphore.o: semaphore.C semaphore.H
	g++ -std=c++11 -c -g semaphore.C

reqchannel.o: reqchannel.H reqchannel.C
	g++ -std=c++11 -c -g reqchannel.C

dataserver: dataserver.C reqchannel.o 
	g++ -std=c++11 -g -o dataserver dataserver.C reqchannel.o -lpthread

client: client.C bounded_buffer.o reqchannel.o semaphore.o bounded_buffer.o
	g++ -std=c++11 -g -o client client.C bounded_buffer.o reqchannel.o semaphore.o -lpthread
	
clean: 
	rm edit client.o dataserver.o reqchannel.o bounded_buffer.o semaphore.o