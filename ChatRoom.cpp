#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include "socket.h"
#include "ChatRoom.h"
#include "thread.h"
#include <stdlib.h>
using namespace Sync;
using namespace std;

ChatRoom::ChatRoom(const string &roomName) : name(roomName) {}

ChatRoom::~ChatRoom()
{
    // Cleanup
    this->isActive = false; // Stop the server loop
    while (!(clients.empty()))
    {

        clients.pop_back();
    }
}

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

long ChatRoom::ThreadMain()
{
    try
    {
        // Main loop of the chatroom
        while (this->isActive)
        {
            // // Check for incoming messages from clients
            // for (auto &client : clients)
            // {

            //     ByteArray receivedData;
            //     int bytesRead = client.socket->Read(receivedData);
            //     std::cout << receivedData.ToString() << std::endl;
            //     if (bytesRead > 0)
            //     {
            //         string message = receivedData.ToString();
            //         // Broadcast the received message to all clients except the sender
            //         string final = client.name + ";" + message;
            //         std::cout << final << std::endl;
            //         broadcastMessage(final, client.name);
            //     }
            //     else
            //     {
            //         std::cout << "im here" << std::endl;
            //         // Client disconnected
            //         removeClientByName(client.name);
            //         break;
            //     }
            // }

            // // Sleep for a short time to avoid busy-waiting
            // usleep(1000); // Sleep for 1 millisecond
        }
    }
    catch (TerminationException error)
    {
        return error; // Handle server shutdown
    }
    return 0;
}
// Set the name of the chatroom
void ChatRoom::setChatroomName(const string &newName)
{
    name = newName;
}

// Add a client to the chatroom
void ChatRoom::addClient(const string &name, Socket *socket)
{
    clients.emplace_back(name, socket); // Pass Sync::Socket pointer
    string message = "Server;" + name + " has joined the chatroom.";
    broadcastMessage(message, name);
}

// Remove a client from the chatroom
void ChatRoom::removeClientByName(const string &clientName)
{
    auto it = clients.begin();
    while (it != clients.end())
    {

        if (it->name == clientName)
        {

            it = clients.erase(it); // Erase element and update iterator
            string message = "Server;" + clientName + " has left the chatroom.";
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

// Get the names of all clients in the chatroom
vector<string> ChatRoom::getClientNames() const
{
    vector<string> names;
    for (const auto &client : clients)
    {
        names.push_back(client.name);
    }
    return names;
}

// Get the number of clients in the chatroom
size_t ChatRoom::getClientCount() const
{
    return clients.size();
}

// Add message to a specific client's message list
void ChatRoom::addToClientMessages(const string &clientName, const string &message)
{
    auto it = find_if(clients.begin(), clients.end(), [&clientName](const ClientInfo &info)
                      { return info.name == clientName; });
    if (it != clients.end())
    {
        it->messages.push_back(message);
    }
}

// Get the name of the chatroom
string ChatRoom::getRoomName() const
{
    return name;
}
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
