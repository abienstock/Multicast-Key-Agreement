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
#include "../crypto/crypto.h"

static void printSkeleton(void *p)
{
  struct SkeletonNode *node = (struct SkeletonNode *) p;
  if (node->children_color == NULL)
    printf("id: %d; no children", node->node_id);
  else {
    printf("left: %d, ", *(node->children_color));
    if (*(node->children_color) == 1) {
      struct Ciphertext *left_ct = *node->ciphertexts;
      printf("left ct: ");
      printf("%d, parent id: %d, child id: %d, ", *((int * ) left_ct->ct), left_ct->parent_id, left_ct->child_id);
    }
    printf("right: %d, ", *(node->children_color+1));
    if (*(node->children_color + 1) == 1) {
      struct Ciphertext *right_ct = *(node->ciphertexts + 1);
      printf("right ct: ");
      printf("%d, parent id: %d, child id: %d.", *((int *) right_ct->ct), right_ct->parent_id, right_ct->child_id);
    }
  }
}

static void print_secrets(void *data) {
  struct PathData *path_data = (struct PathData *) data;
  printf("id: %d, seed: %d, key %d\n", path_data->node_id, *((int *) path_data->seed), *((int *) path_data->key));
}

int get_bytes(uint8_t **buf, uint8_t *result, int num_bytes) {
  int i = 0;
  //while (**buf != 32 && **buf != 0) { //ascii space and null
  while (i < num_bytes) {
    result[i++] = **buf;
    (*buf)++;
  }
  //if (**buf != 0)
  //(*buf)++; SHOULDNT NEED
  return 0;
}

struct Ciphertext *get_ct(uint8_t **buf, int parent_id, size_t ct_size) {
  int num_bytes;
  struct Ciphertext *ciphertext = malloc(sizeof(struct Ciphertext));
  if (ciphertext == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }

  num_bytes = 0;
  uint8_t child_id[4096]; //TODO: HACK!!
  get_bytes(buf, child_id, 4);
  
  num_bytes = 0;
  uint8_t *ct_ptr = malloc(sizeof(uint8_t) * ct_size);
  if (ct_ptr == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }
  get_bytes(buf, ct_ptr, ct_size);
  //*ct_ptr = ct;
  ciphertext->parent_id = parent_id;
  ciphertext->child_id = *((int *) child_id);
  ciphertext->ct = ct_ptr;
  return ciphertext;
}

struct SkeletonNode *build_skel(uint8_t **buf, void *generator, size_t seed_size) {
  /*if (**buf == '\0') {
    printf("return\n");
    return NULL;
    }*/
  int num_bytes;
  size_t ct_size;
  get_seed_size(generator, &ct_size);  
  
  //printf("%s\n", *buf);
  struct SkeletonNode *skel = malloc(sizeof(struct SkeletonNode));
  if (skel == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }

  num_bytes = 0;
  uint8_t id[4]; //TODO: HACK!!
  get_bytes(buf, id, 4);
  //printf("buf: %s id: %d\n", *buf, id);

  int id_num = *((int *) id);
  skel->node_id = id_num;
  skel->node = NULL;
  uint8_t no_child_cts[2*4 + 2 * ct_size];
  memset(no_child_cts, 0, 2*4 + 2 * ct_size);
  //if (!(strncmp("! ! ! ! ", *((char **) buf), 8))) {
  if (!(memcmp(no_child_cts, *buf, 2*4 + 2 * ct_size))) {
    skel->children_color = NULL;
    skel->children = NULL;
    skel->ciphertexts = NULL;
    (*buf) += 2*4 + 2*ct_size;
    (*buf) += 2;
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
    uint8_t no_child_ct[4 + ct_size];
    memset(no_child_ct, 0, 4 + ct_size);
    //if (!(strncmp("! ! ", *((char **) buf), 4))) {
    if (!(memcmp(no_child_ct, *buf, 4 + ct_size))) {
      *cts++ = NULL;
      (*buf) += 4 + ct_size;
      struct Ciphertext* ciphertext = get_ct(buf, id_num, ct_size);
      *cts-- = ciphertext;
      *children_color++ = 0;
      *children_color-- = 1;
    } else {
      struct Ciphertext* ciphertext = get_ct(buf, id_num, ct_size);
      *cts++ = ciphertext;
      *children_color++ = 1;
      uint8_t no_child_ct[4 + ct_size];
      memset(no_child_ct, 0, 4 + ct_size);
      //if (!(strncmp("! ! ", *((char **) buf), 4))) {
      if (!(memcmp(no_child_ct, *buf, 4 + ct_size))) {      
	*cts-- = NULL;
	*children_color-- = 0;
	(*buf) += 4 + ct_size;	
      } else {
	struct Ciphertext* ciphertext = get_ct(buf, id_num, ct_size);
	*cts-- = ciphertext;
	*children_color-- = 1;	
      }
    }
    skel->children_color = children_color;
    skel->ciphertexts = cts;

    //printf("byte %d\n", *(*buf));
    //printf("byte %d\n", *(*buf + 1));
    //printf("byte %d\n", *(*buf + 2));
    //printf("byte %d\n", *(*buf + 3));
    //children??
    uint8_t no_children[2];
    memset(no_children, 0, 2);        
    //if (!(strncmp("0 0 ", *((char **) buf), 4))) {
    if (!(memcmp(no_children, *buf, 2))) {
      skel->children = NULL;
      (*buf) += 2;
    } else {
      struct SkeletonNode **children = malloc(sizeof(struct SkeletonNode *) * 2);
      if (children == NULL) {
	perror("malloc returned NULL");
	return NULL;
      }
      //if (!(strncmp("0 ", *((char **) buf), 2))) {
      if (!memcmp(no_children, *buf, 1)) {
	*children++ = NULL;
	(*buf) += 2;
	struct SkeletonNode *right_skel = build_skel(buf, generator, seed_size);
	right_skel->parent = skel;
	*children-- = right_skel;
      } else {
	(*buf) += 1;
	struct SkeletonNode *left_skel = build_skel(buf, generator, seed_size);
	left_skel->parent = skel;
	*children++ = left_skel;
	//if (!(strncmp("0 ", *((char **) buf), 2))) {
	if (!memcmp(no_children, *buf, 1)) {
	  *children-- = NULL;
	  (*buf) += 1;
	} else {
	  (*buf) += 1;
	  struct SkeletonNode *right_skel = build_skel(buf, generator, seed_size);
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
  void *generator = NULL;
  
  
  if (argc != 5) {
    printf("Usage: [oob host] [oob port] [mult host] [mult port]\n\nParameter Description:\n[oob host]: oob ip addr (-1 if no network)\n[oob port] oob port (-1 if no network)\n[mult host] multicast ip addr (-1 if no network)\n[mult port] multicast port (-1 if no network)\n");
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
  prg_init(&generator);
  /*generator = malloc(sizeof(botan_mac_t));
  botan_mac_t *prf = (botan_mac_t *) generator;
  botan_mac_init(prf, "HMAC(SHA-512)", 0);
  cipher = malloc(sizeof(botan_cipher_t));
  botan_cipher_t *botan_cipher = (botan_cipher_t *) cipher;
  botan_cipher_init(botan_cipher, "ChaCha(20)", BOTAN_CIPHER_INIT_FLAG_DECRYPT);*/
  
  size_t seed_size;
  get_seed_size(generator, &seed_size);
  uint8_t *seed;
  
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
      int remaining;
      uint8_t recv_buf[(4 > seed_size ? 4 : seed_size)]; //TODO: hack?
      if (i == 0) {
	remaining = 4;
      }
      else {
	remaining = seed_size;
      }
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
      //printf("oob_val: %s\n", (char *) recv_buf);
      if (i++ == 0)
	user = init_user(*((int *) recv_buf));
      else {
	seed = malloc(seed_size);
	if (seed == NULL) {
	  perror("malloc returned NULL");
	  exit(-1);
	}
	memcpy(seed, recv_buf, seed_size);
	//printf("seed: %s\n", (char *) seed);
      }
    } else if (FD_ISSET(mult_sock, &read_fds)) {
      uint8_t *mult_buf = malloc(4096);
      if (mult_buf == NULL) {
	perror("malloc returned NULL");
	exit(-1);
      }
      socklen_t len = sizeof(mult_addr);
      int rcvd;
      if ((rcvd = recvfrom(mult_sock, mult_buf, 4096, 0, (struct sockaddr *) &mult_addr, &len)) < 0) { //TODO: recv loop?
	perror("recvfrom() failed");
	exit(-1);
      } //TODO: make sure all is received!!
      
      
      //printf("rcvd: %d buf: %s\n", rcvd, mult_buf);

      uint8_t id[4096]; //TODO: HACK!!
      get_bytes(&mult_buf, id, 4);
      
      if (*((int *) id) == -2) {
	printf("broadcast incoming...\n");
	if (user->in_group)
	  proc_broadcast(user, &mult_buf, generator, seed_size);
      } else {
	struct SkeletonNode *skel = build_skel(&mult_buf, generator, seed_size);

	printf("Skeleton:\n");
	pretty_traverse_skeleton(skel, 0, &printSkeleton);
	printf("\n..................\n");	
	skel->parent = NULL;
	uint8_t *root_seed = proc_ct(user, *((int *) id), skel, seed, generator, seed_size);
	if (root_seed == NULL && user->secrets->head == NULL) {
	  printf("not in group\n");
	  user->in_group = 0; //TODO: possibly redundant?
	} else if (root_seed == NULL) {
	  printf("removed from group\n");
	  user->in_group = 0;
	} else {
	  user->in_group = 1;
	  printf("Secret Path:\n");
	  traverseList(user->secrets, &print_secrets);
	  printf("root seed: %d\n", *((int *) root_seed + 3));
	}
	printf("\n===================\n");	
      }
    }
  }
}
