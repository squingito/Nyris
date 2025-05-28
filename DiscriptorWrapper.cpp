
#include "DiscriptorWrapper.h"


DiscriptorWrap::DiscriptorWrap(int64_t fd, sockaddr_in addr, processor* proc) {
    init(fd, addr, proc);
}

DiscriptorWrap::~DiscriptorWrap() {
    if (secLayer != nullptr) {
        printf("deleting sc layer00000000000000000000000");
        secLayer->delSelf();
        delete secLayer;
    }
    if (proc != nullptr) delete proc;
}

void DiscriptorWrap::init(int64_t fd, sockaddr_in addr, processor* proc) {
    this->fd = fd;
    this->address = addr;
    this->proc = proc;

    flag.wantClose = 0;
    flag.waitingForRead = 0;
    flag.waitingForWrite = 0;
    flag.writeInit = 1;
    secLayer = nullptr;
    
}

flags* DiscriptorWrap::getFlags() {
    return &flag;
}

bool DiscriptorWrap::dataToWrite() {
    return toSend.size() != 0;
}


int64_t DiscriptorWrap::dread(std::vector<char>* out) {
    char readdd[STD_BUFFER_SIZE];
    std::vector<char> buf;
    int64_t numRead = read(fd, readdd, STD_BUFFER_SIZE);

    if (numRead == 0) {
        return -1;
    } else {
        buf.insert(buf.begin(), readdd, readdd + numRead);
    }

    if (secLayer != nullptr) {
        numRead = secLayer->dread(&buf, out, &toSend);
        if (numRead == -1) numRead = 0;
    } else {
        out->clear();
        out->insert(out->end(), buf.data(), buf.data() + numRead);
    }
    
    return numRead;
}

int64_t DiscriptorWrap::dwrite(std::vector<char>* in) {
    //std::lock_guard<std::mutex> lock(this->writeLock);
    if (secLayer != nullptr) {
        secLayer->dwrite(in, &toSend);
    } else {
        toSend.insert(toSend.end(), in->begin(), in->begin() + in->size());
    }
    return 0;
}

int64_t DiscriptorWrap::flushWrites() {

    if (toSend.size() == 0) return 0;
    int64_t a = write(fd, &(toSend)[0], toSend.size());
    toSend.clear();
    
    return a;
}


int64_t DiscriptorWrap::addLayer(ProtocolLayer* in) {
    if (secLayer != nullptr) delete secLayer;
    secLayer = in;
    return 0;
}
