// Including necessary header files
#include "thread.h"
#include "socketserver.h"
#include "ChatRoom.h"

// Including standard library headers
#include <stdlib.h>
#include <time.h>
#include <list>
#include <vector>
#include <algorithm>
#include <string>
#include <iostream>
#include <sstream>

// Using namespaces
using namespace Sync; // Sync namespace for synchronization primitives
using namespace std;  // Standard namespace

// Global variable declarations
vector<ChatRoom *> rooms;          // Vector of pointers to ChatRoom objects
vector<Socket *> connectedClients; // Vector of pointers to Socket objects

// Structure for the chat room consisting of name, password, # of current users, and max user size
struct ChatRoomStructure
{
    std::string name;     // Name of the chat room
    std::string password; // Password for the chat room (if any)
    int current_users;    // Number of current users in the chat room
    int max_users;        // Maximum capacity of the chat room
};

// Function that converts ChatRoom struct data into a byte array for sending over a socket
std::vector<char> chatroomDataToByteArray(const ChatRoomStructure &room)
{
    std::vector<char> data; // Vector to store byte data
    // Converts name to bytes and append to data
    data.insert(data.end(), room.name.begin(), room.name.end());
    data.push_back(';'); // Add delimiter

    // Converts password to bytes and append to data if the room is locked
    if (room.password != "")
    {
        data.insert(data.end(), room.password.begin(), room.password.end());
    }
    data.push_back(';'); // Add delimiter

    // Convert current_users to bytes and append to data
    std::string current_users_str = std::to_string(room.current_users);
    data.insert(data.end(), current_users_str.begin(), current_users_str.end());
    data.push_back(';'); // Add delimiter

    // Converting max_users to bytes and appending to data
    std::string max_users_str = std::to_string(room.max_users);
    data.insert(data.end(), max_users_str.begin(), max_users_str.end());

    return data; // Return byte data
}

// Function to loop through an array of ChatRoom structs and return the data in a byte array format
std::vector<char> getAllChatroomDataAsByteArray(const std::vector<ChatRoomStructure> &rooms)
{
    std::vector<char> allData{};   // Vector to store byte data for all chat rooms
    for (const auto &room : rooms) // Iterate through each chat room
    {
        std::vector<char> roomData = chatroomDataToByteArray(room);      // Get byte data for current room
        allData.insert(allData.end(), roomData.begin(), roomData.end()); // Append room data to allData vector
        allData.push_back('\n');                                         // Add newline delimiter between room data
    }
    return allData; // Return byte data for all chat rooms
}

// Function to remove clients from chat room when they leave/disconnect
void removeClient(Socket *clientSocket)
{
    // Find the position of the client socket in the array
    auto it = std::find(connectedClients.begin(), connectedClients.end(), clientSocket);
    if (it != connectedClients.end()) // Check if client socket found
    {
        // Erase the client socket from the array
        connectedClients.erase(it);
    }
}

// Room name, Password, # of Current Users, Max Capacity, are parameters to the room_data
std::vector<ChatRoomStructure> room_data{}; // Vector to store chat room data

// Get chatroom data as byte array
std::vector<char> chatroomBytes = getAllChatroomDataAsByteArray(room_data); // Byte array containing chat room data

// Function to send updated data to all clients
void sendUpdatedDataToAllClients(const Sync::ByteArray &updatedData, const Socket &clientSocket, const bool sendClient = false)
{
    // Iterate through all connected clients
    for (const auto &socket : connectedClients)
    {
        if (*socket != clientSocket || sendClient) // Check if current socket is not the client that initiated update
        {
            std::ostringstream os; // String stream for constructing update message
            os << "UPDATE_DATA;";  // Append update command to message
            // Send the updated data to the current connected client
            if (!room_data.empty()) // Check if there is chat room data available
            {
                os << updatedData.ToString() << endl; // Append updated data to message
            }
            cout << os.str() << endl;                             // Output the update message
            Sync::ByteArray sendData = Sync::ByteArray(os.str()); // Create byte array from message
            (*socket).Write(sendData);                            // Write data to client socket
        }
    }
}

// Mutex declarations to protect shared data
std::mutex roomsMutex;            // Mutex to protect access to rooms vector
std::mutex roomDataMutex;         // Mutex to protect access to room_data vector
std::mutex chatroomBytesMutex;    // Mutex to protect access to chatroomBytes vector
std::mutex connectedClientsMutex; // Mutex to protect access to connectedClients vector

// This class manages individual client connections in separate threads
class SocketThreadIndividually : public Thread
{
private:
    Socket &clientSocket;            // Reference to client socket
    Sync::ByteArray incomingData;    // Buffer for incoming data
    std::vector<char> &chatroomData; // Reference to chat room data

public:
    bool isActive = true; // Flag to control the thread loop
    SocketThreadIndividually(Socket &socket, std::vector<char> &chatroomData)
        : clientSocket(socket), chatroomData(chatroomData) // Constructor
    {
    }

    ~SocketThreadIndividually() // Destructor
    {
        // Destructor
    }

    // Get the chat room of the client
    ChatRoom *getRoom(string &roomName)
    {
        for (auto &room : rooms) // Loop through each room
        {
            if (room->getRoomName() == roomName) // Check if room name matches
            {
                return room; // Return pointer to the room
            }
        }
        return nullptr; // Return nullptr if room not found
    }

    virtual long ThreadMain() // Main thread function
    {
        ChatRoom *clientRoom = nullptr; // Pointer to client's chat room
        // Main thread loop, waits for data and echoes it back
        while (this->isActive)
        {
            try
            {
                clientSocket.Read(incomingData);              // Read data from client
                string receivedMsg = incomingData.ToString(); // Convert incoming data to string

                std::lock_guard<std::mutex> roomsLocks(roomsMutex);                    // Lock mutex to access rooms vector
                std::lock_guard<std::mutex> roomDataLock(roomDataMutex);               // Lock mutex to access room_data vector
                std::lock_guard<std::mutex> chatroomBytesLock(chatroomBytesMutex);     // Lock mutex to access chatroomBytes vector
                std::lock_guard<std::mutex> connectClientsLock(connectedClientsMutex); // Lock mutex to access connectedClients vector

                if (receivedMsg == "DISCONNECT") // Check if client wants to disconnect
                {
                    removeClient(&clientSocket); // Remove client from chat room
                    break;                       // Exit the loop
                }

                // Separate received messages
                std::istringstream iss(receivedMsg); // Create string stream from received message
                std::vector<std::string> segments;   // Vector to store message segments
                std::string segment;                 // Temporary string to store each segment

                // Split message into segments using delimiter ';'
                while (std::getline(iss, segment, ';')) // Loop until ';' delimiter found
                {
                    segments.push_back(segment); // Add segment to vector
                }

                if (segments[0] == "DISCONNECT_ROOM") // Check if client wants to disconnect from a room
                {
                    connectedClients.push_back(&clientSocket); // Add client to connected clients list
                    string roomName = segments[1];             // Get room name from message
                    string sender = segments[2];               // Get sender's name from message

                    clientRoom = this->getRoom(roomName); // Get pointer to client's chat room

                    std::mutex &roomMutex = clientRoom->getMutex(); // Get mutex of client's chat room
                    std::lock_guard<std::mutex> lock(roomMutex);    // Lock the mutex

                    clientRoom->removeClientByName(sender); // Remove client from chat room
                    int num = clientRoom->getClientCount(); // Get number of clients in the chat room
                    if (num == 0)                           // If no clients left in the chat room
                    {
                        auto it = std::find(rooms.begin(), rooms.end(), clientRoom); // Find chat room in rooms vector

                        if (it != rooms.end()) // If chat room found
                        {
                            // Erase the chat room from the vector
                            rooms.erase(it);
                        }
                        // Erase chat room data from room_data vector
                        room_data.erase(std::remove_if(room_data.begin(), room_data.end(), [&](const ChatRoomStructure &obj)
                                                       { return obj.name == roomName; }),
                                        room_data.end());
                        clientRoom->isActive = false; // Set chat room to inactive
                    }
                    for (auto &room : room_data) // Loop through room data vector
                    {
                        if (room.name == roomName) // Check if room name matches
                        {
                            room.current_users--; // Decrement current_users by one
                        }
                    }
                    chatroomBytes = getAllChatroomDataAsByteArray(room_data);                    // Update chat room data byte array
                    Sync::ByteArray chatroomByteArray(chatroomData.data(), chatroomData.size()); // Create byte array from chat room data
                    sendUpdatedDataToAllClients(chatroomByteArray, clientSocket, true);          // Send updated data to all clients
                }
                else if (segments[0] == "MESSAGE_ROOM") // Check if message is intended for a room
                {
                    string roomName = segments[1];                      // Get room name from message
                    string message = segments[2];                       // Get message content from message
                    string sender = segments[3];                        // Get sender's name from message
                    string final = "MESSAGE;" + sender + ";" + message; // Construct final message
                    clientRoom = this->getRoom(roomName);               // Get pointer to client's chat room
                    std::mutex &roomMutex = clientRoom->getMutex();     // Get mutex of client's chat room
                    std::lock_guard<std::mutex> lock(roomMutex);        // Lock the mutex
                    clientRoom->broadcastMessage(final, sender);        // Broadcast the message to all other users in the chat room
                }
                else if (!segments.empty() && segments[0] == "CREATE_ROOM" && segments.size() == 6) // Check if client wants to create a new room
                {
                    removeClient(&clientSocket);                    // Remove client from current chat room
                    string roomName = segments[1];                  // Get room name from message
                    string clientName = segments[5];                // Get client's name from message
                    rooms.push_back(new ChatRoom(roomName));        // Create new chat room
                    clientRoom = this->getRoom(roomName);           // Get pointer to new chat room
                    std::mutex &roomMutex = clientRoom->getMutex(); // Get mutex of new chat room
                    std::lock_guard<std::mutex> lock(roomMutex);    // Lock the mutex
                    // Add new room data to room_data vector
                    room_data.emplace_back(ChatRoomStructure{segments[1], segments[2], std::stoi(segments[3]), std::stoi(segments[4])});
                    // Update chatroomBytes vector with new room data
                    chatroomBytes = getAllChatroomDataAsByteArray(room_data);
                    Sync::ByteArray chatroomByteArray(chatroomData.data(), chatroomData.size()); // Create byte array from chat room data
                    sendUpdatedDataToAllClients(chatroomByteArray, clientSocket);                // Send updated data to all clients
                    clientRoom->addClient(clientName, &clientSocket);                            // Add client to new chat room
                    Sync::ByteArray sendData = Sync::ByteArray("CREATE_SUCCESS");                // Create success message
                    clientSocket.Write(sendData);                                                // Send success message to client
                }
                // Existing JOIN_ROOM handling inside ThreadMain

                else if (!segments.empty() && segments[0] == "JOIN_ROOM" && segments.size() == 4) // Check if client wants to join a room
                {
                    for (auto &room : room_data) // Loop through room data vector
                    {
                        if (room.name == segments[1]) // Check if room name matches
                        {
                            if ((segments[2] == "NO_PASSWORD" || room.password == segments[2]) && room.current_users < room.max_users) // Check if password is correct and room is not full
                            {
                                string roomName = segments[1];   // Get room name from message
                                string clientName = segments[3]; // Get client's name from message
                                // Logic to loop through room threads to find which room has given name
                                clientRoom = this->getRoom(roomName); // Get pointer to client's chat room

                                std::mutex &roomMutex = clientRoom->getMutex(); // Get mutex of client's chat room
                                std::lock_guard<std::mutex> lock(roomMutex);    // Lock the mutex

                                if (clientRoom->existingUser(clientName)) // Check if client already exists in the chat room
                                {
                                    Sync::ByteArray sendData = Sync::ByteArray("EXISTING_USER"); // Create existing user message
                                    clientSocket.Write(sendData);                                // Send existing user message to client
                                }
                                else
                                {
                                    removeClient(&clientSocket); // Remove client from current chat room
                                    for (auto &room : room_data) // Loop through room data vector
                                    {
                                        if (room.name == roomName) // Check if room name matches
                                        {
                                            room.current_users++; // Increment current_users by one
                                        }
                                    }
                                    // Join room logic here (not detailed, as your focus is on password validation)
                                    chatroomBytes = getAllChatroomDataAsByteArray(room_data);                    // Update chat room data byte array
                                    Sync::ByteArray chatroomByteArray(chatroomData.data(), chatroomData.size()); // Create byte array from chat room data
                                    sendUpdatedDataToAllClients(chatroomByteArray, clientSocket);                // Send updated data to all clients
                                    clientRoom->addClient(clientName, &clientSocket);                            // Add client to chat room
                                    Sync::ByteArray sendData = Sync::ByteArray("JOIN_SUCCESS");                  // Create join success message
                                    clientSocket.Write(sendData);                                                // Send join success message to client
                                }
                            }
                            else if (room.max_users <= room.current_users) // Check if room is full
                            {
                                Sync::ByteArray sendData = Sync::ByteArray("ROOM_FULL"); // Create room full message
                                clientSocket.Write(sendData);                            // Send room full message to client
                            }
                            else if (room.password != segments[2]) // Check if password is incorrect
                            {
                                Sync::ByteArray sendData = Sync::ByteArray("INVALID_PASSWORD"); // Create invalid password message
                                clientSocket.Write(sendData);                                   // Send invalid password message to client
                            }
                            else
                            {
                                Sync::ByteArray sendData = Sync::ByteArray("NO_ROOM"); // Create no room message
                                clientSocket.Write(sendData);                          // Send no room message to client
                            }
                        }
                    }
                }
            }
            catch (...) // Catch all exceptions
            {
            }
        }
        return 0;
    }

    void SendChatroomData() // Function to send chat room data to client
    {
        if (room_data.empty()) // Check if there are no chat rooms
        {
            Sync::ByteArray sendData = Sync::ByteArray("NO_ROOMS"); // Create no rooms message
            clientSocket.Write(sendData);                           // Send no rooms message to client
            return;
        }
        Sync::ByteArray chatroomByteArray(chatroomData.data(), chatroomData.size()); // Create byte array from chat room data
        clientSocket.Write(chatroomByteArray);                                       // Send chat room data to client
    }
};

// This thread handles the server operations, accepting connections and creating client threads
class ServerThread : public Thread
{
private:
    SocketServer &server;                             // Reference to server socket
    bool isActive = true;                             // Flag to control the server loop
    vector<SocketThreadIndividually *> clientThreads; // Vector to store pointers to client threads
    std::vector<char> &chatroomData;                  // Reference to chat room data

public:
    ServerThread(SocketServer &server, std::vector<char> &chatroomData)
        : server(server), chatroomData(chatroomData) // Constructor
    {
    }

    ~ServerThread() // Destructor
    {
        // Cleanup
        cout << "Cleaning up threads!" << endl; // Output cleanup message
        cout.flush();                           // Flush output buffer
        this->isActive = false;                 // Set server loop flag to false
        while (!(clientThreads.empty()))        // Loop until client threads vector is empty
        {
            clientThreads.back()->isActive = false; // Set individual client thread flag to false
            clientThreads.pop_back();               // Remove client thread from vector
        }
    }

    virtual long ThreadMain() // Main thread function
    {
        // Main server loop, waits for new connections
        while (this->isActive)
        {
            try
            {
                Socket *newConnection = new Socket(server.Accept());                            // Accept new client connection
                Socket &socketRef = *newConnection;                                             // Get reference to new socket
                std::lock_guard<std::mutex> roomsLocks(roomsMutex);                             // Lock mutex to access rooms vector
                std::lock_guard<std::mutex> roomDataLock(roomDataMutex);                        // Lock mutex to access room_data vector
                std::lock_guard<std::mutex> chatroomBytesLock(chatroomBytesMutex);              // Lock mutex to access chatroomBytes vector
                std::lock_guard<std::mutex> connectClientsLock(connectedClientsMutex);          // Lock mutex to access connectedClients vector
                connectedClients.push_back(&socketRef);                                         // Add client to connected clients list
                clientThreads.push_back(new SocketThreadIndividually(socketRef, chatroomData)); // Create and store new client thread
                clientThreads.back()->SendChatroomData();                                       // Send chat room data to new client
            }
            catch (TerminationException error) // Catch termination exception
            {
                return error; // Return error code
            }
            catch (...) // Catch all exceptions
            {
                return 1; // Return generic error code
            }
        }
        return 1; // Return generic error code
    }
};

int main(void)
{
    cout << "I am a server." << endl;                           // Output initial server message
    SocketServer server(2004);                                  // Create server socket listening on port 2004
    ServerThread serverOpThread(server, chatroomBytes);         // Create server operation thread
    cout << "Type 'SHUTDOWN' to shut down the server." << endl; // Output shutdown instruction
    // Wait for 'SHUTDOWN' keyword to shutdown
    string input; // Variable to store user input
    while (true)  // Infinite loop
    {
        getline(cin, input);     // Read input from user
        if (input == "SHUTDOWN") // Check if input is 'SHUTDOWN'
        {
            cout << "Shutting down the server..." << endl; // Output shutdown message
            // Send shutdown message to all connected clients
            Sync::ByteArray shutdownMessage = Sync::ByteArray("SERVER_SHUTDOWN"); // Create shutdown message
            for (auto &clientSocket : connectedClients)                           // Loop through connected clients
            {
                (*clientSocket).Write(shutdownMessage); // Send shutdown message to client
            }
            server.Shutdown(); // Shutdown the server
            break;             // Exit the loop
        }
        else
        {
            cout << "Type 'SHUTDOWN' to shut down the server." << endl; // Output instruction to type 'SHUTDOWN'
        }
    }
    return 0;
}