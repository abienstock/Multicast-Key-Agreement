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

#define MAXPENDING 5    /* Maximum outstanding connection requests */

struct SockObj {
  int sock;
  int id;
  void *oob;
};

static void printSkeleton(void *p)
{
  struct SkeletonNode *node = (struct SkeletonNode *) p;
  if (node->children_color == NULL)
    printf("no children");
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

static void printNode(void *p)
{
  struct NodeData *data = (struct NodeData *) ((struct Node *) p)->data;
  if (data->blank == 1)
    printf("BLANK, id: %d", data->id);
  else {
    printf("id: %d, ", data->id);
    printf("key: %d, ", *((int *)data->key));
    printf("seed: %d.", *((int *)data->seed));
  }
}

/*static void print_secrets(void *data) {
  struct PathData *path_data = (struct PathData *) data;
  printf("id: %d, seed: %d, key %d\n", path_data->node_id, *((int *) path_data->seed), *((int *) path_data->key));
  }*/

void *int_gen() {
  int *seed = malloc(sizeof(int));
  if (seed == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }
  *seed = rand();
  return (void *) seed;
}

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

int rand_int(int n, int distrib, float geo_param) {
  int i;
  switch (distrib) {
  case 0: //uniform
    return rand() % n; // in range 0 to n-1
  case 1: // geo
    for (i = 0; i < n; i++) {
      if (((float) rand() / (float) RAND_MAX) < geo_param)
	return i;
    }
    return n-1; // 0-indexed
  default:
    return -1; //TODO: error checking!!
  }
}

//preorder traversal
void write_skeleton(struct SkeletonNode *skel_node, FILE *f) {
  struct NodeData *data = ((struct NodeData *) skel_node->node->data);
  fprintf(f, "%d ", data->id);
  if (skel_node->children_color != NULL) {
    // implicitly encode blue/red
    struct NodeData *child_data;
    struct Ciphertext *ciphertext;
    if (*skel_node->children_color == 1) {
      child_data = (struct NodeData *) (*skel_node->node->children)->data;
      ciphertext = *skel_node->ciphertexts;
      fprintf(f, "%d %d ", child_data->id, *((int *) ciphertext->ct));
    }
    else
      fprintf(f, "! ! ");
    if (*(skel_node->children_color + 1) == 1) {
      child_data = (struct NodeData *) (*(skel_node->node->children + 1))->data;
      ciphertext = *(skel_node->ciphertexts + 1);
      fprintf(f, "%d %d ", child_data->id, *((int *) ciphertext->ct));      
    }
    else
      fprintf(f, "! ! ");
  } else {
    fprintf(f, "! ! ! ! ");
  }
  if (skel_node->children != NULL){
    if (*skel_node->children != NULL) {
      fprintf(f, "1 ");
      write_skeleton(*skel_node->children, f);
    } else {
      fprintf(f, "0 ");
    }
    if (*(skel_node->children + 1) != NULL) {
      fprintf(f, "1 ");
      write_skeleton(*(skel_node->children + 1), f);
    } else {
      fprintf(f, "0 ");
    }
  } else {
    fprintf(f, "0 0 ");
  }
}

int next_op(struct Multicast *multicast, struct List *users, float add_wt, float upd_wt, int distrib, float geo_param, int *max_id) {
  int num_users = multicast->users->len;
  float operation = (float) rand() / (float) RAND_MAX;
  if (operation < add_wt || num_users == 1) { //TODO: forcing add with n=1 correct??
    (*max_id)++; //so adding new users w.l.o.g.
    printf("add: %d\n", *max_id);
    struct MultAddRet add_ret = mult_add(multicast, *max_id, &int_gen, &int_prg, &int_split, &int_identity);

    //pretty_traverse_tree(((struct LBBT *)multicast->tree)->root, 0, &printIntLine);
    //pretty_traverse_skeleton(add_ret.skeleton, 0, &printSkeleton);

    addFront(multicast->users, add_ret.added);
    //struct NodeData *root_data = (struct NodeData *) add_ret.skeleton->node->data;
    //printf("skeleton root seed: %d\n", *((int *) int_prg(root_data->seed)));

    struct ListNode *user_curr = users->head;
    while (user_curr != 0) {
      //void *root_seed =
      free(proc_ct((struct User *) user_curr->data, *max_id, add_ret.skeleton, NULL, &int_prg, &int_split, &int_identity));
      //traverseList(((struct User *)user_curr->data)->secrets, &print_secrets);
      //printf("root seed: %d\n", *((int *) root_seed));
      user_curr = user_curr->next;
    }
    
    struct User *user = init_user(*max_id);
    addFront(users, (void *) user);
    //void *root_seed =
    free(proc_ct(user, *max_id, add_ret.skeleton, add_ret.oob_seed, &int_prg, &int_split, &int_identity));
    //traverseList(user->secrets, &print_secrets);
    //printf("root seed: %d\n", *((int *) root_seed));

    destroy_skeleton(add_ret.skeleton);
    
    return 0;
  } else if (operation < add_wt + upd_wt) {
    int user = rand_int(num_users, distrib, geo_param);
    struct Node *user_node = (struct Node *) findNode(multicast->users, user)->data;
    struct NodeData *user_data = (struct NodeData *) user_node->data;
    printf("upd: %d\n", user_data->id);
    struct MultUpdRet upd_ret = mult_update(multicast, user_node, &int_gen, &int_prg, &int_split, &int_identity);
    //pretty_traverse_tree(((struct LBBT *)multicast->tree)->root, 0, &printIntLine);
    //pretty_traverse_skeleton(upd_ret.skeleton, 0, &printSkeleton);
    //struct NodeData *root_data = (struct NodeData *) upd_ret.skeleton->node->data;
    //printf("skeleton root seed: %d\n", *((int *) int_prg(root_data->seed)));

    struct ListNode *user_curr = users->head;
    while (user_curr != 0) {
      //void *root_seed;
      if (((struct User *) user_curr->data)->id == user_data->id) {
	//root_seed =
	free(proc_ct((struct User *) user_curr->data, user_data->id, upd_ret.skeleton, upd_ret.oob_seed, &int_prg, &int_split, &int_identity));
      } else {
	//root_seed =
	free(proc_ct((struct User *) user_curr->data, user_data->id, upd_ret.skeleton, NULL, &int_prg, &int_split, &int_identity));
      }
      //traverseList(((struct User *)user_curr->data)->secrets, &print_secrets);
      //printf("root seed: %d\n", *((int *) root_seed));
      user_curr = user_curr->next;
    }

    destroy_skeleton(upd_ret.skeleton);
    
    return 1;
  } else {
    int user_num = rand_int(num_users, distrib, geo_param);
    printf("user: %d\n", user_num);
    struct Node *user_node = (struct Node *) findAndRemoveNode(multicast->users, user_num);
    struct NodeData *user_data = (struct NodeData *) user_node->data;
    int rem_id = user_data->id;
    printf("rem: %d\n", rem_id);
    struct RemRet rem_ret = mult_rem(multicast, user_node, &int_gen, &int_prg, &int_split, &int_identity);

    //pretty_traverse_tree(((struct LBBT *)multicast->tree)->root, 0, &printIntLine);
    //pretty_traverse_skeleton(rem_ret.skeleton, 0, &printSkeleton);

    //struct NodeData *root_data = (struct NodeData *) rem_ret.skeleton->node->data;
    //printf("skeleton root seed: %d\n", *((int *) int_prg(root_data->seed)));

    struct User *user = (struct User *) findAndRemoveNode(users, user_num);
    destroy_user(user);

    struct ListNode *user_curr = users->head;
    while (user_curr != 0) {
      if (((struct User *) user_curr->data)->id != rem_id) {
	//void *root_seed =
	free(proc_ct((struct User *) user_curr->data, rem_id, rem_ret.skeleton, NULL, &int_prg, &int_split, &int_identity));
	//traverseList(((struct User *)user_curr->data)->secrets, &print_secrets);
	//printf("root seed: %d\n", *((int *) root_seed));
      }
      user_curr = user_curr->next;
    }

    destroy_skeleton(rem_ret.skeleton);
    
    return 2;
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

  //FILE *mult_f = fdopen(mult_sock, "ab+");
  FILE *skel_f;

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
      sock_obj->oob = &sock_obj->id;
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
	struct MultInitRet init_ret = mult_init(id, tree_flags, 0, &int_gen, &int_prg, &int_split, &int_identity);
	multicast = init_ret.multicast;
	struct SkeletonNode *skeleton = init_ret.skeleton;
	struct List *oob_seeds = init_ret.oob_seeds;
	//struct NodeData *root_data = (struct NodeData *) skeleton->node->data;
	//printf("skeleton root seed: %d\n", *((int *) int_prg(root_data->seed)));
	
	pretty_traverse_tree(((struct LBBT *)multicast->tree)->root, 0, &printNode);
	pretty_traverse_skeleton(skeleton, 0, &printSkeleton);

	int i;
	struct ListNode *socks_curr = socks.head;
	struct ListNode *oob_curr = oob_seeds->head;
	for (i = 0; i < N_uni; i++) { //TODO: FIX THIS HACK
	  struct SockObj *sock = (struct SockObj *) socks_curr->data;
	  if (sock->id < id) { //TODO: FIX THIS HACK
	    printf("id: %d\n", sock->id);
	    sock->oob = oob_curr->data;
	    FD_SET(sock->sock, &write_fds);
	    oob_curr = oob_curr->next;	    
	  }
	  socks_curr = socks_curr->next;
	}

	skel_f = fopen("skel.txt", "ab+");
	fprintf(skel_f, "%d ", op);
	write_skeleton(skeleton, skel_f);
	destroy_skeleton(skeleton);
	fclose(skel_f);	
	FD_SET(mult_sock, &write_fds);
      } else if (op == 0) {
	struct MultAddRet add_ret = mult_add(multicast, id, &int_gen, &int_prg, &int_split, &int_identity);
	struct ListNode *socks_curr = socks.head;
	while (socks_curr != NULL) {
	  struct SockObj *sock = (struct SockObj *) socks_curr->data;
	  if (sock->id == id) {
	    sock->oob = add_ret.oob_seed;
	    FD_SET(sock->sock, &write_fds);
	  }
	  socks_curr = socks_curr -> next;
	}
	skel_f = fopen("skel.txt", "ab+");
	fprintf(skel_f, "%d ", id);
	write_skeleton(add_ret.skeleton, skel_f);
	destroy_skeleton(add_ret.skeleton);
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
	struct MultUpdRet upd_ret = mult_update(multicast, user_node, &int_gen, &int_prg, &int_split, &int_identity); //TODO: replace id with node
	struct ListNode *socks_curr = socks.head;
	while (socks_curr != NULL) {
	  struct SockObj *sock = (struct SockObj *) socks_curr->data;
	  if (sock->id == id) {
	    sock->oob = upd_ret.oob_seed;
	    FD_SET(sock->sock, &write_fds);
	  }
	  socks_curr = socks_curr -> next;
	}
	skel_f = fopen("skel.txt", "ab+");
	fprintf(skel_f, "%d ", id);
	write_skeleton(upd_ret.skeleton, skel_f);
	destroy_skeleton(upd_ret.skeleton);
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
	struct RemRet rem_ret = mult_rem(multicast, user_node, &int_gen, &int_prg, &int_split, &int_identity); //TODO: replace id with node
	skel_f = fopen("skel.txt", "ab+");
	fprintf(skel_f, "%d ", id);
	write_skeleton(rem_ret.skeleton, skel_f);
	destroy_skeleton(rem_ret.skeleton);
	fclose(skel_f);
	FD_SET(mult_sock, &write_fds);
      } //TODO: error handling
    } else {
      //printf("writing\n");
      struct ListNode *curr = socks.head;
      while (curr != NULL) {
	struct SockObj *sock = (struct SockObj *) curr->data;
	if (FD_ISSET(sock->sock, &write_fds)) {
	  printf("oob_val: %d\n", *((int *) sock->oob));
	  char send_buf[32];
	  sprintf(send_buf, "%d", *((int *) sock->oob));
	  int remaining = sizeof(send_buf);
	  int result = 0;
	  int sent = 0;
	  while (remaining > 0) {
	    if ((result = send(sock->sock, send_buf + sent, remaining, 0)) < 0) {
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
	skel_f = fopen("skel.txt", "r");
	
	char f_content[4096];
	size_t n;
	printf("sending skel\n");
	while ((n = fread(f_content, sizeof(char), sizeof(f_content), skel_f)) > 0) {
	  printf("content: %s\n", f_content);
	  //fwrite(f_content, 1, n, mult_f);	  
	  if(sendto(mult_sock, f_content, n, 0, (struct sockaddr *) &mult_addr, sizeof(mult_addr)) < 0) {
	    perror("sendto() failed");
	    exit(-1);
	  }		  
	}

	fclose(skel_f);
	remove("skel.txt");
	FD_CLR(mult_sock, &write_fds);
      }
    }
  }



  /*int *max_id = malloc(sizeof(int));
  if (max_id == NULL) {
    perror("malloc returned NULL");
    return -1;
  }
  *max_id = n-1;



  int ops[3] = { 0, 0, 0 };

  //int i;
  //for (i = 0; i < atoi(argv[2]); i++) {
  //  ops[next_op(lbbt_multicast, NULL, -1, -1, -1, -1, max_id)]++;
  //}

  printf("# adds: %d, # updates: %d, # rems %d\n", ops[0], ops[1], ops[2]);
  printf("# PRGs: %d, # encs: %d\n", *(lbbt_multicast->counts), *(lbbt_multicast->counts + 1));

  mult_destroy(lbbt_multicast);
  free(max_id);*/
  return 0;
}
