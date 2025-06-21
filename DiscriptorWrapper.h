#ifndef fuckDef
#define fuckDef


#include "ManagedThreadPool.h"
#include <unordered_map>
#include <cstdint>
#include <mutex>
#include <vector>
#include <string>

#include <arpa/inet.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <iostream>

#define STD_BUFFER_SIZE 64


struct flags {
    int64_t wantClose:1;
    int64_t waitingForRead:1;
    int64_t waitingForWrite:1;
    int64_t writeInit:1;
    int64_t writeReady:1;
    std::mutex flagLock;
};

struct processor {
    int64_t (*handlerFunction)(std::string*, void*);
    void* stateStore;
};

class DiscriptorWrap;

class ProtocolLayer {
    public:
        virtual int64_t dread(std::vector<char>*, std::vector<char>*,std::vector<char>*) = 0;
        virtual int64_t dwrite(std::vector<char>*, std::vector<char>*) = 0;
        virtual int64_t dhandshake(std::vector<char>*) = 0;
        virtual void serverHandler(DiscriptorWrap*) = 0;
        virtual int64_t delSelf() {return 0;};
        
        //virtual int64_t writeReady();
        //virtual std::string print() = 0;
        
};



class DiscriptorWrap {

    public:
        DiscriptorWrap();
        DiscriptorWrap(int64_t fd, sockaddr_in addr, processor* proc);
        virtual ~DiscriptorWrap();
        void init(int64_t fd, sockaddr_in addr, processor* proc);

        int64_t dread(std::vector<char>*);
        //int64_t write(std::string*);
        int64_t dwrite(std::vector<char>*);
        bool dataToWrite();
        int64_t getFD();
        //bool writeReady();
        int64_t flushWrites();
        sockaddr_in* getAddress();
        

        ProtocolLayer* getProtocolLayer();
        int64_t addLayer(ProtocolLayer*);

        std::string print();
        flags* getFlags();
        int64_t bindName(std::string);

    

    protected:
        int64_t fd;
        std::vector<char> toRead;
        std::vector<char> toSend;
        sockaddr_in address;
        std::string bindingName;


        ProtocolLayer* secLayer;
        flags flag;

        processor* proc;
        std::mutex writeLock;
        std::mutex readLock;

        

    
};

#endif