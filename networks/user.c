#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

int main (int argc, char *argv[]) {
  int oob_sock;
  int mult_sock;
  struct sockaddr_in oob_addr;
  struct sockaddr_in mult_addr;
  unsigned short oob_port;
  unsigned short mult_port;
  struct ip_mreq mreq;
  
  
  if (argc != 5) {
    perror("Incorrect number of arguments");
    exit(-1);
  }

  char *oob_host = argv[1];
  oob_port = atoi(argv[2]);
  
  char *mult_host = argv[3];
  mult_port = atoi(argv[4]);

  if ((oob_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    perror("socket() failed");
    exit(-1);
  }

  memset(&oob_addr, 0, sizeof(oob_addr));
  oob_addr.sin_family = AF_INET;
  oob_addr.sin_addr.s_addr = inet_addr(oob_host);
  oob_addr.sin_port = htons(oob_port);

  if (connect(oob_sock, (struct sockaddr *) &oob_addr, sizeof(oob_addr)) < 0) {
    perror("connect() failed");
    exit(-1);
  }

  char buf[1024];
  if (recv(oob_sock, buf, 1024, 0) < 0) {
    perror("recv() failed");
    exit(-1);
  }
  printf("%s\n", buf);

  if ((mult_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    perror("socket() failed");
    exit(-1);
  }

  u_int yes = 1;
  if (setsockopt(mult_sock, SOL_SOCKET, SO_REUSEPORT, (char *) &yes, sizeof(yes)) < 0) {
    perror("setsockopt() failed");
    exit(-1);
  }

  memset(&mult_addr, 0, sizeof(mult_addr));
  mult_addr.sin_family = AF_INET;
  mult_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  mult_addr.sin_port = htons(mult_port);

  if (bind(mult_sock, (struct sockaddr *) &mult_addr, sizeof(mult_addr)) < 0) {
    perror("bind() failed");
    exit(-1);
  }

  mreq.imr_multiaddr.s_addr = inet_addr(mult_host);
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);
  if (setsockopt(mult_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
    perror("setsockopt() failed");
    exit(-1);
  }
  
  for (;;) {
    char mult_buf[1024];
    socklen_t len = sizeof(mult_addr);
    if (recvfrom(mult_sock, mult_buf, sizeof(mult_buf), 0, (struct sockaddr *) &mult_addr, &len) < 0) {
      perror("recvfrom() failed");
      exit(-1);
    }

    printf("%s\n", mult_buf);
  }
}
