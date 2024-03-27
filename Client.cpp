#include "socket.h"
#include "thread.h"
#include <iostream>
#include <stdlib.h>
#include <time.h>

using namespace Sync;

int main(void)
{
	std::cout << "I am a Client." << std::endl; // prints initial client message

	Sync::ByteArray serverData; // buffer for data to/from server
	std::string userInput = ""; // stores user input
	// create and open socket connection to server
	Socket socket("127.0.0.1", 3000);
	socket.Open();
	while (userInput != "done") // loops until user types "done"
	{
		std::cin >> userInput; // reads user input
		try
		{
			serverData = Sync::ByteArray(userInput);		 // converts user input into ByteArray
			socket.Write(serverData);						 // sends data to server
			socket.Read(serverData);						 // reads response from server
			std::cout << "Client has typed ";				 // acknowledgment message
			std::cout << serverData.ToString() << std::endl; // prints server's response
		}
		catch (...)
		{
			return 0; // exits on exception
		}
	}
	socket.Close(); // closes the socket connection
	return 0;		// exits the program
}
