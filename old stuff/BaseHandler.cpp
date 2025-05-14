#include "BaseHandler.h"

BaseHandler::BaseHandler(int64_t fd, sslMode inMode) {
    sock = new SslSock(fd, inMode);
    sfd = fd;
    mode = inMode;
}

int64_t BaseHandler::init() {
    if (CLIENT_SOCK == mode) {
    sock->sslHandshake();
    sock->flushWrites();
    }
    while (1) {
        if (sock->wantWrite()) {
            sock->flushWrites();
        }
        if (sock->checkHandshake()) {
            sock->sslDirect();
            sock->flushWrites();
            return 0;
        }

        int64_t ret = sock->readHandler();
    }

    sock->sslDirect();
    sock->flushWrites();
    return 0;
}

int64_t BaseHandler::setTimeout(timeval time) {
    setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, (const char*) &time, sizeof(time));
    return 0;
}

int64_t BaseHandler::write(std::string* in) {
    sock->encryptWrite(in);
    return sock->flushWrites();
}

int64_t BaseHandler::read(std::string* in) {

    
    return sock->readHandler();
    
}