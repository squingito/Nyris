#include "SecureConnectionManager.h"

ServerWrap::ServerWrap(int64_t fd, sockaddr_in addr, processor* proc, void (*run)(Request*)) {
    this->init(fd, addr, proc);
    this->run = run;
    
}

TempRequest ServerWrap::getRunner() {
    return run;
}

int64_t ServerWrap::setRunner(TempRequest in) {
    run = in;
}

DiscriptorWrap* ServerWrap::serverHandler() {
    int64_t fdHold = 0;
    sockaddr_in valuesHold;
    socklen_t len = sizeof(valuesHold);
    fdHold = accept(fd, (sockaddr*) &(valuesHold), &len);
                        
    if (fdHold == -1) return nullptr;
    DiscriptorWrap* wrap = new DiscriptorWrap(fdHold, valuesHold, nullptr);
    if (this->secLayer != nullptr) {
        secLayer->serverHandler(wrap);
    }
    
    //wrap->addLayer(new SslProtocolLayer(SERVER_SOCK));


    return wrap;
}
 



void SecureConnectionManagerRunner(Request* in) {
    if (in->str.size() > 0) {
        std::vector<char> toWrite;
        toWrite.insert(toWrite.end(), in->str.data(), in->str.data() + in->str.size());
        printf("Message of %d bytes: %.*s", toWrite.size(), toWrite.size(), (toWrite.data()));
        
        in->sock->dwrite(&toWrite);
        //in->sock->flushWrites();
    }
    in->parent->wakeUpCall();
    delete in;
    
}

SecureConnectionManager::SecureConnectionManager(int64_t port) {
    init();
    initServer(port, nullptr, new SslProtocolLayer(HEADER_SOCK));
    initServer(port + 1, nullptr, nullptr);
}

int64_t SecureConnectionManager::initServer(int64_t port, void (*run)(Request*), SslProtocolLayer* in) {
    char str[INET_ADDRSTRLEN];

    int64_t listeningSocketCurrent= -1;
    listeningSocketCurrent = socket(AF_INET, SOCK_STREAM, 0);
    if (listeningSocketCurrent < 0) return -1;

    int enable = 1;
    if (setsockopt(listeningSocketCurrent, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) return -2;

    memset(&lSockAddr, 0, sizeof(lSockAddr));
    lSockAddr.sin_family = AF_INET;
    lSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    lSockAddr.sin_port = htons(port);

    if (bind(listeningSocketCurrent, (struct sockaddr *) &lSockAddr, sizeof(lSockAddr)) < 0) return -3;
    

    //pollfd ls;
    //ls.fd = listeningSocketCurrent;
    //ls.events = POLLIN;
    //polls.push_back(ls);
    ServerWrap* srap = new ServerWrap(listeningSocketCurrent, lSockAddr, nullptr, nullptr);
    if (in != nullptr) {
        srap->addLayer(in);
    }
    //map[ls.fd] = srap;

    if (listen(listeningSocketCurrent, 128) < 0) return -4;
    insert(listeningSocketCurrent, srap);
    //listeningSocks.push_back()
    return 0;

}

int64_t SecureConnectionManager::init() {
    queue = new ConcurrentPrioQueue<Request>();

    pool = new ManagedThreadPool<Request,void>(SecureConnectionManagerRunner, nullptr,  queue, 4);
    pipe(pipefds);
    pollfd pipeRead;
    pipeRead.fd = pipefds[0];
    pipeRead.events = POLLIN;
    polls.push_back(pipeRead);
    return 0;
}

int64_t SecureConnectionManager::insert(int64_t fd, DiscriptorWrap* wrap) {
    DiscriptorWrap* grabbed = map[fd];
    if (grabbed != nullptr || wrap == nullptr) {
        return -1;
    }
    {
        std::lock_guard<std::mutex> lock(insertMutex);
        insets.push_back(wrap);
        
    }
    return 0;

}

int64_t SecureConnectionManager::wakeUpCall() {
    std::lock_guard<std::mutex> lock(pipeMutex);
    char input[2];
    input[0] = 'a';
    input[1] = '\0';
    return write(pipefds[1], input, 1);
}

int64_t SecureConnectionManager::bindName(int64_t in, std::string binding) {
    std::lock_guard lock(bindingLock);
    DiscriptorWrap* bindi = map[in];
    if (bindingMap[binding] != nullptr) return -1;
    if (bindi == nullptr) return -1;
    bindingMap[binding] = bindi;
    
    bindi->bindName(binding);
    return 0;
}


int64_t SecureConnectionManager::serverRunner() {
    printf("runna\n");
    fflush(stdout);
    SslProtocolLayer::sslInit("server.crt", "server.key");
    int num = 0;
    printf("pokemonGoin");
    
    while (1) {

        //check for inserts and handel them 

        {
            std::lock_guard<std::mutex> lock(insertMutex);

            for (int i = 0; i < insets.size(); i++) {
                pollfd ls;
                ls.fd = insets[i]->getFD();
                if (dynamic_cast<ServerWrap*>(insets[i])) {
                    ls.events = POLLIN;
                } else {
                    ls.events = POLLIN & POLLOUT;
                }
                
                polls.push_back(ls);

                map[ls.fd] = insets[i];
            }
            insets.clear();
        }
        

        int64_t nready = poll(&(polls[0]), polls.size(), 50);
        if (nready == 0) continue;
        printf("pokemonGoin %d\n", num);

        if (polls.size() >= 2) {
            if (polls[0].revents & POLLIN) {
                char buf[2048];
                int64_t a = read(polls[0].fd, buf, 2048);
                polls[0].revents = 0;
            }
        }
        
        std::vector<pollfd> newGuys;
        std::vector<int64_t> oldGuys;
        std::vector<Request> reqs;
        num = num + 1;
        for (int i = 0; i < polls.size(); i++) {
            pollfd* toUse = &polls[i];
            DiscriptorWrap* wrap = map[toUse->fd];
            ServerWrap* serverWrap = dynamic_cast<ServerWrap*>(wrap);
            if (toUse->fd == pipefds[0]) continue;
            if (serverWrap != nullptr) {
                if (toUse->revents & POLLIN) {

                    while (1) {
                    

                        DiscriptorWrap* out = serverWrap->serverHandler();

                        if (out == nullptr) break;
                        map[out->getFD()] = out;
                        pollfd newGuy;
                        newGuy.fd = out->getFD();
                        newGuy.events = POLLIN | POLLERR | POLLHUP | POLLOUT;
                        //newGuy.revents = newGuy.revents | POLLOUT;

                        newGuys.push_back(newGuy);
                        break;

                    }
                    
                }
            } else  {
                
                if (toUse->revents & POLLIN) {

                    std::vector<char> read;
                    int num = wrap->dread(&read);

                    //wrap->flushWrites();
                    //printf("num read %d", num);
                    if (num > 0) {
                        Request* req = new Request;
                            req->parent = this;
                            req->sock = wrap;
                            req->hold = 1;
                            req->prio = 1;
                            req->str = read;
                            queue->push(req);

                    } else if (num == -1) {
                        oldGuys.push_back(i);
                    }
                    
                }
                if (toUse->revents & POLLOUT || wrap->getFlags()->writeReady) {
                    toUse->events = toUse->events & ~POLLOUT;
                    wrap->getFlags()->writeReady = 1;
                    if (wrap->dataToWrite()) {
                        wrap->flushWrites();
                        toUse->events = toUse->events | POLLOUT;
                        wrap->getFlags()->writeReady = 0;
      
                    }
                }
                toUse->revents = 0;
            } 
            
        }
        for (int i = oldGuys.size() - 1; i >= 0; i--) {
            close(polls[oldGuys[i]].fd);
            DiscriptorWrap* hold = map[polls[oldGuys[i]].fd];
            map.erase(polls[oldGuys[i]].fd);
            delete hold;

            if (oldGuys[i] == polls.size() - 1) {
                
                polls.erase(polls.end() - 1);
            } else {
                polls[oldGuys[i]] = polls[polls.size() - 1];
                polls.erase(polls.end() - 1);

            }
        }
        for (int i = 0; i < newGuys.size(); i++) {
            polls.push_back(newGuys[i]);
        }
    }
}

/*
int64_t SecureConnectionManager::serverRunner() {
    printf("runna\n");
    fflush(stdout);
    SslProtocolLayer::sslInit("server.crt", "server.key");
    int num = 0;
    printf("pokemonGoin");
    while (1) {
        printf("pokemonGoin %d\n", num);
        num = num + 1;

            if (polls.size() > 2) printf("%d   %d", map[polls[2].fd]->getFlags()->writeReady, map[polls[2].fd]->getFlags()->waitingForWrite);
            //sleep(1000);
        
        int64_t nready = poll(&(polls[0]), polls.size(), -1);
        std::vector<pollfd> newGuys;
        std::vector<int64_t> oldGuys;
        std::vector<Request> reqs;
        for (int i = 0; i < polls.size(); i++) {
            pollfd* toUse = &polls[i];
            if (toUse->fd == listeningSocket) {
                if (toUse->revents & POLLIN) {
                    int64_t fdHold = 0;
                    sockaddr_in valuesHold;
                    socklen_t len = sizeof(valuesHold);
                    while (fdHold != -1) {
                    
                        fdHold = accept(listeningSocket, (sockaddr*) &(valuesHold), &len);
                        
                        if (fdHold == -1) break;
                        DiscriptorWrap* wrap = new DiscriptorWrap(fdHold, valuesHold, nullptr);
                        wrap->addLayer(new SslProtocolLayer(SERVER_SOCK));
    

                        map[fdHold] = wrap;
                        pollfd newGuy;
                        newGuy.fd = fdHold;
                        newGuy.events = POLLIN | POLLERR | POLLHUP | POLLOUT;
                        //newGuy.revents = newGuy.revents | POLLOUT;

                        newGuys.push_back(newGuy);
                        break;

                    }
                    
                }
            } else if (toUse->fd == pipefds[0]) {
                if (toUse->revents & POLLIN) {
                    char buf[STD_BUFFER_SIZE];
                    read(toUse->fd, buf, STD_BUFFER_SIZE);
                    toUse->revents = 0;
                }
            } else {
                DiscriptorWrap* wrap = map[toUse->fd];
                { //block thing
                    flags* flag = wrap->getFlags();
                    std::lock_guard<std::mutex> lock(flag->flagLock);
                    if (flag->wantClose) {
                        oldGuys.push_back(i);
                        toUse->revents = 0;
                    }
                    


                    if (toUse->revents & POLLERR || toUse->revents & POLLHUP) {
                        // error case and close case
                        toUse->revents = 0;
                        oldGuys.push_back(i);
                        sleep(10000);
                        
                    }
                    
                    if (toUse->revents & POLLIN) {
                        /*
                            Request* req = new Request;
                            req->parent = this;
                            flag->waitingForRead = true;
                            req->sock = wrap;
                            req->hold = 1;
                            req->prio = 1;
                            toUse->revents = toUse->revents & ~POLLIN;
                            toUse->events = ~POLLIN & toUse->events;
                                   printf("dsafdkjsaflkdjsaklfjlasdk");
        fflush(stdout);
                            queue->push(req);
                                   printf("dsafdkjsaflkdjsaklfjlasdk");
        fflush(stdout);
                        


std::vector<char> read;
int num = wrap->dread(&read);
printf("num read %d", num);
if (num > 0) {
    std::vector<char> toWrite;
    toWrite.insert(toWrite.end(), read.data(), read.data() + read.size());
    
    if (num > 0) {
    wrap->dwrite(&toWrite);Request* req = new Request;
                            req->parent = this;
                            flag->waitingForWrite = true;
                            req->sock = wrap;
                            req->hold = 2;
                            req->prio = 1;
                            toUse->revents = toUse->revents & ~POLLOUT;
    //in->sock->flushWrites();
    printf("%s", &(read[0]));
    fflush(stdout);
    } 
}
flags* flag = wrap->getFlags();
//printf("dsafdkjsaflkdjsaklfjlasdk");
fflush(stdout);
std::lock_guard<std::mutex> lock(flag->flagLock);
//if (num == -1) flag->wantClose = 1;
flag->waitingForRead = false;
                        

                    }
                    if (toUse->revents & POLLOUT || flag->writeReady) {
                        toUse->events = ~POLLOUT & toUse->events;
                        flag->writeReady = 1;
                        

                        if (wrap->dataToWrite()) {
                            // do write
                            Request* req = new Request;
                            req->parent = this;
                            flag->waitingForWrite = true;
                            req->sock = wrap;
                            req->hold = 2;
                            req->prio = 1;
                            toUse->revents = toUse->revents & ~POLLOUT;
                            
                            printf("dsafdkjsaflkdjsaklfjlasdk");
        fflush(stdout);
                            queue->push(req);
                                   printf("d999999999999safdkjsaflkdjsaklfjlasdk");
        fflush(stdout);
                        }
                    }

                    if (!flag->waitingForRead) {
                        toUse->events = toUse->events | POLLIN;
                    }
                    if (!flag->waitingForWrite && !(flag->writeReady)) {
                        toUse->events = toUse->events | POLLOUT;
                        printf("herheeheh");
                    }
                }
                
            }
        }
        
        for (int i = oldGuys.size() - 1; i >= 0; i--) {
            close(polls[oldGuys[i]].fd);
            DiscriptorWrap* hold = map[polls[oldGuys[i]].fd];
            map.erase(polls[oldGuys[i]].fd);
            delete hold;
            if (oldGuys[i] == polls.size() - 1) {
                
                polls.erase(polls.end() - 1);
            } else {
                polls[oldGuys[i]] = polls[polls.size() - 1];
                polls.erase(polls.end() - 1);

            }
        }
        for (int i = 0; i < newGuys.size(); i++) {
            polls.push_back(newGuys[i]);
        }
    }
    return 0;
}

*/