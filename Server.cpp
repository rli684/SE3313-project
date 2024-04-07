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

// global variable declarations
vector<ChatRoom *> rooms;
vector<Socket *> connectedClients;

// structure for the chat room consisting of name, password, # of current users, and max user size
struct ChatRoomStructure
{
    std::string name;
    std::string password;
    int current_users;
    int max_users;
};

// function that converts ChatRoom struct data into a byte array for sending over a socket
std::vector<char> chatroomDataToByteArray(const ChatRoomStructure &room)
{
    std::vector<char> data;
    // converts name to bytes and append to data
    data.insert(data.end(), room.name.begin(), room.name.end());
    data.push_back(';'); // add delimiter

    // converts password to bytes and append to data if the room is locked
    if (room.password != "")
    {
        data.insert(data.end(), room.password.begin(), room.password.end());
    }
    data.push_back(';'); // add delimiter

    // convert current_users to bytes and append to data
    std::string current_users_str = std::to_string(room.current_users);
    data.insert(data.end(), current_users_str.begin(), current_users_str.end());
    data.push_back(';'); // add delimiter

    // converting max_users to bytes and appending to data
    std::string max_users_str = std::to_string(room.max_users);
    data.insert(data.end(), max_users_str.begin(), max_users_str.end());

    return data;
}

// function to loop through an array of ChatRoom structs and return the data in a byte array format
std::vector<char> getAllChatroomDataAsByteArray(const std::vector<ChatRoomStructure> &rooms)
{
    std::vector<char> allData{};
    for (const auto &room : rooms)
    {
        std::vector<char> roomData = chatroomDataToByteArray(room);
        allData.insert(allData.end(), roomData.begin(), roomData.end());
        allData.push_back('\n'); // add newline delimiter between room data
    }
    return allData;
}


// function to remove clients from chat room when they leave/disconnect
void removeClient(Socket *clientSocket)
{
    // find the position of the client socket in the array
    auto it = std::find(connectedClients.begin(), connectedClients.end(), clientSocket);
    if (it != connectedClients.end())
    {
        // erase the client socket from the array
        connectedClients.erase(it);
    }
}

// Room name, Password, # of Current Users, Max Capacity, are parameters to the room_data
std::vector<ChatRoomStructure> room_data{};

// get chatroom data as byte array
std::vector<char> chatroomBytes = getAllChatroomDataAsByteArray(room_data);

void sendUpdatedDataToAllClients(const Sync::ByteArray &updatedData, const Socket &clientSocket, const bool sendClient = false)
{
    // iterate through all connected clients
    for (const auto &socket : connectedClients)
    {
        if (*socket != clientSocket || sendClient) // current socket is not the client that initiated update
        {
            Sync::ByteArray sendData = Sync::ByteArray("UPDATE_DATA;"); // update all other clients when users disconnect from room
            (*socket).Write(sendData);
            // send the updated data to the current connected client
            if (!room_data.empty())
            {
                (*socket).Write(updatedData);
            }
        }
    }
}

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
                if (receivedMsg == "DISCONNECT")
                {
                    removeClient(&clientSocket);
                    break;
                }

                // seperating received msgs here
                std::istringstream iss(receivedMsg);
                std::vector<std::string> segments;
                std::string segment;

                while (std::getline(iss, segment, ';'))
                {
                    segments.push_back(segment);
                }

                if (segments[0] == "DISCONNECT_ROOM")
                {
                    string roomName = segments[1];
                    string sender = segments[2];
                    clientRoom = this->getRoom(roomName);
                    clientRoom->removeClientByName(sender);
                    int num = clientRoom->getClientCount();
                    if (num == 0)
                    {
                        auto it = std::find(rooms.begin(), rooms.end(), clientRoom);

                        if (it != rooms.end())
                        {
                            // erase the element using the iterator
                            rooms.erase(it);
                        }
                        room_data.erase(std::remove_if(room_data.begin(), room_data.end(), [&](const ChatRoomStructure &obj)
                                                       { return obj.name == roomName; }),
                                        room_data.end());
                        clientRoom->isActive = false;
                    }
                    for (auto &room : room_data)
                    {
                        if (room.name == roomName)
                        {
                            room.current_users--; // increment current_users by one, frees up space for another user to join
                        }
                    }
                    chatroomBytes = getAllChatroomDataAsByteArray(room_data);
                    Sync::ByteArray chatroomByteArray(chatroomData.data(), chatroomData.size());
                    sendUpdatedDataToAllClients(chatroomByteArray, clientSocket, true);
                }
                else if (segments[0] == "MESSAGE_ROOM") // message byteArray was sent, process the message
                {
                    string roomName = segments[1];
                    string message = segments[2];
                    string sender = segments[3];
                    string final = "MESSAGE;" + sender + ";" + message;
                    clientRoom = this->getRoom(roomName);
                    clientRoom->broadcastMessage(final, sender); // broadcast the message to all other users to view live chat
                }
                else if (!segments.empty() && segments[0] == "CREATE_ROOM" && segments.size() == 6) // user room creation byteArray received, process it
                {
                    // pushing new room to room list
                    room_data.emplace_back(ChatRoomStructure{segments[1], segments[2], std::stoi(segments[3]), std::stoi(segments[4])});
                    // Updating byte array of room data
                    chatroomBytes = getAllChatroomDataAsByteArray(room_data);
                    Sync::ByteArray chatroomByteArray(chatroomData.data(), chatroomData.size());
                    sendUpdatedDataToAllClients(chatroomByteArray, clientSocket);
                    string roomName = segments[1];
                    string clientName = segments[5];
                    rooms.push_back(new ChatRoom(roomName));
                    clientRoom = this->getRoom(roomName);
                    clientRoom->addClient(clientName, &clientSocket);
                    Sync::ByteArray sendData = Sync::ByteArray("CREATE_SUCCESS");
                    clientSocket.Write(sendData);
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
                                string roomName = segments[1];
                                string clientName = segments[3];
                                // logic to loop through room threads to find which room has given name
                                clientRoom = this->getRoom(roomName);
                                if (clientRoom->existingUser(clientName))
                                {
                                    Sync::ByteArray sendData = Sync::ByteArray("EXISTING_USER");
                                    clientSocket.Write(sendData);
                                }
                                else
                                {
                                    for (auto &room : room_data)
                                    {
                                        if (room.name == roomName)
                                        {
                                            room.current_users++; // Increment current_users by one
                                        }
                                    }
                                    // Join room logic here (not detailed, as your focus is on password validation)
                                    chatroomBytes = getAllChatroomDataAsByteArray(room_data);
                                    Sync::ByteArray chatroomByteArray(chatroomData.data(), chatroomData.size());
                                    sendUpdatedDataToAllClients(chatroomByteArray, clientSocket);
                                    clientRoom->addClient(clientName, &clientSocket);
                                    Sync::ByteArray sendData = Sync::ByteArray("JOIN_SUCCESS");
                                    clientSocket.Write(sendData);
                                }
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
                std::cout << "here" << std ::endl;
                // exception handling (silently ignores errors)
            }
        }
        return 0;
    }

    void SendChatroomData()
    {
        if (room_data.empty())
        {
            Sync::ByteArray sendData = Sync::ByteArray("NO_ROOMS");
            clientSocket.Write(sendData);
            return;
        }
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
                Socket *newConnection = new Socket(server.Accept()); // accepts new client connection
                Socket &socketRef = *newConnection;                  // gets reference to the new socket
                connectedClients.push_back(&socketRef);
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
    cout << "Type 'SHUTDOWN' to shut down the server." << endl;
    // Wait for 'SHUTDOWN' keyword to shutdow
    string input;
    while (true)
    {
        getline(cin, input); // Read the entire line of input
        if (input == "SHUTDOWN")
        {
            cout << "Shutting down the server..." << endl;
            // Send shutdown message to all connected clients
            Sync::ByteArray shutdownMessage = Sync::ByteArray("SERVER_SHUTDOWN");
            for (auto &clientSocket : connectedClients)
            {
                (*clientSocket).Write(shutdownMessage);
            }
            server.Shutdown(); // shut down the serve
            break;
        }
        else
        {
            cout << "Type 'SHUTDOWN' to shut down the server." << endl;
        }
    }
    return 0; // exits the program
}
