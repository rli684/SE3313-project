#include "thread.h"
#include "socketserver.h"
#include "ChatRoom.h"

#include <stdlib.h>
#include <time.h>
#include <list>
#include <vector>
#include <algorithm>
#include <string>
#include <iostream>
#include <sstream>
using namespace Sync;
using namespace std;

vector<ChatRoom *> rooms;

struct ChatRoomStructure
{
    std::string name;
    std::string password;
    int current_users;
    int max_users;
    bool locked;
};

// Function to convert ChatRoom struct data into a byte array for sending over a socket
std::vector<char> chatroomDataToByteArray(const ChatRoomStructure &room)
{
    std::vector<char> data;
    // Convert name to bytes and append to data
    data.insert(data.end(), room.name.begin(), room.name.end());
    data.push_back(';'); // Add delimiter

    // Convert password to bytes and append to data if the room is locked
    if (room.password != "")
    {
        data.insert(data.end(), room.password.begin(), room.password.end());
    }
    data.push_back(';'); // Add delimiter

    // Convert current_users to bytes and append to data
    std::string current_users_str = std::to_string(room.current_users);
    data.insert(data.end(), current_users_str.begin(), current_users_str.end());
    data.push_back(';'); // Add delimiter

    // Convert max_users to bytes and append to data
    std::string max_users_str = std::to_string(room.max_users);
    data.insert(data.end(), max_users_str.begin(), max_users_str.end());

    return data;
}

// Function to loop through an array of ChatRoom structs and return the data in a byte array format
std::vector<char> getAllChatroomDataAsByteArray(const std::vector<ChatRoomStructure> &rooms)
{
    std::vector<char> allData;
    for (const auto &room : rooms)
    {
        std::vector<char> roomData = chatroomDataToByteArray(room);
        allData.insert(allData.end(), roomData.begin(), roomData.end());
        allData.push_back('\n'); // Add newline delimiter between room data
    }
    return allData;
}

// Room name, Password, # of Current Users, Max Capacity
std::vector<ChatRoomStructure> room_data = {
    {"Chris's room", "password1", 5, 5},
    {"Elbert's room", "", 1, 5} // No password for unlocked room
};

// Get chatroom data as byte array
std::vector<char> chatroomBytes = getAllChatroomDataAsByteArray(room_data);

// this class manages individual client connections in separate threads
class SocketThreadIndividually : public Thread
{
private:
    Socket &clientSocket;         // stores the client socket reference
    Sync::ByteArray incomingData; // buffer for incoming data
    std::vector<char> &chatroomData;

public:
    bool isActive = true; // flag to control the thread loop
    SocketThreadIndividually(Socket &socket, std::vector<char> &chatroomData)
        : clientSocket(socket), chatroomData(chatroomData)
    {
    }

    ~SocketThreadIndividually()
    {
        // destructor
    }

    ChatRoom *getRoom(string &roomName)
    {
        for (auto &room : rooms) // get the chatRoom of the client
        {
            if (room->getRoomName() == roomName)
            {
                return room;
            }
        }
        return nullptr;
    }

    virtual long ThreadMain()
    {
        ChatRoom *clientRoom = nullptr;
        // main thread loop, waits for data and echoes it back
        while (this->isActive)
        {
            try
            {
                clientSocket.Read(incomingData); // reads data from client
                string receivedMsg = incomingData.ToString();
                std::cout << receivedMsg << std::endl;

                // seperating received msgs here
                std::istringstream iss(receivedMsg);
                std::vector<std::string> segments;
                std::string segment;

                while (std::getline(iss, segment, ';'))
                {
                    segments.push_back(segment);
                }

                std::cout << segments[3] << std::endl;

                if (!segments.empty() && segments[0] == "CREATE_ROOM" && segments.size() == 6) // user room creation
                {
                    // Pushing new room to room list
                    room_data.emplace_back(ChatRoomStructure{segments[1], segments[2], std::stoi(segments[3]), std::stoi(segments[4])});

                    // Updating byte array of room data
                    chatroomBytes = getAllChatroomDataAsByteArray(room_data);
                    string roomName = segments[1];
                    string clientName = segments[5];
                    rooms.push_back(new ChatRoom(roomName));
                    clientRoom = this->getRoom(roomName);
                    clientRoom->addClient(clientName, &clientSocket);
                    Sync::ByteArray sendData = Sync::ByteArray("CREATE_SUCCESS");
                    clientSocket.Write(sendData);
                    return 1;
                }
                // Existing JOIN_ROOM handling inside ThreadMain

                else if (!segments.empty() && segments[0] == "JOIN_ROOM" && segments.size() == 4) // user joins a room
                {
                    for (auto &room : room_data)
                    {
                        if (room.name == segments[1])
                        {
                            if ((segments[2] == "NO_PASSWORD" || room.password == segments[2]) && room.current_users < room.max_users) // successful room connection
                            {
                                // Join room logic here (not detailed, as your focus is on password validation)
                                std::cout << "User joined the room successfully!" << std::endl;
                                chatroomBytes = getAllChatroomDataAsByteArray(room_data);
                                string roomName = segments[1];
                                string clientName = segments[3];
                                // logic to loop through room threads to find which room has given name

                                Sync::ByteArray sendData = Sync::ByteArray("JOIN_SUCCESS");
                                clientSocket.Write(sendData);
                                return 0; // End thread execution after handling
                            }
                            else if (room.max_users <= room.current_users) // room is full, return full room msg
                            {
                                Sync::ByteArray sendData = Sync::ByteArray("ROOM_FULL");
                                clientSocket.Write(sendData);
                            }
                            else if (room.password != segments[2]) // password is wrong, return invalid password msg
                            {
                                Sync::ByteArray sendData = Sync::ByteArray("INVALID_PASSWORD");
                                clientSocket.Write(sendData);
                            }
                            else
                            {
                                Sync::ByteArray sendData = Sync::ByteArray("NO_ROOM");
                                clientSocket.Write(sendData);
                            }
                        }
                    }
                }
            }
            catch (...)
            {
                // exception handling (silently ignores errors)
            }
        }
        return 0;
    }

    void SendChatroomData()
    {
        Sync::ByteArray chatroomByteArray(chatroomData.data(), chatroomData.size());
        clientSocket.Write(chatroomByteArray);
    }
};

// this thread handles the server operations, accepting connections and creating client threads
class ServerThread : public Thread
{
private:
    SocketServer &server;                             // server socket reference
    bool isActive = true;                             // flag to control the server loop
    vector<SocketThreadIndividually *> clientThreads; // stores client thread pointers
    std::vector<char> &chatroomData;

public:
    ServerThread(SocketServer &server, std::vector<char> &chatroomData)
        : server(server), chatroomData(chatroomData)
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
                Socket *newConnection = new Socket(server.Accept());                            // accepts new client connection
                Socket &socketRef = *newConnection;                                             // gets reference to the new socket
                clientThreads.push_back(new SocketThreadIndividually(socketRef, chatroomData)); // creates and stores new thread for client
                clientThreads.back()->SendChatroomData();                                       // Send chatroom data to the new client
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
    cout << "I am a server." << endl;                   // initial server message
    SocketServer server(3000);                          // creates server socket listening on port 3000
    ServerThread serverOpThread(server, chatroomBytes); // creates server operation thread
    FlexWait cinWaiter(1, stdin);                       // waits for input to shutdown
    cinWaiter.Wait();                                   // waits for any input
    cin.get();                                          // gets the input (for shutdown)
    server.Shutdown();                                  // shuts down the server
    return 0;                                           // exits the program
}
