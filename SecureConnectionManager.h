#ifndef SECURE_CONNECTION_MANAGER_H
#define SECURE_CONNECTION_MANAGER_H

#include "ManagedThreadPool.h"
#include <unordered_map>
#include "DiscriptorWrapper.h"
#include "SslProtocolLayer.h"

struct Request;

using TempRequest = void (*)(Request*);

class SecureConnectionManager;

class ServerWrap: public DiscriptorWrap {
private:
    void (*run)(Request*);

public:
    ServerWrap(int64_t fd, sockaddr_in addr, processor* proc, void (*run)(Request*));
    TempRequest getRunner();
    int64_t setRunner(void (*out)(Request*));
    DiscriptorWrap* serverHandler();
};


struct Request {
    int64_t hold;
    DiscriptorWrap* sock;
    int64_t prio;
    SecureConnectionManager* parent;
    std::vector<char> str;

    bool operator>(const Request& two) const {
        return prio > two.prio;
    } 
};

void SecureConnectionManagerRunner(Request*);
/*
struct ServerTools {
    SecureConnectionManager* manager;
    DiscriptorWrap* sender;
    std::string recieved;
};
*/




class SecureConnectionManager {
    public:
        SecureConnectionManager(int64_t);
        int64_t init();
        int64_t serverRunner();
        int64_t wakeUpCall();
        int64_t insert(int64_t, DiscriptorWrap*);
        //int64_t insertServer(int64_t, ServerWrap*);
        int64_t initServer(int64_t port, void (*run)(Request*), SslProtocolLayer*);
        int64_t bindName(int64_t, std::string);

    private:
        
        int64_t listeningSocket;
        sockaddr_in lSockAddr;

        
        std::vector<int64_t*> sockets;
        ConcurrentPrioQueue<Request>* queue;
        ManagedThreadPool<Request,void>* pool;
        std::vector<pollfd> polls;
        std::unordered_map<int64_t, DiscriptorWrap*> map;
        std::unordered_map<std::string, DiscriptorWrap*> bindingMap;

        std::mutex bindingLock;

        std::vector<DiscriptorWrap*> insets;

        std::mutex insertMutex;

        std::mutex pipeMutex;
        int32_t pipefds[2];

};










#endif