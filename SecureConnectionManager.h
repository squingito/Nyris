#ifndef SECURE_CONNECTION_MANAGER_H
#define SECURE_CONNECTION_MANAGER_H

#include "ManagedThreadPool.h"
#include <unordered_map>
#include "DiscriptorWrapper.h"
#include "SslProtocolLayer.h"

class SecureConnectionManager;



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

struct ServerTools {
    SecureConnectionManager* manager;
    DiscriptorWrap* sender;
    std::string recieved;
};


class SecureConnectionManager {
    public:
        SecureConnectionManager(int64_t);
        int64_t init(int64_t port, void (*run)(Request*));
        int64_t serverRunner();
        int64_t wakeUpCall();

    private:
        int64_t listeningSocket;
        sockaddr_in lSockAddr;

        
        std::vector<int64_t*> sockets;
        ConcurrentPrioQueue<Request>* queue;
        ManagedThreadPool<Request,void>* pool;
        std::vector<pollfd> polls;
        std::unordered_map<int64_t, DiscriptorWrap*> map;

        std::mutex pipeMutex;
        int32_t pipefds[2];

};










#endif