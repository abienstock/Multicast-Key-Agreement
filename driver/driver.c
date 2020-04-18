#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "../group_manager/trees/trees.h"
#include "../ll/ll.h"
#include "../group_manager/multicast/multicast.h"
#include "../users/user.h"

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

int next_op(struct Multicast *multicast, struct List *users, float add_wt, float upd_wt, int distrib, float geo_param, int *max_id) {
  int num_users = multicast->users->len;
  float operation = (float) rand() / (float) RAND_MAX;
  if (operation < add_wt || num_users == 1) { //TODO: forcing add with n=1 correct??
    (*max_id)++; //so adding new users w.l.o.g.
    printf("add: %d\n", *max_id);
    struct MultAddRet add_ret = mult_add(multicast, *max_id, &int_gen, &int_prg, &int_split, &int_identity);
    addFront(multicast->users, add_ret.added);
    struct NodeData *root_data = (struct NodeData *) add_ret.skeleton->node->data;
    //printf("skeleton root seed: %d\n", *((int *) int_prg(root_data->seed)));

    struct ListNode *user_curr = users->head;
    while (user_curr != 0) {
      void *root_seed = proc_ct((struct User *) user_curr->data, *max_id, add_ret.skeleton, NULL, &int_prg, &int_split, &int_identity);
      //traverseList(((struct User *)user_curr->data)->secrets, &print_secrets);
      //printf("root seed: %d\n", *((int *) root_seed));
      user_curr = user_curr->next;
    }
    
    struct User *user = init_user(*max_id);
    addFront(users, (void *) user);
    void *root_seed = proc_ct(user, *max_id, add_ret.skeleton, add_ret.oob_seed, &int_prg, &int_split, &int_identity);
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
    struct SkeletonNode *skeleton = upd_ret.skeleton;
    //pretty_traverse_tree(((struct LBBT *)multicast->tree)->root, 0, &printIntLine);
    struct NodeData *root_data = (struct NodeData *) skeleton->node->data;
    //printf("skeleton root seed: %d\n", *((int *) int_prg(root_data->seed)));

    struct ListNode *user_curr = users->head;
    while (user_curr != 0) {
      void *root_seed;
      if (((struct User *) user_curr->data)->id == user_data->id)
	root_seed = proc_ct((struct User *) user_curr->data, user_data->id, skeleton, upd_ret.oob_seed, &int_prg, &int_split, &int_identity);
      else
	root_seed = proc_ct((struct User *) user_curr->data, user_data->id, skeleton, NULL, &int_prg, &int_split, &int_identity);
      //traverseList(((struct User *)user_curr->data)->secrets, &print_secrets);
      //printf("root seed: %d\n", *((int *) root_seed));
      user_curr = user_curr->next;
    }

    
    destroy_skeleton(skeleton);
    
    return 1;
  } else {
    int user = rand_int(num_users, distrib, geo_param);    
    printf("user: %d\n", user);
    struct Node *user_node = (struct Node *) findAndRemoveNode(multicast->users, user);
    struct NodeData *user_data = (struct NodeData *) user_node->data;
    int rem_id = user_data->id;
    printf("rem: %d\n", rem_id);
    struct RemRet rem_ret = mult_rem(multicast, user_node, &int_gen, &int_prg, &int_split, &int_identity);
    struct NodeData *root_data = (struct NodeData *) rem_ret.skeleton->node->data;
    //printf("skeleton root seed: %d\n", *((int *) int_prg(root_data->seed)));

    findAndRemoveNode(users, user);

    struct ListNode *user_curr = users->head;
    while (user_curr != 0) {
      if (((struct User *) user_curr->data)->id != rem_id) {
	void *root_seed = proc_ct((struct User *) user_curr->data, rem_id, rem_ret.skeleton, NULL, &int_prg, &int_split, &int_identity);
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
  int n, distrib;
  float add_wt, upd_wt, geo_param;

  srand(time(0));

  if (argc != 10 && argc != 11) {
    printf("Usage: [init group size] [num ops] [add weight] [update weight] [LBBT addition strategy] [LBBT truncation strategy] [B-tree addition strategy] [B-tree degree] [operation distribution] [geometric distribution parameter] \n\nParameter Description:\n[init group size]: number of users in group at initialization\n[num ops]: number of group operations\n[add weight]: probability that a given operation will be an add (in [0,1])\n[update weight]: probability that a given operation will be an update (in [0,1]). Note: add weight + update weight must be in [0,1], remove weight = 1 - add weight - remove weight (probability that a given operation will be a remove)\n[LBBT addition strategy]: The strategy used to add users in an LBBT tree -- 0 = greedy, 1 = random, 2 = append, 3 = compare all\n[LBBT truncation strategy]: The strategy used to truncate the LBBT tree after removals -- 0 = truncate, 1 = keep, 2 = compare all\n[B-tree addition strategy]: The strategy used to add users in B trees -- 0 = greedy, 1 = random, 2 = compare both\n[B-tree degree]: Degree of the B-tree -- 3 = 3, 4 = 4, 0 = compare both\n[operation distribution]: The distribution for choosing which users to perform operations (and for the user to be removed, if the operation is a removal) -- 0 = uniform, 1 = geometric\n[geometric distribution parameter]: The p parameter of the geometric distribution, if chosen\n");
    exit(0);
  }

  n = atoi(argv[1]);
  distrib = atoi(argv[9]);
  add_wt = atof(argv[3]);
  upd_wt = atof(argv[4]);

  if(add_wt + upd_wt > 1 || add_wt < 0 || upd_wt < 0) {
    perror("Weights must not be negative nor sum to greater than 1");
    exit(1);
  }
  //rem_wt = 1 - add_wt - upd_wt;
  
  geo_param = 0;
  if(distrib == 1) {
    geo_param = atof(argv[10]);
    if(geo_param >= 1 || geo_param <=0) {
      perror("p has to be 0 < p < 1");
      exit(1);
    }
  }
  
  int lbbt_flags[2] = { atoi(argv[5]), atoi(argv[6]) };

  int *max_id = malloc(sizeof(int));
  if (max_id == NULL) {
    perror("malloc returned NULL");
    return -1;
  }
  *max_id = n-1;

  struct List *users = malloc(sizeof(struct List)); // FOR TESTING
  if (users == NULL) {
    perror("malloc returned NULL");
    return -1;
  }
  initList(users);
  
  int i;
  for (i = 0; i < n; i++) {
    struct User *user = init_user(i);
    addFront(users, (void *) user);
  }

  struct MultInitRet init_ret = mult_init(n, lbbt_flags, 0, &int_gen, &int_prg, &int_split, &int_identity);
  struct Multicast *lbbt_multicast = init_ret.multicast;
  struct SkeletonNode *skeleton = init_ret.skeleton;
  struct List *oob_seeds = init_ret.oob_seeds;
  struct NodeData *root_data = (struct NodeData *) skeleton->node->data;
  //printf("skeleton root seed: %d\n", *((int *) int_prg(root_data->seed)));

  struct ListNode *user_curr = users->tail;
  //struct ListNode *mult_curr = lbbt_multicast->users->head;
  struct ListNode *oob_curr = oob_seeds->head;
  while (user_curr != 0) {
    //struct Node *leaf = (struct Node *) mult_curr->data;
    //struct NodeData *leaf_data = (struct NodeData *) leaf->data;
    void *root_seed = proc_ct((struct User *) user_curr->data, -1, skeleton, oob_curr->data, &int_prg, &int_split, &int_identity); // -1 for create
    //traverseList(((struct User *)user_curr->data)->secrets, &print_secrets);
    //printf("root seed: %d\n", *((int *) root_seed));
    user_curr = user_curr->prev;
    //mult_curr = mult_curr->next;
    oob_curr = oob_curr->next;
  }

  int ops[3] = { 0, 0, 0 };

  for (i = 0; i < atoi(argv[2]); i++) {
    ops[next_op(lbbt_multicast, users, add_wt, upd_wt, distrib, geo_param, max_id)]++;
  }

  printf("# adds: %d, # updates: %d, # rems %d\n", ops[0], ops[1], ops[2]);
  printf("# PRGs: %d, # encs: %d\n", *(lbbt_multicast->counts), *(lbbt_multicast->counts + 1));

  mult_destroy(lbbt_multicast);
  free(max_id);

  return 0;
}
