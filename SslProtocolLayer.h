#ifndef guardSsl
#define guardSsl
#include "DiscriptorWrapper.h"
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <vector>








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


class SslProtocolLayer: public ProtocolLayer {

    static SSL_CTX* ctx;
public:

    SslProtocolLayer(sslMode mode);
    ~SslProtocolLayer();

    

    bool init(sslMode) ;
    int64_t sslDirect(std::vector<char>* woutput);

    int64_t dhandshake(std::vector<char>* woutput);
    int64_t decrypt(std::vector<char>* in);
    static int64_t sslInit(char* cert, char* key);
    int64_t dread(std::vector<char>* rinput, std::vector<char>* routput, std::vector<char>* woutput);
    

    int64_t dwrite(std::vector<char>* input, std::vector<char>* output);

private:
    SSL* ssl;
    
    
    sslMode mode;

    BIO *readBio;
    BIO *writeBio;

    

};

#endif