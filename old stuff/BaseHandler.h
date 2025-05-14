#ifndef BASEHANDLER_H
#define BASEHANDLER_H

#include "SslSock.h"

#define SSL_NOT_READY -1
#define SOCKET_ERROR -2


class BaseHandler {
    public:
        BaseHandler(int64_t, sslMode);

        //int64_t read(char*);
        int64_t read(std::string*);
        int64_t write(std::string*);
        //int64_t write(char*);
        int64_t init();

        int64_t setTimeout(timeval time);



    private:
        SslSock* sock;
        sslMode mode;
        int64_t sfd;



};

#endif 