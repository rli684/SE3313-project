all: Server

Server : Server.o thread.o socket.o socketserver.o Blockable.o ChatRoom.o
	g++ -o Server Server.o thread.o socket.o socketserver.o Blockable.o ChatRoom.o -pthread -l rt

ChatRoom.o : ChatRoom.cpp ChatRoom.h
	g++ -c ChatRoom.cpp -std=c++11
	
Blockable.o : Blockable.h Blockable.cpp
	g++ -c Blockable.cpp -std=c++11

Server.o : Server.cpp thread.h socketserver.h
	g++ -c Server.cpp -std=c++11

thread.o : thread.cpp thread.h
	g++ -c thread.cpp -std=c++11

socket.o : socket.cpp socket.h
	g++ -c socket.cpp -std=c++11

socketserver.o : socketserver.cpp socket.h socketserver.h
	g++ -c socketserver.cpp -std=c++11
