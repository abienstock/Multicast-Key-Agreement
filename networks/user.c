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
  int max_sock;
  
  
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
  max_sock = oob_sock;

  memset(&oob_addr, 0, sizeof(oob_addr));
  oob_addr.sin_family = AF_INET;
  oob_addr.sin_addr.s_addr = inet_addr(oob_host);
  oob_addr.sin_port = htons(oob_port);

  if (connect(oob_sock, (struct sockaddr *) &oob_addr, sizeof(oob_addr)) < 0) {
    perror("connect() failed");
    exit(-1);
  }

  if ((mult_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    perror("socket() failed");
    exit(-1);
  }
  if (mult_sock > max_sock)
    max_sock = mult_sock;

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

  fd_set read_fds, write_fds;
  FD_ZERO(&write_fds);

  for (;;) {
    FD_ZERO(&read_fds);
    FD_SET(oob_sock, &read_fds);
    FD_SET(mult_sock, &read_fds);

    if (select(max_sock + 1, &read_fds, &write_fds, NULL, NULL) < 0) {
      perror("select() failed");
      exit(-1);
    }

    if (FD_ISSET(oob_sock, &read_fds)) {
      char recv_buf[32];
      int remaining = sizeof(recv_buf);
      int result = 0;
      int received = 0;
      while (remaining > 0) {
	if ((result = recv(oob_sock, recv_buf + received, remaining, 0)) < 0) {
	  perror("recv() failed");
	  exit(-1);
	} else {
	  remaining -= result;
	  received += result;
	}
      }
      printf("%s", recv_buf);
    } else if (FD_ISSET(mult_sock, &read_fds)) {
      char mult_buf[4096];
      socklen_t len = sizeof(mult_addr);
      if (recvfrom(mult_sock, mult_buf, sizeof(mult_buf), 0, (struct sockaddr *) &mult_addr, &len) < 0) {
	perror("recvfrom() failed");
	exit(-1);
      }
      
      printf("%s\n", mult_buf);
    }
  }
}
