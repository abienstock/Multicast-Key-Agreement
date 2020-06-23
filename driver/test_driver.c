#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "../group_manager/trees/trees.h"
#include "../ll/ll.h"
#include "../group_manager/multicast/multicast.h"
#include "../users/user.h"
#include "../crypto/crypto.h"

/*static void printSkeleton(void *p)
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
    //printf("key: %d, ", *((int *)data->key));
    //printf("seed: %d.", *((int *)data->seed));
  }
}


static void print_secrets(void *data) {
  struct PathData *path_data = (struct PathData *) data;
  printf("id: %d, seed: %d, key %d\n", path_data->node_id, *((int *) path_data->seed), *((int *) path_data->key));
  }*/

void check_agreement(struct Multicast *multicast, struct List *users, int id, struct SkeletonNode *skeleton, struct List *oob_seeds, void *generator) {
  struct NodeData *root_data = (struct NodeData *) skeleton->node->data;
  void *mgr_key = malloc(multicast->prg_out_size);
  if (mgr_key == NULL) {
    perror("malloc returned NULL");
  }
  prg(generator, root_data->seed, mgr_key);  
  //printf("skeleton root seed: %d\n", *((int *) int_prg(root_data->seed)));

  struct ListNode *user_curr = users->head;
  struct ListNode *oob_curr;
  if (oob_seeds != NULL)
    oob_curr = oob_seeds->head;
  while (user_curr != 0) {
    struct User *user = (struct User *) user_curr->data;
    void *oob_seed = NULL;
    if (id == -1 || id == user->id) {
      oob_seed = oob_curr->data;
      oob_curr = oob_curr->next;      
    }
    void *key = proc_ct(user, id, skeleton, oob_seed, generator);
    free(key);
    user_curr = user_curr->next;
  }
  removeAllNodes(oob_seeds);
  free(oob_seeds);
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

int next_op(struct Multicast *multicast, struct List *users, float add_wt, float upd_wt, int distrib, float geo_param, int *max_id, void *generator) {
  int op, id;
  struct SkeletonNode *skeleton;
  struct List *oob_seeds;
  int num_users = multicast->users->len;
  float operation = (float) rand() / (float) RAND_MAX;
  printf("num users: %d\n", num_users);
  if (operation < add_wt || num_users == 1) { // add TODO: forcing add with n=1 correct??
    (*max_id)++; //so adding new users w.l.o.g.
    printf("add: %d\n", *max_id);
    struct MultAddRet add_ret = mult_add(multicast, *max_id, NULL, NULL);
    //pretty_traverse_tree(((struct LBBT *)multicast->tree)->root, 0, &printNode);
    //pretty_traverse_skeleton(add_ret.skeleton, 0, &printSkeleton);
    if (multicast->crypto) {
      struct User *user = init_user(*max_id);
      addAfter(users, users->tail, (void *) user);
    }
    id = *max_id;
    skeleton = add_ret.skeleton;
    oob_seeds = add_ret.oob_seeds;
    op = 0;
  } else if (operation < add_wt + upd_wt) { // update
    int user_num = rand_int(num_users, distrib, geo_param);
    struct MultUpdRet upd_ret = mult_update(multicast, user_num, NULL, NULL);
    //pretty_traverse_tree(((struct LBBT *)multicast->tree)->root, 0, &printNode);
    //pretty_traverse_skeleton(upd_ret.skeleton, 0, &printSkeleton);
    id = ((struct NodeData *) upd_ret.updated->data)->id;
    printf("upd: %d\n", id);    
    skeleton = upd_ret.skeleton;
    oob_seeds = upd_ret.oob_seeds;
    op = 1;
  } else { // rem
    int user_num = rand_int(num_users, distrib, geo_param);    
    printf("user_num: %d\n", user_num);
    struct RemRet rem_ret = mult_rem(multicast, user_num, NULL, NULL);
    //pretty_traverse_tree(((struct LBBT *)multicast->tree)->root, 0, &printNode);
    //pretty_traverse_skeleton(rem_ret.skeleton, 0, &printSkeleton);
    if (multicast->crypto) {
      struct User *user = (struct User *) findAndRemoveNode(users, user_num);
      destroy_user(user);
    }
    id = ((struct NodeData *) rem_ret.data)->id;
    printf("rem: %d\n", id);      
    skeleton = rem_ret.skeleton;
    oob_seeds = NULL;
    op = 2;
  }
  if (multicast->crypto) {
    check_agreement(multicast, users, id, skeleton, oob_seeds, generator);
  }
  destroy_skeleton(skeleton);  
  return op;
}

int main(int argc, char *argv[]) {
  int n, distrib;
  float add_wt, upd_wt, geo_param;
  void *sampler = NULL, *generator = NULL;

  srand(time(0));

  if (argc != 11 && argc != 12) {
    printf("Usage: [testing] [init group size] [num ops] [add weight] [update weight] [LBBT addition strategy] [LBBT truncation strategy] [B-tree addition strategy] [B-tree degree] [operation distribution] [geometric distribution parameter] \n\nParameter Description:\n[crypto]: whether or not to test actual key agreement -- 0 = no, 1 = yes\n[init group size]: number of users in group at initialization\n[num ops]: number of group operations\n[add weight]: probability that a given operation will be an add (in [0,1])\n[update weight]: probability that a given operation will be an update (in [0,1]). Note: add weight + update weight must be in [0,1], remove weight = 1 - add weight - remove weight (probability that a given operation will be a remove)\n[LBBT addition strategy]: The strategy used to add users in an LBBT tree -- 0 = greedy, 1 = random, 2 = append, 3 = compare all\n[LBBT truncation strategy]: The strategy used to truncate the LBBT tree after removals -- 0 = truncate, 1 = keep, 2 = compare all\n[B-tree addition strategy]: The strategy used to add users in B trees -- 0 = greedy, 1 = random, 2 = compare both\n[B-tree degree]: Degree of the B-tree -- 3 = 3, 4 = 4, 0 = compare both\n[operation distribution]: The distribution for choosing which users to perform operations (and for the user to be removed, if the operation is a removal) -- 0 = uniform, 1 = geometric\n[geometric distribution parameter]: The p parameter of the geometric distribution, if chosen\n");
    exit(0);
  }

  n = atoi(argv[2]);
  distrib = atoi(argv[10]);
  add_wt = atof(argv[4]);
  upd_wt = atof(argv[5]);

  if(add_wt + upd_wt > 1 || add_wt < 0 || upd_wt < 0) {
    perror("Weights must not be negative nor sum to greater than 1");
    exit(1);
  }
  //rem_wt = 1 - add_wt - upd_wt;
  
  geo_param = 0;
  if(distrib == 1) {
    geo_param = atof(argv[11]);
    if(geo_param >= 1 || geo_param <=0) {
      perror("p has to be 0 < p < 1");
      exit(1);
    }
  }
  
  int lbbt_flags[2] = { atoi(argv[6]), atoi(argv[7]) };

  int *max_id = malloc(sizeof(int));
  if (max_id == NULL) {
    perror("malloc returned NULL");
    return -1;
  }
  *max_id = n-1;
  
  int crypto = atoi(argv[1]);
  struct List *users = NULL;
  int i;  
  if (crypto) {
    sampler_init(&sampler);
    prg_init(&generator);

    users = malloc(sizeof(struct List));
    if (users == NULL) {
      perror("malloc returned NULL");
      return -1;
    }
    initList(users);
    
    for (i = 0; i < n; i++) {
      struct User *user = init_user(i);
      addAfter(users, users->tail, (void *) user);
    }
  }
  struct MultInitRet init_ret = mult_init(n, crypto, lbbt_flags, 0, sampler, generator);
  struct Multicast *lbbt_multicast = init_ret.multicast;
  struct SkeletonNode *skeleton = init_ret.skeleton;
  struct List *oob_seeds = init_ret.oob_seeds;

  if (crypto)
    check_agreement(lbbt_multicast, users, -1, skeleton, oob_seeds, generator);
  destroy_skeleton(skeleton);

  int ops[3] = { 0, 0, 0 };

  for (i = 0; i < atoi(argv[3]); i++) {
    ops[next_op(lbbt_multicast, users, add_wt, upd_wt, distrib, geo_param, max_id, generator)]++;
  }

  printf("# adds: %d, # updates: %d, # rems %d\n", ops[0], ops[1], ops[2]);
  printf("# PRGs: %d, # encs: %d\n", *(lbbt_multicast->counts), *(lbbt_multicast->counts + 1));

  mult_destroy(lbbt_multicast);
  free(max_id);
  return 0;
}
