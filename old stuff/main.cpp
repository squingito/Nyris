#include "SslSock.h"

void die(const char *msg) {
    printf("error in %s", msg);
    exit(-1);

}

void do_stdin_read(std::string* in)
{
    do {
        char buf[2048];
        ssize_t n = read(STDIN_FILENO, buf, sizeof(buf) - 1);
        if (n > 0) buf[n] = '\0';
        if (n>0)
            in->append(buf);
        struct pollfd fdset;
        fdset.fd = STDIN_FILENO;
        fdset.events = POLLIN;
        if (n < 0 || poll(&fdset, 1, 0) != 1) break;
    } while (1);
}

int main() {
    char str[INET_ADDRSTRLEN];
    int port = 55555;

    int servfd = socket(AF_INET, SOCK_STREAM, 0);
    if (servfd < 0)
    die("socket()");

    int enable = 1;
    if (setsockopt(servfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0)
    die("setsockopt(SO_REUSEADDR)");

    /* Specify socket address */
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind(servfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    die("bind()");

    if (listen(servfd, 128) < 0)
    die("listen()");

    int clientfd;
    struct sockaddr_in peeraddr;
    socklen_t peeraddr_len = sizeof(peeraddr);

    struct pollfd fdset[2];
    memset(&fdset, 0, sizeof(fdset));

    fdset[0].fd = STDIN_FILENO;
    fdset[0].events = POLLIN;

    SslSock::sslInit("server.crt", "server.key");

    while (1) {
        printf("waiting for next connection on port %d\n", port);

        clientfd = accept(servfd, (struct sockaddr *)&peeraddr, &peeraddr_len);
        if (clientfd < 0) die("accept()");

        SslSock* sock = new SslSock(clientfd, SERVER_SOCK);

        inet_ntop(peeraddr.sin_family, &peeraddr.sin_addr, str, INET_ADDRSTRLEN);
        printf("new connection from %s:%d\n", str, ntohs(peeraddr.sin_port));

        fdset[1].fd = clientfd;

        /* event loop */

        fdset[1].events = POLLERR | POLLHUP | POLLNVAL | POLLIN;
    #ifdef POLLRDHUP
        fdset[1].events |= POLLRDHUP;
    #endif

        while (1) {
            fdset[1].events &= ~POLLOUT;
            fdset[1].events |= (sock->wantWrite() ? POLLOUT : 0);


            int nready = poll(&fdset[0], 2, -1);

            if (nready == 0)
                continue; /* no fd ready */

            int revents = fdset[1].revents;
            if (revents & POLLIN) {
                
                int64_t val = sock->readHandler();

                if (val < 0 && val != SSL_HANDSHAKE_PROGRESS) {
                
                break;
                }
            }
            if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
                break;
            }
        #ifdef POLLRDHUP
            if (revents & POLLRDHUP)
                break;
        #endif
            if (fdset[0].revents & POLLIN) {
                std::string toPass;
                do_stdin_read(&toPass);
                sock->encryptWrite(&toPass);
            }
            if (revents & POLLOUT) sock->flushWrites();
        }

        close(fdset[1].fd);
        delete sock;
    }
    

}