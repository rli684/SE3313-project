#include "thread.h"
#include "socketserver.h"
#include <stdlib.h>
#include <time.h>
#include <list>
#include <vector>
#include <algorithm>

using namespace Sync;
using namespace std;

// this class manages individual client connections in separate threads
class SocketThreadIndividually : public Thread
{
private:
    Socket &clientSocket;         // stores the client socket reference
    Sync::ByteArray incomingData; // buffer for incoming data
public:
    bool isActive = true; // flag to control the thread loop
    SocketThreadIndividually(Socket &socket)
        : clientSocket(socket)
    {
    }

    ~SocketThreadIndividually()
    {
        // destructor
    }

    virtual long ThreadMain()
    {
        // main thread loop, waits for data and echoes it back
        while (this->isActive)
        {
            try
            {
                clientSocket.Read(incomingData); // reads data from client
                if (incomingData.ToString() == "")
                {
                    clientSocket.Write(incomingData); // sends back empty string (client disconnect scenario)
                    break;
                }
                string receivedMsg = incomingData.ToString();                                                // converts received data to string
                receivedMsg += ". This string has been modified by the Server and sent back to the Client."; // modifies the received message
                // prepares modified message for sending
                Sync::ByteArray sendData = Sync::ByteArray(receivedMsg);
                clientSocket.Write(sendData); // sends modified message back to client
            }
            catch (...)
            {
                // exception handling (silently ignores errors)
            }
        }
        return 0;
    }
};

// this thread handles the server operations, accepting connections and creating client threads
class ServerThread : public Thread
{
private:
    SocketServer &server;                             // server socket reference
    bool isActive = true;                             // flag to control the server loop
    vector<SocketThreadIndividually *> clientThreads; // stores client thread pointers
public:
    ServerThread(SocketServer &server)
        : server(server)
    {
    }

    ~ServerThread()
    {
        // cleanup
        cout << "Cleaning up threads!" << endl;
        cout.flush();
        this->isActive = false; // stops the server loop
        while (!(clientThreads.empty()))
        {
            clientThreads.back()->isActive = false; // stops individual client threads
            clientThreads.pop_back();               // removes the thread from the list
        }
    }

    virtual long ThreadMain()
    {
        // main server loop, waits for new connections
        while (this->isActive)
        {
            try
            {
                Socket *newConnection = new Socket(server.Accept());              // accepts new client connection
                Socket &socketRef = *newConnection;                               // gets reference to the new socket
                clientThreads.push_back(new SocketThreadIndividually(socketRef)); // creates and stores new thread for client
            }
            catch (TerminationException error)
            {
                return error; // handles server shutdown
            }
            catch (...)
            {
                return 1; // generic error handling
            }
        }
        return 1;
    }
};

int main(void)
{
    cout << "I am a server." << endl;    // initial server message
    SocketServer server(22);           // creates server socket listening on port 3000
    ServerThread serverOpThread(server); // creates server operation thread
    FlexWait cinWaiter(1, stdin);        // waits for input to shutdown
    cinWaiter.Wait();                    // waits for any input
    cin.get();                           // gets the input (for shutdown)
    server.Shutdown();                   // shuts down the server
    return 0;                            // exits the program
}
