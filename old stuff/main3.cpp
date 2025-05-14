#include "BaseHandler.h"

void die(const char *msg) {
    printf("error in %s", msg);
    exit(-1);

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

  sslInit(nullptr, nullptr);

  BaseHandler* h = new BaseHandler(sockfd, CLIENT_SOCK);
  h->init();
  
  std::string str;
  std::string ht = "hi there";
  h->write(&ht);
  timeval ti;
  ti.tv_sec = 5;
  ti.tv_usec = 0;
  h->setTimeout(ti);

  int64_t a = 1;
  while (a != SOCKET_NO_DATA && a >= 0) {
    a = h->read(&str);
  }
  printf("we ret from read with %d\n", a);

}