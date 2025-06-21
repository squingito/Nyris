#include "SslProtocolLayer.h"
SSL_CTX* SslProtocolLayer::ctx = nullptr;


SslProtocolLayer::SslProtocolLayer(sslMode mode) {
    init(mode);
    //this->toSend = out;
}



bool SslProtocolLayer::init(sslMode mode) {
    if (mode == HEADER_SOCK) return false;
    printf("[Ssl] created\n");
    this->mode = mode;
    readBio = BIO_new(BIO_s_mem());
    writeBio = BIO_new(BIO_s_mem());
    ssl = SSL_new(ctx);

    if (mode == SERVER_SOCK) {
        SSL_set_accept_state(ssl); // server mode
    } if (mode == CLIENT_SOCK) {
        SSL_set_connect_state(ssl); //client
    }

    SSL_set_bio(ssl, readBio, writeBio);
    return true;
}

void SslProtocolLayer::serverHandler(DiscriptorWrap* in) {
    in->addLayer(new SslProtocolLayer(SERVER_SOCK));
}


int64_t SslProtocolLayer::sslDirect(std::vector<char>* woutput) {

    #ifdef SSL_SOCK_DEBUG
        printf("DEBUG: Allowing SSL write bio to write to socket\n");
    #endif

    char buffer[STD_BUFFER_SIZE];
    int64_t numRead;
    do {
        //printf("herereh");
        numRead = BIO_read(writeBio, buffer, STD_BUFFER_SIZE);
        if (numRead > 0) {
            woutput->insert(woutput->end(), buffer, buffer + numRead);
            
        } else if (!BIO_should_retry(writeBio)) {
            return SSL_FAILURE;
        }
    } while (numRead > 0);
    return SSL_OK;
}

int64_t SslProtocolLayer::dhandshake(std::vector<char>* woutput) {
    int64_t sslStatus = SSL_do_handshake(ssl);
    int64_t sslError = SSL_get_error(ssl, sslStatus);

    #ifdef SSL_SOCK_DEBUG
        printf("DEBUG: Handshaking\n");
        if (sslError == SSL_ERROR_WANT_READ) printf("DEBUG: Handshake wants to read\n");
        if (sslError == SSL_ERROR_WANT_WRITE) printf("DEBUG: Handshake wants to write\n");
    #endif

    int64_t numRead;
     
    if (sslError == SSL_ERROR_WANT_WRITE || sslError == SSL_ERROR_WANT_READ) {
        int64_t check = sslDirect(woutput);
    } else if (sslError != SSL_ERROR_NONE) {
        return SSL_HANDSHAKE_FAILURE;
    } 
    if (!SSL_is_init_finished(ssl)) {
        return SSL_HANDSHAKE_PROGRESS;
    }
    sslDirect(woutput);

    #ifdef SSL_SOCK_DEBUG
        printf("DEBUG: Handshake complete\n");
    #endif
    
    return SSL_OK;
}

int64_t SslProtocolLayer::decrypt(std::vector<char>* in) {

    #ifdef SSL_SOCK_DEBUG
        printf("DEBUG: Processing Data from socket\n");
    #endif

    char buffer[STD_BUFFER_SIZE];
    int64_t numDecrypted;
    int64_t total = 0;
    do {
        numDecrypted = SSL_read(ssl, buffer, STD_BUFFER_SIZE);
        
        if (numDecrypted > 0) {
            in->insert(in->end(), buffer, buffer + numDecrypted);
            total = total + numDecrypted;
            
        }
    } while (numDecrypted > 0);
    return total;
}

int64_t SslProtocolLayer::sslInit(char* cert, char* key) {
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

int64_t SslProtocolLayer::dread(std::vector<char>* rinput, std::vector<char>* routput, std::vector<char>* woutput) {

    int64_t status, numRead;
    int64_t toProcess = rinput->size();
    int64_t numDecrypted = 0;

    //if (toProcess > 0) {
        const char* cStr = rinput->data();
    
        do {
            numRead = BIO_write(readBio, cStr, toProcess);

            #ifdef SSL_SOCK_DEBUG
                printf("DEBUG: Read handler wrote %d bytes to the read bio\n", numRead);
            #endif

            if (numRead <= 0) return -1;
            cStr += numRead;
            toProcess -= numRead;

            if (!SSL_is_init_finished(ssl)) {
                status = dhandshake(woutput);
                if (status == SSL_HANDSHAKE_FAILURE) return SSL_HANDSHAKE_FAILURE;
                if (status == SSL_HANDSHAKE_PROGRESS) return SSL_HANDSHAKE_PROGRESS;
            }

            std::vector<char> decryptedText;
            numDecrypted = decrypt(&decryptedText);
            if (decryptedText.size() > 0) {
                if (routput != nullptr) {
                    routput->insert(routput->end(), decryptedText.begin(), decryptedText.end());
                }
                
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
        
        return numDecrypted;
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

int64_t SslProtocolLayer::dwrite(std::vector<char>* input, std::vector<char>* output) {

    if (!SSL_is_init_finished(ssl)) return SSL_HANDSHAKE_NOT_FINISHED;
    #ifdef SSL_SOCK_DEBUG
        printf("DEBUG: Encrypting data to send\n");
    #endif

    int64_t sslStatus;
    while (input->size() > 0) {
        const char* toEncryptC = input->data();
        int64_t writeNum = SSL_write(ssl, toEncryptC, input->size());
        sslStatus = SSL_get_error(ssl, writeNum);
        
        if (sslStatus == SSL_FAILURE) return SSL_FAILURE;
        
        bool test;
        if (writeNum > 0) {
            input->erase(input->begin(), input->begin() + writeNum);

        } else if (test = (writeNum == input->size())) {
            input->clear();
        } 
        sslDirect(output);

        
    }
    return SSL_OK;

}

int64_t SslProtocolLayer::delSelf()  {
    printf("[Ssl] destory\n");
    if (mode == HEADER_SOCK) return 0;
    if (ssl) {
        SSL_shutdown(ssl); // Optional: graceful shutdown
        SSL_free(ssl);     // Frees the SSL structure and associated BIOs
        ssl = nullptr;
    } else {

    if (readBio) {
        BIO_free(readBio);
        readBio = nullptr;
    }
    if (writeBio) {
        BIO_free(writeBio);
        writeBio = nullptr;
    }
    return 0;
}
return 0;

 
}