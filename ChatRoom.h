#ifndef CHATROOM_H
#define CHATROOM_H

#include <string>
#include <vector>
#include "socket.h"
#include "thread.h"

using namespace Sync;
using namespace std;

class ChatRoom : public Thread
{
private:
    struct ClientInfo
    {
        string name;
        Socket *socket;
        vector<string> messages;

        ClientInfo(const string &n, Socket *s) : name(n), socket(s) {}
    };

    vector<ClientInfo> clients;
    string name;

public:
    bool isActive = true;
    ChatRoom(const string &roomName);

    ~ChatRoom();

    bool existingUser(const string &username);

    void broadcastMessage(const string &message, const string &sender);

    long ThreadMain() override;

    void setChatroomName(const string &newName);

    void addClient(const string &name, Socket *socket);

    void removeClientByName(const string &clientName);

    vector<string> getClientNames() const;

    size_t getClientCount() const;

    void addToClientMessages(const string &clientName, const string &message);

    string getRoomName() const;

    Socket getClientSocketByName(const string &clientName) const;

    string getClientNameBySocket(const Sync::Socket &clientSocket) const;

    vector<string> getMessagesByClientName(const string &clientName) const;
};

#endif // CHATROOM_H
