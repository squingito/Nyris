// SslSock.h
#ifndef SSLSOCK_H
#define SSLSOCK_H


#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <vector>

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

#define SSL_SOCK_DEBUG


#define SSL_OK 0
#define SSL_HANDSHAKE_PROGRESS -1
#define SSL_FAILURE -2
#define SSL_HANDSHAKE_FAILURE -3
#define SSL_HANDSHAKE_NOT_FINISHED -4

#define SOCKET_NO_DATA -11
#define SOCKET_PEER_CLOSED -12

#define ERROR_CREATING_CONTEXT -100
#define ERROR_ENTERING_CERTIFICATE -101
#define ERROR_ENTERING_PK -102
#define ERROR_CHECKING_PK -103




enum sslMode {
    CLIENT_SOCK,
    SERVER_SOCK
};

struct sslSockFlags {
    int64_t writeReady:1;
    int64_t readMode:1;
};





class SslSock {
public:
    
    SslSock(int64_t, sslMode);
    ~SslSock();
    bool wantWrite();

    int64_t sockRead(std::vector<char>*);
    int64_t flushWrites();

    int64_t sslHandshake();
    int64_t sslDirect();
    int64_t decrypt(std::vector<char>*);
    int64_t encryptWrite(std::string*);
    int64_t readHandler();
    int64_t readHandler(std::string*);
    int64_t checkHandshake();
    //void setReadMode(bool);

    static int64_t sslInit(char*, char*);

    SSL* ssl;

private:
    static SSL_CTX* ctx;
    
    int64_t sockfd;
    sslMode mode;
    sslSockFlags flags;
    //int64_t lastStatus;

    BIO *readBio;
    BIO *writeBio;

    std::vector<char> toSend;
    std::vector<char> toEncrypt;

    int64_t (*handlerFunction)(std::string*, void**);
    void* stateStore;

    bool init(int64_t, sslMode);

};


#endif // SSLSOCK_H
