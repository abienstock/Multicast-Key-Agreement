#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "../group_manager/trees/trees.h"
#include "../ll/ll.h"
#include "../group_manager/multicast/multicast.h"
#include "../users/user.h"
#include "../crypto/crypto.h"

#define MAXPENDING 5    /* Maximum outstanding connection requests */

struct SockObj {
  int sock;
  int id;
  uint8_t *oob;
  size_t oob_bytes;
};

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
      printf("%d, parent id: %d, child id: %d, ", *((int *) left_ct->ct), left_ct->parent_id, left_ct->child_id);
    }
    printf("right: %d, ", *(node->children_color+1));
    if (*(node->children_color + 1) == 1) {
      struct Ciphertext *right_ct = *(node->ciphertexts + 1);
      printf("right ct: ");
      printf("%d, parent id: %d, child id: %d.", *((int *) right_ct->ct), right_ct->parent_id, right_ct->child_id);
    }
  }
}

static void printNode(void *p)
{
  struct NodeData *data = (struct NodeData *) ((struct Node *) p)->data;
  if (data->blank == 1)
    printf("BLANK, id: %d", data->id);
  else {
    printf("id: %d, ", data->id);
    printf("key: %d, ", *((int *) data->key));
    printf("seed: %d.", *((int *) data->seed));
  }
}

/*static void print_secrets(void *data) {
  struct PathData *path_data = (struct PathData *) data;
  printf("id: %d, seed: %d, key %d\n", path_data->node_id, *((int *) path_data->seed), *((int *) path_data->key));
  }*/

int driver_split(uint8_t *out, uint8_t *seed, uint8_t *key, uint8_t *nonce, uint8_t *next_seed, size_t seed_size, size_t key_size, size_t nonce_size) {
  uint8_t *out_bytes = (uint8_t *) out;
  uint8_t *seed_bytes = (uint8_t *) seed;
  uint8_t *key_bytes = (uint8_t *) key;
  uint8_t *nonce_bytes = (uint8_t *) nonce;
  uint8_t *next_seed_bytes = (uint8_t *) next_seed;
  memcpy(seed_bytes, out_bytes, seed_size);
  memcpy(key_bytes, out_bytes + seed_size, key_size);
  memcpy(nonce_bytes, out_bytes + seed_size + key_size, nonce_size);
  memcpy(next_seed_bytes, out_bytes + seed_size + key_size + nonce_size, seed_size);
  return 0;
}

//preorder traversal
void write_skeleton(struct SkeletonNode *skel_node, FILE *f, size_t ct_size) {
  struct NodeData *data = ((struct NodeData *) skel_node->node->data);
  //fprintf(f, "%d ", data->id);
  fwrite(&data->id, 4, 1, f);
  //fprintf(f, " ");	  
  if (skel_node->children_color != NULL) {
    // implicitly encode blue/red
    struct NodeData *child_data;
    struct Ciphertext *ciphertext;
    if (*skel_node->children_color == 1) {
      child_data = (struct NodeData *) (*skel_node->node->children)->data;
      ciphertext = *skel_node->ciphertexts;
      //fprintf(f, "%d ", child_data->id);
      fwrite(&child_data->id, 4, 1, f);
      //fprintf(f, " ");	        
      fwrite((uint8_t *) ciphertext->ct, ct_size, 1, f); //TODO: check if casting needed
      //fprintf(f, " ");
    } else {
      //fprintf(f, "! ! ");
      uint8_t no_ct = 0;
      int j;
      for (j = 0; j < 4 + ct_size; j++) {
	fwrite(&no_ct, 1, 1, f);
      }
    }
    if (*(skel_node->children_color + 1) == 1) {
      child_data = (struct NodeData *) (*(skel_node->node->children + 1))->data;
      ciphertext = *(skel_node->ciphertexts + 1);
      //fprintf(f, "%d ", child_data->id);
      fwrite(&child_data->id, 4, 1, f);
      //fprintf(f, " ");	        
      printf("%d\n", *((int *) ciphertext->ct));
      fwrite((uint8_t *) ciphertext->ct, ct_size, 1, f);
      //fprintf(f, " ");
    } else {
      //fprintf(f, "! ! ");
      uint8_t no_ct = 0;
      int j;
      for (j = 0; j < 4 + ct_size; j++) {
	fwrite(&no_ct, 1, 1, f);
      }      
    }
  } else {
    //fprintf(f, "! ! ! ! ");
    uint8_t no_ct = 0;
    int j;
    for (j = 0; j < 2 * (4 + ct_size); j++) {
      fwrite(&no_ct, 1, 1, f);
    }
  }

  //children
  uint8_t child = 1;
  uint8_t no_child = 0;
  if (skel_node->children != NULL){
    if (*skel_node->children != NULL) {
      printf("left child\n");
      //fprintf(f, "1 ");
      fwrite(&child, 1, 1, f);
      write_skeleton(*skel_node->children, f, ct_size);
    } else {
      //fprintf(f, "0 ");
      fwrite(&no_child, 1, 1, f);
    }
    if (*(skel_node->children + 1) != NULL) {
      printf("right child\n");      
      //fprintf(f, "1 ");
      fwrite(&child, 1, 1, f);
      write_skeleton(*(skel_node->children + 1), f, ct_size);
    } else {
      fwrite(&no_child, 1, 1, f);
      //fprintf(f, "0 ");
    }
  } else {
    //fprintf(f, "0 0 ");
    fwrite(&no_child, 1, 1, f);
    fwrite(&no_child, 1, 1, f);
  }
}

int main(int argc, char *argv[]) {
  int oob_sock;
  int clnt_sock;
  int mult_sock;
  struct sockaddr_in oob_addr;
  struct sockaddr_in mult_addr;
  unsigned short oob_port;
  unsigned short mult_port;
  unsigned int clnt_len;
  unsigned char multicast_ttl = 1;  
  int max_sock;
  int N_uni;
  void *sampler = NULL, *generator = NULL, *cipher = NULL, *test_cipher = NULL;

  srand(time(0));

  if (argc != 8) {
    printf("Usage: [oob host] [oob port] [mult host] [mult port] [tree type] [LBBT addition strategy (if tree type = 0)] [LBBT truncation strategy (if tree type = 0)] [B-tree addition strategy (if tree type = 1)] [B-tree degree (if tree type = 1)]\n\nParameter Description:\n[oob host]: oob ip addr (-1 if no network)\n[oob port] oob port (-1 if no network)\n[mult host] multicast ip addr (-1 if no network)\n[mult port] multicast port (-1 if no network)\n[tree type]: type of tree structure for keys -- 0 = LBBT, 1 = B-tree\n[LBBT addition strategy]: The strategy used to add users in an LBBT tree -- 0 = greedy, 1 = random, 2 = append\n[LBBT truncation strategy]: The strategy used to truncate the LBBT tree after removals -- 0 = truncate, 1 = keep\n[B-tree addition strategy]: The strategy used to add users in B trees -- 0 = greedy, 1 = random\n[B-tree degree]: Degree of the B-tree -- 3 = 3, 4 = 4\n");
    exit(0);
  }

  char *oob_host = argv[1];
  oob_port = atoi(argv[2]);
  char *mult_host = argv[3];
  mult_port = atoi(argv[4]);  
  int tree_flags[2] = { atoi(argv[6]), atoi(argv[7]) }; // LBBT

  if ((oob_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    perror("socket() failed");
    exit(-1);
  }
  max_sock = oob_sock;

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

  printf("set up crypto\n");
  //FILE *mult_f = fdopen(mult_sock, "ab+");
  FILE *skel_f;
  
  sampler_init(&sampler);
  prg_init(&generator);
  cipher_init(&cipher, 0); //0 for enc mode
  cipher_init(&test_cipher, 1);
  
  size_t seed_size, ct_size;
  get_seed_size(generator, &seed_size);
  get_ct_size(cipher, seed_size, &ct_size);
  printf("done setting up crypto\n");

  clnt_len = sizeof(oob_addr);
  N_uni = 0;
  struct List socks;
  initList(&socks);

  fd_set read_fds, write_fds;  
  FD_ZERO(&write_fds);

  struct Multicast *multicast = NULL;

  for (;;) {
    FD_ZERO(&read_fds);
    FD_SET(oob_sock, &read_fds);
    FD_SET(0, &read_fds);

    if (select(max_sock + 1, &read_fds, &write_fds, NULL, NULL) < 0) {
      perror("select() failed");
      exit(-1);
    }

    if (FD_ISSET(oob_sock, &read_fds)) {
      if ((clnt_sock = accept(oob_sock, (struct sockaddr *) &oob_addr, &clnt_len)) < 0) {
	perror("accept() failed");
	exit(-1);
      }

      struct SockObj *sock_obj = malloc(sizeof(struct SockObj));
      if (sock_obj == NULL) {
	perror("malloc returned NULL");
	return -1;
      }
      sock_obj->sock = clnt_sock;
      sock_obj->id = N_uni++;
      //memcpy(sock_obj->oob, &sock_obj->id, 4); //TODO: make sure correct
      sock_obj->oob = &sock_obj->id;
      sock_obj->oob_bytes = 4; //TODO: ALWAYS 4??
      if (addFront(&socks, sock_obj) == NULL) {
	perror("addFront() failed");
	return -1;
      }
      if (clnt_sock > max_sock)
	max_sock = clnt_sock;

      printf("num users: %d\n", N_uni);
      
      FD_SET(clnt_sock, &write_fds);

    } else if (FD_ISSET(0, &read_fds)) {
      int op, id; // if op = -1: create, id = num users, op = 0: add, op = 1: upd, op = 2: rem
      scanf("%d %d", &op, &id);

      if (op == -1) {
	struct MultInitRet init_ret = mult_init(id, tree_flags, 0, sampler, generator, cipher);
	printf("initialized\n");
	multicast = init_ret.multicast;
	struct SkeletonNode *skeleton = init_ret.skeleton;
	struct List *oob_seeds = init_ret.oob_seeds;
	//struct NodeData *root_data = (struct NodeData *) skeleton->node->data;
	//printf("skeleton root seed: %d\n", *((int *) int_prg(root_data->seed)));
	
	pretty_traverse_tree(((struct LBBT *)multicast->tree)->root, 0, &printNode);
	pretty_traverse_skeleton(skeleton, 0, &printSkeleton);
	size_t out_size;
	get_prg_out_size(generator, &out_size);
	uint8_t *out = malloc(out_size);
	if (out == NULL) {
	  perror("malloc returned NULL");
	  return NULL;
	}
	struct NodeData *data = ((struct LBBT *)multicast->tree)->root->data;
	prg(generator, data->seed, out);
	printf("root seed: %d\n", *((int *) out + 3));

	int i;
	struct ListNode *socks_curr = socks.head;
	struct ListNode *oob_curr = oob_seeds->head;
	for (i = 0; i < N_uni; i++) { //TODO: FIX THIS HACK
	  struct SockObj *sock = (struct SockObj *) socks_curr->data;
	  if (sock->id < id) { //TODO: FIX THIS HACK
	    printf("id: %d\n", sock->id);
	    sock->oob = oob_curr->data;
	    sock->oob_bytes = seed_size;
	    FD_SET(sock->sock, &write_fds);
	    oob_curr = oob_curr->next;	    
	  }
	  socks_curr = socks_curr->next;
	}

	skel_f = fopen("skel.txt", "ab+");
	//fprintf(skel_f, "%d ", op);
	fwrite(&op, 4, 1, skel_f);
	//fprintf(skel_f, " ");	  	
	write_skeleton(skeleton, skel_f, ct_size);
	//destroy_skeleton(skeleton);
	fclose(skel_f);	
	FD_SET(mult_sock, &write_fds);
      } else if (op == 0) {
	struct MultAddRet add_ret = mult_add(multicast, id, sampler, generator, cipher);
	pretty_traverse_tree(((struct LBBT *)multicast->tree)->root, 0, &printNode);
	pretty_traverse_skeleton(add_ret.skeleton, 0, &printSkeleton);
	size_t out_size;
	get_prg_out_size(generator, &out_size);
	uint8_t *out = malloc(out_size);
	if (out == NULL) {
	  perror("malloc returned NULL");
	  return NULL;
	}
	struct NodeData *data = ((struct LBBT *)multicast->tree)->root->data;
	prg(generator, data->seed, out);
	printf("root seed: %d\n", *((int *) out + 3));	
	
	struct ListNode *socks_curr = socks.head;
	while (socks_curr != NULL) {
	  struct SockObj *sock = (struct SockObj *) socks_curr->data;
	  if (sock->id == id) {
	    sock->oob = add_ret.oob_seed;
	    sock->oob_bytes = seed_size;
	    FD_SET(sock->sock, &write_fds);
	  }
	  socks_curr = socks_curr -> next;
	}
	skel_f = fopen("skel.txt", "ab+");
	//fprintf(skel_f, "%d ", id);
	fwrite(&id, 4, 1, skel_f);
	//fprintf(skel_f, " ");	  	
	write_skeleton(add_ret.skeleton, skel_f, ct_size);
	//destroy_skeleton(add_ret.skeleton);
	fclose(skel_f);
	FD_SET(mult_sock, &write_fds);
      } else if (op == 1) {
	struct List *users = multicast->users;
	struct ListNode *users_curr = users->head;
	struct Node *user_node = NULL;
	while (users_curr != NULL) {
	  user_node = (struct Node *) users_curr->data;
	  struct NodeData *user_data = (struct NodeData *) user_node->data;
	  if (user_data->id == id)
	    break;
	  users_curr = users_curr->next;
	}
	struct MultUpdRet upd_ret = mult_update(multicast, user_node, sampler, generator, cipher);
	pretty_traverse_tree(((struct LBBT *)multicast->tree)->root, 0, &printNode);
	pretty_traverse_skeleton(upd_ret.skeleton, 0, &printSkeleton);
	size_t out_size;
	get_prg_out_size(generator, &out_size);
	uint8_t *out = malloc(out_size);
	if (out == NULL) {
	  perror("malloc returned NULL");
	  return NULL;
	}
	struct NodeData *data = ((struct LBBT *)multicast->tree)->root->data;
	prg(generator, data->seed, out);
	printf("root seed: %d\n", *((int *) out + 3));	
	
	struct ListNode *socks_curr = socks.head;
	while (socks_curr != NULL) {
	  struct SockObj *sock = (struct SockObj *) socks_curr->data;
	  if (sock->id == id) {
	    sock->oob = upd_ret.oob_seed;
	    sock->oob_bytes = seed_size;
	    FD_SET(sock->sock, &write_fds);
	  }
	  socks_curr = socks_curr -> next;
	}
	skel_f = fopen("skel.txt", "ab+");
	//fprintf(skel_f, "%d ", id);
	fwrite(&id, 4, 1, skel_f);
	//fprintf(skel_f, " ");	
	write_skeleton(upd_ret.skeleton, skel_f, ct_size);
	//destroy_skeleton(upd_ret.skeleton);
	fclose(skel_f);
	FD_SET(mult_sock, &write_fds);	
      } else if (op == 2) {
	struct List *users = multicast->users;
	struct ListNode *users_curr = users->head;
	struct Node *user_node = NULL;
	while (users_curr != NULL) {
	  user_node = (struct Node *) users_curr->data;
	  struct NodeData *user_data = (struct NodeData *) user_node->data;
	  if (user_data->id == id)
	    break;
	  users_curr = users_curr->next;
	}
	struct RemRet rem_ret = mult_rem(multicast, user_node, sampler, generator, cipher);
	pretty_traverse_tree(((struct LBBT *)multicast->tree)->root, 0, &printNode);
	pretty_traverse_skeleton(rem_ret.skeleton, 0, &printSkeleton);
	size_t out_size;
	get_prg_out_size(generator, &out_size);
	uint8_t *out = malloc(out_size);
	if (out == NULL) {
	  perror("malloc returned NULL");
	  return NULL;
	}
	struct NodeData *data = ((struct LBBT *)multicast->tree)->root->data;
	prg(generator, data->seed, out);
	printf("root seed: %d\n", *((int *) out + 3));	
	
	skel_f = fopen("skel.txt", "ab+");
	//fprintf(skel_f, "%d ", id); //TODO: change to bytes??
	fwrite(&id, 4, 1, skel_f);
	//fprintf(skel_f, " ");		
	write_skeleton(rem_ret.skeleton, skel_f, ct_size);
	//destroy_skeleton(rem_ret.skeleton);
	fclose(skel_f);
	FD_SET(mult_sock, &write_fds);
      } else if (op == -2) {
	size_t out_size;
	get_prg_out_size(generator, &out_size);
	uint8_t *out = malloc(out_size);
	if (out == NULL) {
	  perror("malloc returned NULL");
	  return NULL;
	}
	struct NodeData *data = ((struct LBBT *)multicast->tree)->root->data;
	prg(generator, data->seed, out);
	size_t key_size, nonce_size;
	get_key_size(cipher, &key_size);
	get_nonce_size(cipher, &nonce_size);
	uint8_t *new_seed = malloc(seed_size);
	uint8_t *key = malloc(key_size);
	uint8_t *nonce = malloc(nonce_size);
	uint8_t *next_seed = malloc(seed_size);
	driver_split(out, new_seed, key, nonce, next_seed, seed_size, key_size, nonce_size);
	size_t new_ct_size;
	get_ct_size(cipher, 5, &new_ct_size);
	uint8_t *ct = malloc(new_ct_size);
	char *pltxt = malloc(5);
	pltxt = "test";
	printf("test size: %zu\n", new_ct_size);
	enc(cipher, key, nonce, pltxt, ct, 5, new_ct_size);
	skel_f = fopen("skel.txt", "ab+");
	fwrite(&op, 4, 1, skel_f);
	fwrite(ct, new_ct_size, 1, skel_f);
	fclose(skel_f);
	FD_SET(mult_sock, &write_fds);
      } //TODO: error handling
    } else {
      //printf("writing\n");
      struct ListNode *curr = socks.head;
      while (curr != NULL) {
	struct SockObj *sock = (struct SockObj *) curr->data;
	if (FD_ISSET(sock->sock, &write_fds)) {
	  //printf("oob_val: %d\n", *((int *) sock->oob));
	  //printf("oob_val: %s\n", (char *) sock->oob);	  
	  //char send_buf[32];
	  //sprintf(send_buf, "%d", *((int *) sock->oob));
	  int remaining = sock->oob_bytes;
	  int result = 0;
	  int sent = 0;
	  while (remaining > 0) {
	    if ((result = send(sock->sock, ((uint8_t *) sock->oob) + sent, remaining, 0)) < 0) {
	      perror("send() failed");
	      exit(-1);
	    } else {
	      remaining -= result;
	      sent += result;
	    }
	  }
	  FD_CLR(sock->sock, &write_fds);
	}
	curr = curr->next;
      }
      if (FD_ISSET(mult_sock, &write_fds)) {
	skel_f = fopen("skel.txt", "rb");
	
	uint8_t f_content[4096]; //TODO: HACK!!
	size_t n;
	printf("sending skel\n");
	while ((n = fread(f_content, 1, 4096, skel_f)) > 0) {
	  printf("len: %zu\n", n);
	  printf("content: \n");
	  //printf("%s\n", f_content);
	  //fwrite(f_content, 1, n, mult_f);

	  int remaining = n;
	  int result = 0;
	  int sent = 0;
	  while (remaining > 0) {
	    if((result = sendto(mult_sock, f_content, n, 0, (struct sockaddr *) &mult_addr, sizeof(mult_addr))) < 0) {
	      perror("sendto() failed");
	      exit(-1);
	    } else {
	      remaining -= result;
	      sent += result;
	    }
	  }

	}

	fclose(skel_f);
	remove("skel.txt");
	FD_CLR(mult_sock, &write_fds);
      }
    }
  }

  return 0;
}
