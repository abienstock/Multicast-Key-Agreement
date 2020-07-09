#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include "../group_manager/trees/trees.h"
#include "../ll/ll.h"
#include "../group_manager/multicast/multicast.h"
#include "../users/user.h"
#include "../crypto/crypto.h"
#include "../utils.h"

/*static void printSkeleton(void *p) {
  struct SkeletonNode *node = (struct SkeletonNode *) p;
  printf("id: %d, ", node->node_id);
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

static void printNode(void *p) {
  struct NodeData *data = (struct NodeData *) ((struct Node *) p)->data;
  if (data->blank == 1)
    printf("BLANK, id: %d", data->id);
  else {
    printf("id: %d, ", data->id);
    printf("key: %d, ", *((int *)data->key));
    printf("seed: %d.", *((int *)data->seed));
  }
}

static void print_secrets(void *data) {
  struct PathData *path_data = (struct PathData *) data;
  printf("id: %d, seed: %d, key %d\n", path_data->node_id, *((int *) path_data->seed), *((int *) path_data->key));
  }*/

/*
 * check that output key is same amongst manager and all of users
 */
void check_agreement(struct Multicast *multicast, struct List *users, int id, struct SkeletonNode *skeleton, struct List *oob_seeds, void *generator) {
  struct NodeData *root_data = (struct NodeData *) skeleton->node->data;
  void *mgr_key = malloc_check(multicast->prg_out_size);
  prg(generator, root_data->seed, mgr_key);  
  //printf("skeleton out key: %d\n", *((int *) mgr_key));

  struct ListNode *user_curr = users->head;
  struct ListNode *oob_curr = NULL;
  if (oob_seeds != NULL)
    oob_curr = oob_seeds->head;
  while (user_curr != 0) {
    struct User *user = (struct User *) user_curr->data;
    void *oob_seed = NULL;
    if (id == -1 || id == user->id) {
      oob_seed = oob_curr->data;
      oob_curr = oob_curr->next;      
    }
    //traverseList(user->secrets, print_secrets);
    void *user_key = proc_ct(user, id, skeleton, oob_seed, generator);
    //printf("user out key: %d\n", *((int *) user_key));
    //traverseList(user->secrets, print_secrets);
    assert(!memcmp(mgr_key, user_key, multicast->prg_out_size));
    free(user_key);
    user_curr = user_curr->next;
  }
  free(mgr_key);
  if (oob_seeds != NULL) {
    removeAllNodes(oob_seeds);
    free(oob_seeds);
  }
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
    return -1;
  }
}

/*
 * randomly determine the next operation
 */
int next_op(struct Multicast **mult_trees, struct List *users, int *max_id, float add_wt, float upd_wt, int distrib, float geo_param, int crypto, int crypto_tree, void *sampler, void *generator) {
  int op, id;
  struct SkeletonNode *skeleton;
  struct List *oob_seeds;
  int num_users = mult_trees[0]->users->len;
  float operation = (float) rand() / (float) RAND_MAX;
  int i;
  if (operation < add_wt || num_users == 1) { // force add with n=1
    (*max_id)++; //so adding new users w.l.o.g..
    printf("add: %d\n", *max_id);
    for (i = 0; i < 3; i++) { //TODO: no hardcode?
      struct MultAddRet add_ret = mult_add(mult_trees[i], *max_id, sampler, generator);
      if (crypto && i == crypto_tree) {
	struct User *user = init_user(*max_id, mult_trees[i]->prg_out_size, mult_trees[i]->seed_size);
	addAfter(users, users->tail, (void *) user);
	skeleton = add_ret.skeleton;
	oob_seeds = add_ret.oob_seeds;	
      } // TODO: free non-crypto skeletons here?
    }
    id = *max_id;
    op = 0;
  } else if (operation < add_wt + upd_wt) { // update
    int user_num = rand_int(num_users, distrib, geo_param); // user to update chosen w.r.t. time of addition to group
    for (i = 0; i < 3; i++) { //TODO: no hardcode
      struct MultUpdRet upd_ret = mult_update(mult_trees[i], user_num, sampler, generator);
      id = ((struct NodeData *) upd_ret.updated->data)->id;      
      if (crypto && i == crypto_tree) {
	skeleton = upd_ret.skeleton;
	oob_seeds = upd_ret.oob_seeds;
      }
    }
    printf("upd: %d\n", id);    
    op = 1;
  } else { // rem
    int user_num = rand_int(num_users, distrib, geo_param); // user to update chosen w.r.t. time of addition to group 
    printf("user_num: %d\n", user_num);
    for (i = 0; i < 3; i++) { //TODO: no hardcode?
      struct RemRet rem_ret = mult_rem(mult_trees[i], user_num, sampler, generator);
      id = rem_ret.id;      
      if (crypto && i  == crypto_tree) {
	struct User *user = (struct User *) findAndRemoveNode(users, user_num);
	free_user(user);
	skeleton = rem_ret.skeleton;
	oob_seeds = NULL;	
      }
    }
    printf("rem: %d\n", id);
    op = 2;
  }
  //pretty_traverse_tree(((struct LBBT *)multicast->tree)->root, 0, &printNode);
  //pretty_traverse_skeleton(add_ret.skeleton, 0, &printSkeleton);  
  if (crypto) {
    check_agreement(mult_trees[crypto_tree], users, id, skeleton, oob_seeds, generator);
  }
  /*if (skeleton->node == ((struct LBBT *) mult_trees[0]->tree)->root && skeleton->node->num_leaves == 1)
    free_skeleton(skeleton, 1, crypto);
  else
  free_skeleton(skeleton, 0, crypto);*/
  return op;
}

int main(int argc, char *argv[]) {
  int n, distrib;
  float add_wt, upd_wt, geo_param;
  void *sampler = NULL, *generator = NULL;

  srand(time(0));

  char *usage = "Usage: [init group size] [num ops] [add weight] [update weight] [LBBT addition strategy] [LBBT truncation strategy] [B-tree addition strategy] [B-tree degree] [operation distribution] [testing] [geometric distribution parameter] [testing tree] \n\nParameter Description:\n[init group size]: number of users in group at initialization\n[num ops]: number of group operations\n[add weight]: probability that a given operation will be an add (in [0,1])\n[update weight]: probability that a given operation will be an update (in [0,1]). Note: add weight + update weight must be in [0,1], remove weight = 1 - add weight - remove weight (probability that a given operation will be a remove)\n[LBBT addition strategy]: The strategy used to add users in an LBBT tree (ONLY 0 IMPLEMENTED) -- 0 = greedy, 1 = random, 2 = append, 3 = compare all\n[LBBT truncation strategy]: The strategy used to truncate the LBBT tree after removals (ONLY 0 IMPLEMENTED) -- 0 = truncate, 1 = keep, 2 = compare all\n[B-tree addition strategy]: The strategy used to add users in B trees (NOT IMPLEMENTED) -- 0 = greedy, 1 = random, 2 = compare both\n[B-tree degree]: Degree of the B-tree (NOT IMPLEMENTED) -- 3 = 3, 4 = 4, 0 = compare both\n[operation distribution]: The distribution for choosing which users to perform operations (and for the user to be removed, if the operation is a removal) -- 0 = uniform, 1 = geometric\n[testing]: whether or not to test actual key agreement -- 0 = no, 1 = yes\n[geometric distribution parameter]: The p parameter of the geometric distribution, if chosen\n[testing tree]: the tree to use for testing, if chosen -- 0 = LBBT, 1 = BTree, 2 = RB Tree\n";
  
  if (argc != 11 && argc != 12 && argc != 13)
    die_with_error(usage);


  n = atoi(argv[1]);
  distrib = atoi(argv[9]);
  add_wt = atof(argv[3]);
  upd_wt = atof(argv[4]);

  if(add_wt + upd_wt > 1 || add_wt < 0 || upd_wt < 0)
    die_with_error("Weights must not be negative nor sum to greater than 1");
  //rem_wt = 1 - add_wt - upd_wt;
  
  geo_param = 0;
  if (distrib) {
    if (argc != 12 && argc != 13)
      die_with_error("no geometric distribution parameter supplied");
    geo_param = atof(argv[11]);
    if(geo_param >= 1 || geo_param <=0)
      die_with_error("p has to be 0 < p < 1");
  } else if (argc == 13) {
    die_with_error(usage);
  }
  
  int lbbt_flags[2] = { atoi(argv[5]), atoi(argv[6]) };
  int btree_flags[2] = { atoi(argv[7]), atoi(argv[8]) };
  //YI: int rbtree_flags[2] = { argv[], argv[]};
  int max_id = n-1;

  int crypto_tree = -1;  
  int crypto = atoi(argv[10]);
  if (crypto) {
    if (distrib && argc != 13)
      die_with_error("crypto tree not specified\n");
    else if (!distrib && argc != 12)
      die_with_error("crypto tree not specified\n");      
    sampler_init(&sampler);
    prg_init(&generator);
    if (distrib)
      crypto_tree = atoi(argv[12]);
    else
      crypto_tree = atoi(argv[11]);  
  } else if ((distrib && argc == 13) || (!distrib && argc == 12)) {
    die_with_error(usage);
  }

  struct Multicast *mult_trees[3];
  struct MultInitRet lbbt_init_ret;
  if (crypto && crypto_tree == 0)
    lbbt_init_ret = mult_init(n, 1, lbbt_flags, 0, sampler, generator);
  else {
    lbbt_init_ret = mult_init(n, 0, lbbt_flags, 0, sampler, generator);
  }
  mult_trees[0] = lbbt_init_ret.multicast;
  //pretty_traverse_tree(((struct LBBT *)init_ret.multicast->tree)->root, 0, &printNode);
  //pretty_traverse_skeleton(init_ret.skeleton, 0, &printSkeleton);
  struct MultInitRet btree_init_ret;
  if (crypto && crypto_tree == 1)
    btree_init_ret = mult_init(n, 1, btree_flags, 1, sampler, generator);
  else {
    btree_init_ret = mult_init(n, 0, btree_flags, 1, sampler, generator);
  }
  mult_trees[1] = btree_init_ret.multicast;

  struct MultInitRet rbtree_init_ret;
  if (crypto && crypto_tree == 1)
    rbtree_init_ret = mult_init(n, 1, btree_flags, 2, sampler, generator);
  else {
    rbtree_init_ret = mult_init(n, 0, btree_flags, 2, sampler, generator);
  }
  mult_trees[2] = btree_init_ret.multicast;



  struct MultInitRet crypto_init_ret;  
  if (crypto) {
    switch(crypto_tree) {
    case 0: {
      crypto_init_ret = lbbt_init_ret;
      break;
    }
    case 1: {
      crypto_init_ret = btree_init_ret;
      break;
    }
    case 2: {
      crypto_init_ret = rbtree_init_ret;
      break;
    }
    }
  }
  
  int i; 
  struct List *users = NULL;
  struct Multicast *crypto_mult = NULL;
  if (crypto) { // for checking key agreement from crypto ops
    crypto_mult = mult_trees[crypto_tree];
    users = malloc_check(sizeof(struct List));
    initList(users);
    for (i = 0; i < n; i++) {
      struct User *user = init_user(i, crypto_init_ret.multicast->prg_out_size, crypto_init_ret.multicast->seed_size);
      addAfter(users, users->tail, (void *) user);
    }
    check_agreement(crypto_mult, users, -1, crypto_init_ret.skeleton, crypto_init_ret.oob_seeds, generator);
  }
  free_skeleton(lbbt_init_ret.skeleton, 0, crypto); //TODO: free all of them

  int ops[3] = { 0, 0, 0 };

  for (i = 0; i < atoi(argv[2]); i++) {
    ops[next_op(mult_trees, users, &max_id, add_wt, upd_wt, distrib, geo_param, crypto, crypto_tree, sampler, generator)]++;
  }

  printf("# adds: %d, # updates: %d, # rems %d\n", ops[0], ops[1], ops[2]);
  printf("lbbt: # PRGs: %d, # encs: %d\n", *(lbbt_init_ret.multicast->counts), *(lbbt_init_ret.multicast->counts + 1));
  printf("btree: # PRGs: %d, # encs: %d\n", *(btree_init_ret.multicast->counts), *(btree_init_ret.multicast->counts + 1));  

  free_mult(lbbt_init_ret.multicast);
  if (crypto) {
    free_users(users);
    free_sampler(sampler);
    free_prg(generator);
  }
  return 0;
}
