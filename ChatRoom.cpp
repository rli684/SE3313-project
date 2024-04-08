// Including necessary libraries and headers
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include "socket.h"
#include "ChatRoom.h"
#include "thread.h"
#include <stdlib.h>

// Using necessary namespaces
using namespace Sync;
using namespace std;

// Constructor for ChatRoom class
ChatRoom::ChatRoom(const string &roomName) : name(roomName) {}

// Destructor for ChatRoom class
ChatRoom::~ChatRoom()
{
    // Cleanup
    this->isActive = false; // Stop the server loop
    while (!(clients.empty()))
    {
        clients.pop_back();
    }
}

// Method to broadcast a message to all clients except the sender
void ChatRoom::broadcastMessage(const string &message, const string &sender)
{
    ByteArray data(message);

    for (auto &client : clients)
    {
        if (client.name != sender)
        {
            client.socket->Write(data);
        }
    }
}

// Method to check if a user already exists in the chatroom
bool ChatRoom::existingUser(const string &username)
{
    for (auto &client : clients)
    {
        if (client.name == username)
        {
            return true;
        }
        return false;
    }
    return false;
}

// Main thread function for the chatroom
long ChatRoom::ThreadMain()
{
    try
    {
        // Main loop of the chatroom
        while (this->isActive)
        {
        }
    }
    catch (TerminationException error)
    {
        return error; // Handle server shutdown
    }
    return 0;
}

// Method to set the name of the chatroom
void ChatRoom::setChatroomName(const string &newName)
{
    name = newName;
}

// Method to add a client to the chatroom
void ChatRoom::addClient(const string &name, Socket *socket)
{
    clients.emplace_back(name, socket); // Pass Sync::Socket pointer
    string message = "MESSAGE;Server;" + name + " has joined the chatroom.";
    broadcastMessage(message, name);
}

// Method to remove a client from the chatroom by name
void ChatRoom::removeClientByName(const string &clientName)
{
    auto it = clients.begin();
    while (it != clients.end())
    {
        if (it->name == clientName)
        {
            it = clients.erase(it); // Erase element and update iterator
            string message = "MESSAGE;Server;" + clientName + " has left the chatroom.";
            std::cout << message << std::endl;
            broadcastMessage(message, clientName);
            return; // Assuming each client has a unique name, so we can return after erasing one client
        }
        else
        {
            ++it; // Move to the next element
        }
    }
    cout << "Client not found: " << clientName << endl;
    // Throw the runtime error only if the client is not found
}

// Method to get the names of all clients in the chatroom
vector<string> ChatRoom::getClientNames() const
{
    vector<string> names;
    for (const auto &client : clients)
    {
        names.push_back(client.name);
    }
    return names;
}

// Method to get the number of clients in the chatroom
size_t ChatRoom::getClientCount() const
{
    return clients.size();
}

// Method to add a message to a specific client's message list
void ChatRoom::addToClientMessages(const string &clientName, const string &message)
{
    auto it = find_if(clients.begin(), clients.end(), [&clientName](const ClientInfo &info)
                      { return info.name == clientName; });
    if (it != clients.end())
    {
        it->messages.push_back(message);
    }
}

// Method to get the name of the chatroom
string ChatRoom::getRoomName() const
{
    return name;
}

// Method to get the socket of a client by their name
Socket ChatRoom::getClientSocketByName(const string &clientName) const
{
    auto it = find_if(clients.begin(), clients.end(), [&clientName](const ClientInfo &info)
                      { return info.name == clientName; });
    if (it != clients.end())
    {
        return *(it->socket); // Dereference the pointer
    }
    throw runtime_error("Client not found in the chatroom.");
}

// Method to get the name of a client by their socket
string ChatRoom::getClientNameBySocket(const Socket &clientSocket) const
{
    for (const auto &client : clients)
    {
        if (*(client.socket) == clientSocket) // Dereference and compare the pointer
        {
            return client.name;
        }
    }
    return ""; // Return empty string if socket is not found
}

// Method to get messages by client name
vector<string> ChatRoom::getMessagesByClientName(const string &clientName) const
{
    auto it = find_if(clients.begin(), clients.end(), [&clientName](const ClientInfo &info)
                      { return info.name == clientName; });
    if (it != clients.end())
    {
        return it->messages;
    }
    return {}; // Return empty vector if client not found
}

// Method to get the mutex for thread synchronization
std::mutex &ChatRoom::getMutex()
{
    return mutex;
}
