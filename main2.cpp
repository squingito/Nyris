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
    int port = 55555;
    const char* host_ip = "127.0.0.1";
    const char * host_name = NULL;
    int ip_family = AF_INET;

    int sockfd = socket(ip_family, SOCK_STREAM, 0);

  if (sockfd < 0)
    die("socket()");

  /* Specify socket address */

  if (ip_family == AF_INET6) {
    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = ip_family;
    addr.sin6_port = htons(port);

    if (inet_pton(ip_family, host_ip, &(addr.sin6_addr)) <= 0)
      die("inet_pton()");

    if (connect(sockfd, (struct sockaddr*) &addr, sizeof(addr)) < 0)
      die("connect()");
  }

  if (ip_family == AF_INET) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = ip_family;
    addr.sin_port = htons(port);

    if (inet_pton(ip_family, host_ip, &(addr.sin_addr)) <= 0)
      die("inet_pton()");

    if (connect(sockfd, (struct sockaddr*) &addr, sizeof(addr)) < 0)
      die("connect()");
  }

  printf("socket connected\n");

  
  struct pollfd fdset[2];
  memset(&fdset, 0, sizeof(fdset));

  fdset[0].fd = STDIN_FILENO;
  fdset[0].events = POLLIN;

  SslSock::sslInit(nullptr, nullptr);
  SslSock* sock = new SslSock(sockfd, CLIENT_SOCK);

  if (host_name)
    SSL_set_tlsext_host_name(sock->ssl, host_name); // TLS SNI

  fdset[1].fd = sockfd;
  fdset[1].events = POLLERR | POLLHUP | POLLNVAL | POLLIN;
#ifdef POLLRDHUP
  fdset[1].events |= POLLRDHUP;
#endif

  /* event loop */

  sock->sslHandshake();
  sock->flushWrites();

  while (1) {
    fdset[1].events &= ~POLLOUT;
    fdset[1].events |= sock->wantWrite()? POLLOUT:0;

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
                printf("dfdasfsdae %d\n", (revents & POLLNVAL));
                break;
            }
            if (fdset[0].revents & POLLIN) {
                std::string toPass;
                do_stdin_read(&toPass);
                sock->encryptWrite(&toPass);
            }
            if (revents & POLLOUT) sock->flushWrites();
        }

  close(fdset[1].fd);
  delete sock;

  return 0;
}