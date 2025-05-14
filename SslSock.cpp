
#include "SslSock.h"



SSL_CTX* SslSock::ctx = nullptr;

int64_t SslSock::sslInit(char* cert, char* key) {
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ERR_load_crypto_strings();
    ctx = SSL_CTX_new(TLS_method());
    if (!ctx) return ERROR_CREATING_CONTEXT;
    if (cert != nullptr && key != nullptr) {
        if (SSL_CTX_use_certificate_file(ctx, cert,  SSL_FILETYPE_PEM) != 1) return ERROR_ENTERING_CERTIFICATE;
        if (SSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM) != 1) return ERROR_ENTERING_PK;
        if (SSL_CTX_check_private_key(ctx) != 1) return ERROR_CHECKING_PK;
    
        #ifdef SSL_SOCK_DEBUG
            printf("DEBUG: SSL cert and key files are good to go\n");
        #endif
    }

    SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(ctx, TLS1_3_VERSION);
    SSL_CTX_set_options(ctx,  SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3);
    return 0;
}

SslSock::SslSock(int64_t fd, sslMode mode) {
    init(fd, mode);
    toSend.clear();
    toEncrypt.clear();

}

SslSock::~SslSock() {
    SSL_free(ssl);

}


bool SslSock::wantWrite() {
    return toSend.size() > 0;
}

int64_t SslSock::sockRead(std::vector<char>* input) {
    int64_t numRead;
    char buffer[STD_BUFFER_SIZE];
    numRead = read(sockfd, buffer, STD_BUFFER_SIZE );

    #ifdef SSL_SOCK_DEBUG
        printf("DEBUG: read returns %d\n", numRead);
    #endif

    if (numRead > 0) {
        input->insert(input->begin(), buffer, buffer + numRead);
    }
    return input->size();
}

int64_t SslSock::flushWrites() {
    if (toSend.size() > 0) {
        int64_t numWrite;

        #ifdef SSL_SOCK_DEBUG
            printf("DEBUG: flushing %d bytes from the write buffer\n", toSend.size());
        #endif

        do {
            const char* toSendC = &toSend[0];
            numWrite = write(sockfd, toSendC, toSend.size());

            if (numWrite > 0) {
                if (numWrite < toSend.size()) toSend.erase(toSend.begin(), toSend.begin() + numWrite);
                if (numWrite == toSend.size()) toSend.clear();
            }
        } while (numWrite > 0);
    }
    return 0;
}

int64_t SslSock::readHandler() {
    return readHandler(nullptr);
}

int64_t SslSock::readHandler(std::string* input) {
    std::vector<char> readBytes;
    int64_t toProcess = sockRead(&readBytes);
    
    int64_t status, numRead;

    //if (toProcess > 0) {
        const char* cStr = &readBytes[0];
       
        do {
            numRead = BIO_write(readBio, cStr, toProcess);

            #ifdef SSL_SOCK_DEBUG
                printf("DEBUG: Read handler wrote %d bytes to the read bio\n", numRead);
            #endif

            if (numRead <= 0) return -1;
            cStr += numRead;
            toProcess -= numRead;

            if (!SSL_is_init_finished(ssl)) {
                status = sslHandshake();
                if (status == SSL_HANDSHAKE_FAILURE) return SSL_HANDSHAKE_FAILURE;
                if (status == SSL_HANDSHAKE_PROGRESS) return SSL_HANDSHAKE_PROGRESS;
            }

            std::vector<char> decryptedText;
            int64_t numDecrypted = decrypt(&decryptedText);
            if (decryptedText.size() > 0) {
                if (input != nullptr) input->append(&decryptedText[0], decryptedText.size());
                if (input == nullptr || flags.readMode == 1)  printf("Message of %d bytes: %.*s", decryptedText.size(), decryptedText.size(), &decryptedText[0]);
            }
            
            int64_t sslStatus = SSL_get_error(ssl, numDecrypted);


            /*
            if (sslStatus == SSL_ERROR_WANT_WRITE) {

                #ifdef SSL_SOCK_DEBUG
                    printf("DEBUG: SSL want reneg\n");
                #endif

                int64_t check = sslDirect();
                if (check == SSL_FAILURE) return SSL_FAILURE;
            } else if (sslStatus != SSL_ERROR_NONE && sslStatus != SSL_ERROR_WANT_READ) return SSL_FAILURE;
            */
        } while (toProcess > 0);
        
        return SSL_OK;
        /*
    } else {
        if (toProcess == 0) return SOCKET_PEER_CLOSED;
        if (toProcess < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return SOCKET_NO_DATA;
            }
        }
        return SSL_FAILURE;
    }
        */

}

int64_t SslSock::decrypt(std::vector<char>* in) {

    #ifdef SSL_SOCK_DEBUG
        printf("DEBUG: Processing Data from socket\n");
    #endif

    char buffer[STD_BUFFER_SIZE];
    int64_t numDecrypted;
    do {
        numDecrypted = SSL_read(ssl, buffer, STD_BUFFER_SIZE);
        if (numDecrypted > 0) {
            in->insert(in->end(), buffer, buffer + numDecrypted);
            
        }
    } while (numDecrypted > 0);
    return numDecrypted;
}

int64_t SslSock::sslHandshake() {
    int64_t sslStatus = SSL_do_handshake(ssl);
    int64_t sslError = SSL_get_error(ssl, sslStatus);

    #ifdef SSL_SOCK_DEBUG
        printf("DEBUG: Handshaking\n");
        if (sslError == SSL_ERROR_WANT_READ) printf("DEBUG: Handshake wants to read\n");
        if (sslError == SSL_ERROR_WANT_WRITE) printf("DEBUG: Handshake wants to write\n");
    #endif

    int64_t numRead;
     
    if (sslError == SSL_ERROR_WANT_WRITE || sslError == SSL_ERROR_WANT_READ) {
        int64_t check = sslDirect();
    } else if (sslError != SSL_ERROR_NONE) {
        return SSL_HANDSHAKE_FAILURE;
    } 
    if (!SSL_is_init_finished(ssl)) {
        return SSL_HANDSHAKE_PROGRESS;
    }
    sslDirect();

    #ifdef SSL_SOCK_DEBUG
        printf("DEBUG: Handshake complete\n");
    #endif
    
    return SSL_OK;
}

/*
    Gives the ssl write bio direct access to the socket in order to do things like handshakes
*/
int64_t SslSock::sslDirect() {

    #ifdef SSL_SOCK_DEBUG
        printf("DEBUG: Allowing SSL write bio to write to socket\n");
    #endif

    char buffer[STD_BUFFER_SIZE];
    int64_t numRead;
    do {
        numRead = BIO_read(writeBio, buffer, STD_BUFFER_SIZE);
        if (numRead > 0) {
            toSend.insert(toSend.end(), buffer, buffer + numRead);
            
        } else if (!BIO_should_retry(writeBio)) {
            return SSL_FAILURE;
        }
    } while (numRead > 0);
    return SSL_OK;
}

bool SslSock::init(int64_t sock, sslMode mode) {
    readBio = BIO_new(BIO_s_mem());
    writeBio = BIO_new(BIO_s_mem());
    sockfd = sock;
    ssl = SSL_new(ctx);

    if (mode == SERVER_SOCK) {
        SSL_set_accept_state(ssl); // server mode
    } if (mode == CLIENT_SOCK) {
        SSL_set_connect_state(ssl); //client
    }

    SSL_set_bio(ssl, readBio, writeBio);
    return true;
}

int64_t SslSock::encryptWrite(std::string* input) {

    if (!SSL_is_init_finished(ssl)) return SSL_HANDSHAKE_NOT_FINISHED;
    toEncrypt.insert(toEncrypt.end(), input->c_str(), input->c_str() + input->size());
    #ifdef SSL_SOCK_DEBUG
        printf("DEBUG: Encrypting data to send\n");
    #endif

    int64_t sslStatus;
    while (toEncrypt.size() > 0) {
        const char* toEncryptC = &toEncrypt[0];
        int64_t writeNum = SSL_write(ssl, toEncryptC, toEncrypt.size());
        sslStatus = SSL_get_error(ssl, writeNum);
        
        if (sslStatus == SSL_FAILURE) return SSL_FAILURE;
        
        bool test;
        if (writeNum > 0) {
            toEncrypt.erase(toEncrypt.begin(), toEncrypt.begin() + writeNum);

        } else if (test = (writeNum == toEncrypt.size())) {
            toEncrypt.clear();
        } 
        sslDirect();

        
    }
    return SSL_OK;

}


int64_t SslSock::checkHandshake() {
    return SSL_is_init_finished(ssl);
}