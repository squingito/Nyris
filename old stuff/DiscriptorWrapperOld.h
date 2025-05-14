#include <vector>
#include <string>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>

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



class DiscriptorLayer {
    public:
        virtual int64_t read(std::string*) = 0;
        virtual int64_t write(std::string*) = 0;
        virtual std::vector<char>* getSendBuffer() = 0;
        virtual DiscriptorLayer* getNext() = 0;
        virtual int64_t flushWrites() = 0;
        virtual int64_t getFD() = 0;
        virtual sockaddr_in* getAddress() = 0;
        virtual std::string print() = 0;
        
};

class DiscriptorCap: public DiscriptorLayer {
    public:
        virtual int64_t read(std::string*) = 0;
        virtual int64_t write(std::string*) = 0;
        virtual std::vector<char>* getSendBuffer() = 0;
        DiscriptorLayer* getNext();
        virtual int64_t flushWrites() = 0;
        virtual int64_t getFD() = 0;
        virtual sockaddr_in* getAddress() = 0;
        virtual std::string print() = 0;
};

class SocketLayer: public DiscriptorCap {
    public:
        int64_t read(std::string*);
        int64_t write(std::string*);
        std::vector<char>* getSendBuffer();
        int64_t getFD();
        sockaddr_in* getAddress();
        std::string print();

    private:
        int64_t fd;
        std::vector<char> toSend;
        sockaddr_in* address;
};

/*
class SSLLayer: public DiscriptorLayer {
    public:
        int64_t read(std::string*);
        int64_t write(std::string*);
        int64_t getFD();
        sockaddr_in* getAddress();
        std::string print();

    private:
        //sslMode mode;
        DiscriptorLayer nextLayer;

        SSL* ssl;
        BIO *readBio;
        BIO *writeBio;
        std::vector<char> toEncrypt;

};
*/

class DiscriptorWrap {

    public:
        DiscriptorWrap(DiscriptorLayer*)
        int64_t read(std::string*);
        int64_t write(std::string*);
        int64_t getFD();
        bool writeReady();
        int64_t flushWrites();
        DiscriptorLayer* getNext();
        sockaddr_in* getAddress();
        std::string print();
        int64_t getFlags() = 0;
        void setFlags(int64_t);
    

    private:
        DiscriptorLayer* discriptor;
        std::vector<char>* toSend;
        Flags flag;
        int64_t deapth;
        processor* processor;
        std::mutex writeLock;
        std::mutex flagLock;
        DiscriptorCap* bottom;
        

    
};