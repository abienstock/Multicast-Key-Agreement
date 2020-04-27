#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "../ll/ll.h"

#define MAXPENDING 5    /* Maximum outstanding connection requests */

struct SockObj {
  int sock;
  int id;
  void *oob;
};

int main(int argc, char *argv[]) {
  int oob_sock;
  int clnt_sock;
  int mult_sock;
  struct sockaddr_in oob_addr;
  struct sockaddr_in mult_addr;
  unsigned short oob_port;
  unsigned short mult_port;
  unsigned int clnt_len;
  unsigned char multicast_ttl;  
  int max_sock;
  
  
  if (argc != 5) {
    perror("Incorrect number of arguments");
    exit(-1);
  }

  char *oob_host = argv[1];
  oob_port = atoi(argv[2]);
  
  char *mult_host = argv[3];
  mult_port = atoi(argv[4]);
  multicast_ttl =  1;

  if ((oob_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    perror("socket() failed");
    exit(-1);
  }
  max_sock = oob_sock;
  //fcntl(oob_sock, F_SETFL, O_NONBLOCK);  

  memset(&oob_addr, 0, sizeof(oob_addr));
  oob_addr.sin_family = AF_INET;
  oob_addr.sin_addr.s_addr = inet_addr(oob_host);
  oob_addr.sin_port = htons(oob_port);

  if (bind(oob_sock, (struct sockaddr *) &oob_addr, sizeof(oob_addr)) < 0) {
    perror("bind() failed");
    exit(-1);
  }

  if (listen(oob_sock, MAXPENDING) < 0) {
    perror("listen() failed");
    exit(-1);
  }

  if ((mult_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    perror("socket() failed");
    exit(-1);
  }
  if (setsockopt(mult_sock, IPPROTO_IP, IP_MULTICAST_TTL, (void *) &multicast_ttl, sizeof(multicast_ttl)) < 0) {
    perror("setsockopt() failed");
    exit(-1);
  }
  if (mult_sock > max_sock)
    max_sock = mult_sock;

  memset(&mult_addr, 0, sizeof(mult_addr));
  mult_addr.sin_family = AF_INET;
  mult_addr.sin_addr.s_addr = inet_addr(mult_host);
  mult_addr.sin_port = htons(mult_port);

  clnt_len = sizeof(oob_addr);

  int N_uni = 0;
  struct List socks;
  initList(&socks);

  fd_set read_fds, write_fds;  
  FD_ZERO(&write_fds);

  int i = 0;

  for (;;) {
    int err;

    FD_ZERO(&read_fds);
    FD_SET(oob_sock, &read_fds);
    FD_SET(0, &read_fds);

    err = select(max_sock + 1, &read_fds, &write_fds, NULL, NULL);
    if (err < 0) {
      perror("select() failed");
      exit(-1);
    }

    if (FD_ISSET(oob_sock, &read_fds)) {
      if ((clnt_sock = accept(oob_sock, (struct sockaddr *) &oob_addr, &clnt_len)) < 0) {
	perror("accept() failed");
	exit(-1);
      }
      //fcntl(clnt_sock, F_SETFL, O_NONBLOCK);

      struct SockObj *sock_obj = malloc(sizeof(struct SockObj));
      if (sock_obj == NULL) {
	perror("malloc returned NULL");
	return -1;
      }
      sock_obj->sock = clnt_sock;
      sock_obj->id = N_uni++;
      sock_obj->oob = NULL;
      if (addFront(&socks, sock_obj) == NULL) {
	perror("addFront() failed");
	return -1;
      }
      if (clnt_sock > max_sock)
	max_sock = clnt_sock;
      
      //FD_SET(clnt_sock, &read_fds);

    } else if (FD_ISSET(0, &read_fds)) {
      int j;
      scanf("%d", &j);

      if (i < 2){
	struct ListNode *curr = socks.head;
	while (curr != NULL) {
	  struct SockObj *sock = (struct SockObj *) curr->data;
	  if (sock->id == j) {
	    sock->oob = "test";
	    FD_SET(sock->sock, &write_fds);
	    break;
	  }
	  curr = curr->next;
	}
	i++;
      } else {
	FD_SET(mult_sock, &write_fds);
      }
    } else {
      struct ListNode *curr = socks.head;
      while (curr != NULL) {
	struct SockObj *sock = (struct SockObj *) curr->data;	
	if (FD_ISSET(sock->sock, &write_fds)) {
	  //FILE *client = fdopen(sock->sock, "ab+");
	  if (send(sock->sock, sock->oob, strlen(sock->oob), 0) < 0) {
	    perror("send() failed");
	    exit(-1);
	  }
	  //fwrite(sock->oob, 1, strlen(sock->oob), client);
	  //fclose(client);
	  FD_CLR(sock->sock, &write_fds);
	}
	curr = curr->next;
      }
      if (FD_ISSET(mult_sock, &write_fds)) {
	if(sendto(mult_sock, "hi", strlen("hi"), 0, (struct sockaddr *) &mult_addr, sizeof(mult_addr)) < 0) {
	  perror("sendto() failed");
	  exit(-1);
	}
      }
      FD_CLR(mult_sock, &write_fds);
    }
  }
  
}
