#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "../skeleton.h"
#include "../group_manager/trees/trees.h"
#include "user.h"

void *int_prg(void *seed) {
  int *out = malloc(sizeof(int));
  if (out == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }
  *out = *((int *) seed) + 2;
  return (void *) out;
}

void **int_split(void *data) {
  int **split = malloc(sizeof(int *) * 3);
  int *seed = malloc(sizeof(int));
  int *key = malloc(sizeof(int));
  int *next_seed = malloc(sizeof(int));
  int out = *(int *) data;
  *seed = out;
  *key = out + 1;
  *next_seed = out + 2;

  *split = seed;
  *(split + 1) = key;
  *(split + 2) = next_seed;
  return (void **) split;
}

void *int_identity(void *key, void *data) {
  int *plaintext = malloc(sizeof(int));
  *plaintext = *((int *) data);
  return (void *) plaintext;
}

static void printSkeleton(void *p)
{
  struct SkeletonNode *node = (struct SkeletonNode *) p;
  if (node->children_color == NULL)
    printf("id: %d; no children", node->node_id);
  else {
    printf("left: %d, ", *(node->children_color));
    if (*(node->children_color) == 1) {
      struct Ciphertext *left_ct = *node->ciphertexts;
      printf("left ct: %d, parent id: %d, child id: %d, ", *((int *) left_ct->ct), left_ct->parent_id, left_ct->child_id);
    }
    printf("right: %d, ", *(node->children_color+1));
    if (*(node->children_color + 1) == 1) {
      struct Ciphertext *right_ct = *(node->ciphertexts + 1);
      printf("right ct: %d, parent id: %d, child id: %d.", *((int *) right_ct->ct), right_ct->parent_id, right_ct->child_id);
    }
  }
}

static void print_secrets(void *data) {
  struct PathData *path_data = (struct PathData *) data;
  printf("id: %d, seed: %d, key %d\n", path_data->node_id, *((int *) path_data->seed), *((int *) path_data->key));
}

int get_int(char **buf) {
  char temp[406];
  int i = 0;
  while (**buf != ' ' && **buf != '\0') {
    temp[i++] = **buf;
    (*buf)++;
  }
  if (**buf != '\0')
    (*buf)++;
  temp[i] = '\0';
  return atoi(temp);
}

struct Ciphertext *get_ct(char **buf, int parent_id) {
  struct Ciphertext *ciphertext = malloc(sizeof(struct Ciphertext));
  if (ciphertext == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }
  int child_id = get_int(buf);
  int ct = get_int(buf);
  int *ct_ptr = malloc(sizeof(int));
  if (ct_ptr == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }
  *ct_ptr = ct;
  ciphertext->parent_id = parent_id;
  ciphertext->child_id = child_id;
  ciphertext->ct = (void *) ct_ptr;
  return ciphertext;
}

struct SkeletonNode *build_skel(char **buf) {
  /*if (**buf == '\0') {
    printf("return\n");
    return NULL;
    }*/ 
  
  printf("printing\n");
  printf("%s\n", *buf);
  printf("done\n");
  struct SkeletonNode *skel = malloc(sizeof(struct SkeletonNode));
  if (skel == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }
  
  int id = get_int(buf);
  //printf("buf: %s id: %d\n", *buf, id);
  skel->node_id = id;
  skel->node = NULL;
  if (!(strncmp("! ! ! ! ", *buf, 8))) {
    skel->children_color = NULL;
    skel->children = NULL;
    skel->ciphertexts = NULL;
    (*buf += 12);
  } else {
    int *children_color = malloc(sizeof(int) * 2);
    if (children_color == NULL) {
      perror("malloc returned NULL");
      return NULL;
    }
    struct Ciphertext **cts = malloc(sizeof(struct Ciphertext *) * 2);
    if (cts == NULL) {
      perror("malloc returned NULL");
      return NULL;
    }
    if (!(strncmp("! ! ", *buf, 4))) {
      *cts++ = NULL;
      (*buf) += 4;
      struct Ciphertext* ciphertext = get_ct(buf, id);
      *cts-- = ciphertext;
      *children_color++ = 0;
      *children_color-- = 1;
    } else {
      struct Ciphertext* ciphertext = get_ct(buf, id);
      *cts++ = ciphertext;
      *children_color++ = 1;
      if (strncmp("! ! ", *buf, 4) != 0) {
	struct Ciphertext* ciphertext = get_ct(buf, id);
	*cts-- = ciphertext;
	*children_color-- = 1;
      } else {
	*cts-- = NULL;
	*children_color-- = 0;
	(*buf += 4);	
      }
    }
    skel->children_color = children_color;
    skel->ciphertexts = cts;

    //children??
    if (!(strncmp("0 0 ", *buf, 4))) {
      skel->children = NULL;
      (*buf += 4);
    } else {
      struct SkeletonNode **children = malloc(sizeof(struct SkeletonNode *) * 2);
      if (children == NULL) {
	perror("malloc returned NULL");
	return NULL;
      }
      if (!(strncmp("0 ", *buf, 2))) {
	*children++ = NULL;
	(*buf) += 4;
	struct SkeletonNode *right_skel = build_skel(buf);
	right_skel->parent = skel;
	*children-- = right_skel;
      } else {
	(*buf) += 2;	
	struct SkeletonNode *left_skel = build_skel(buf);
	left_skel->parent = skel;
	*children++ = left_skel;
	if (!(strncmp("0 ", *buf, 2))) {
	  *children-- = NULL;
	  (*buf) += 2;
	} else {
	  (*buf) += 2;	  
	  struct SkeletonNode *right_skel = build_skel(buf);
	  right_skel->parent = skel;
	  *children-- = right_skel;
	}
      }
      skel->children = children;
    }
  }
  return skel;
}

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

  struct User *user;
  int *seed = NULL;
  
  fd_set read_fds, write_fds;
  FD_ZERO(&write_fds);


  int i = 0;
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
      printf("oob_val: %s\n", recv_buf);
      if (i++ == 0)
	user = init_user(atoi(recv_buf));
      else {
	seed = malloc(sizeof(int)); //TODO: hack??
	if (seed == NULL) {
	  perror("malloc returned NULL");
	  exit(-1);
	}
	*seed = atoi(recv_buf);
      }
    } else if (FD_ISSET(mult_sock, &read_fds)) {
      char *mult_buf = malloc(sizeof(char) * 4096);
      if (mult_buf == NULL) {
	perror("malloc returned NULL");
	exit(-1);
      }
      socklen_t len = sizeof(mult_addr);
      int rcvd;
      if ((rcvd = recvfrom(mult_sock, mult_buf, 4096, 0, (struct sockaddr *) &mult_addr, &len)) < 0) {
	perror("recvfrom() failed");
	exit(-1);
      }
      printf("rcvd: %d buf: %s\n", rcvd, mult_buf);

      int id = get_int(&mult_buf);
      struct SkeletonNode *skel = build_skel(&mult_buf);
      pretty_traverse_skeleton(skel, 0, &printSkeleton);
      skel->parent = NULL;
      void *root_seed = proc_ct(user, id, skel, (void *) seed, &int_prg, &int_split, &int_identity);
      if (root_seed == NULL && user->secrets->head == NULL) {
	printf("not in group\n");
      } else if (root_seed == NULL) {
	printf("removed from group\n");
      } else {
	traverseList(user->secrets, &print_secrets);
	printf("root seed: %d\n", *((int *) root_seed));
      }
    }
  }
}
