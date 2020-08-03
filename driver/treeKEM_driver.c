#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include "../group_manager/trees/trees.h"
#include "../ll/ll.h"
#include "../group_manager/treeKEM/treeKEM.h"
#include "../users/user.h"
#include "../crypto/crypto.h"
#include "../utils.h"

/*static void printSkeleton(void *p) {
  struct SkeletonNode *node = (struct SkeletonNode *) p;
  printf("id: %d, ", node->node_id);
  if (node->children_color == NULL)
    printf("no children");
  else {
    int i;
    for (i = 0; i < node->node->num_children; i++) {
      printf("child %d: %d, ", i, *(node->children_color + i));
      if (*(node->children_color + i) == 1) {
	struct Ciphertext *ct = *(node->ciphertexts + i);
	printf("ct: %d, parent id: %d, child id: %d, ", *((int *) ct->ct), node->node_id, ct->child_id);
      }
    }
  }
}

static void printIntLine(void *p) {
  struct NodeData *data = (struct NodeData *) ((struct Node *) p)->data;
  struct BTreeNodeData *btree_data = (struct BTreeNodeData *) data->tree_node_data;
  printf("id: %d, height: %d, opt add child: %d, lowest nonfull: %d, num_children: %d, num leaves: %d", data->id, btree_data->height, btree_data->opt_add_child, btree_data->lowest_nonfull, ((struct Node *) p)->num_children, ((struct Node *) p)->num_leaves);
}

static void printNode(void *p) {
  struct NodeData *data = (struct NodeData *) ((struct Node *) p)->data;
  //if (data->blank == 1)
  //printf("BLANK, id: %d", data->id);
  //else {
  printf("id: %d, ", data->id);
  //printf("key: %d, ", *((int *)data->key));
  //printf("seed: %d.", *((int *)data->seed));
  //}
}

static void print_secrets(void *data) {
  struct PathData *path_data = (struct PathData *) data;
  printf("id: %d, seed: %d, key %d\n", path_data->node_id, *((int *) path_data->seed), *((int *) path_data->key));
  }*/

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
 * check that output key is same amongst manager and all of users
 */
void check_agreement(struct treeKEM *treeKEM, struct List *users, int id, struct SkeletonNode *skeleton, void *generator) {
  struct NodeData *root_data = (struct NodeData *) skeleton->node->data;
  void *mgr_key = malloc_check(treeKEM->prg_out_size);
  prg(generator, root_data->seed, mgr_key);  
  //printf("skeleton out key: %d\n", *((int *) mgr_key));

  struct ListNode *user_curr = users->head;
  while (user_curr != 0) {
    struct User *user = (struct User *) user_curr->data;
    if (user->id == id)
      continue;
    //traverseList(user->secrets, print_secrets);
    void *user_key = proc_ct(user, id, skeleton, NULL, generator);
    //printf("user %d, out key: %d\n", user->id, *((int *) user_key));
    //traverseList(user->secrets, print_secrets);
    assert(!memcmp(mgr_key, user_key, treeKEM->prg_out_size));
    free(user_key);
    user_curr = user_curr->next;
  }
  free(mgr_key);
}

void commit_proposal(struct treeKEM **treeKEM_trees, int pre_max_id, struct List *users, int *op, int *max_id, int distrib, float geo_param, int crypto, int crypto_tree) {
  int num_users = treeKEM_trees[0]->users->len;  
  int i, id;
  if (*op == 0) { //add
    (*max_id)++; //so adding new users w.l.o.g..
    printf("add: %d\n", *max_id);
    for (i = 0; i < 3; i++) { //TODO: no hardcode?
      treeKEM_add(treeKEM_trees[i], *max_id);
      if (crypto && i == crypto_tree) {
	struct User *user = init_user(*max_id, treeKEM_trees[i]->prg_out_size, treeKEM_trees[i]->seed_size);
	addAfter(users, users->tail, (void *) user);
      } // TODO: free non-crypto skeletons here?
    }
    id = *max_id;
  } else if (*op == 1) { //update
    int user_num = rand_int(num_users - (*max_id - pre_max_id), distrib, geo_param); // user to update chosen w.r.t. time of addition to group; exclude newly added users before batch
    for (i = 0; i < 3; i++) //TODO: no hardcode
      id = treeKEM_update(treeKEM_trees[i], user_num);
    printf("upd: %d\n", id);
  } else { //remove
    int user_num = rand_int(num_users - (*max_id - pre_max_id), distrib, geo_param); // user to update chosen w.r.t. time of addition to group ; exclude newly added users before batch
    printf("user_num: %d\n", user_num);
    for (i = 0; i < 3; i++) { //TODO: no hardcode?
      id = treeKEM_rem(treeKEM_trees[i], user_num);
      if (crypto && i  == crypto_tree) {
	struct User *user = (struct User *) findAndRemoveNode(users, user_num);
	free_user(user);
      }
    }
    printf("rem: %d\n", id);
  }
}

void commit(struct treeKEM **treeKEM_trees, struct List *users, struct List *proposals, int *max_id, int distrib, float geo_param, int crypto, int crypto_tree, void *sampler, void *generator) {
  int pre_max_id = *max_id;
  struct ListNode *curr = proposals->head;
  while (curr != 0) {
    commit_proposal(treeKEM_trees, pre_max_id, users, curr->data, max_id, distrib, geo_param, crypto, crypto_tree);
    curr = curr->next;
  }
  int committer = rand_int(treeKEM_trees[0]->users->len - (*max_id - pre_max_id), distrib, geo_param); //exclude added users in batch
  //TODO: pick comitter differently
  int i;
  for (i = 0; i < 3; i++) { //TODO: no hardcode?
    struct SkeletonNode *commit_skel = treeKEM_commit(treeKEM_trees[i], committer, sampler, generator);
    if (crypto && i == crypto_tree)
      check_agreement(treeKEM_trees[i], users, committer, commit_skel, generator);
  }
}

int unif_sample(int left, int right) {
  int length = right - left + 1;
  return left + rand() % length;
}

/*
 * randomly determine the next batch and process it
 */
void next_batch(struct treeKEM **treeKEM_trees, struct List *proposals, int left_endpoint, int right_endpoint, float add_wt, float upd_wt) {
  int prev_num_users = treeKEM_trees[0]->users->len;
  int num_proposals = unif_sample(left_endpoint, right_endpoint);
  int i;
  for (i = 0; i < num_proposals; i++) {
    int *op = malloc_check(sizeof(int));
    //int id;
    //struct SkeletonNode *skeleton;
    //struct List *oob_seeds;
    float operation = (float) rand() / (float) RAND_MAX;
    //int i;
    if (operation < add_wt) // force add with n=1
      *op = 0;
    else if (operation < add_wt + upd_wt || prev_num_users == 1) // update
      *op = 1;
    else { // rem
      *op = 2;
      prev_num_users--;
    }
    addAfter(proposals, proposals->tail, op);
  }
}

int main(int argc, char *argv[]) {
  int n, distrib, left_endpoint, right_endpoint;
  float add_wt, upd_wt, geo_param;
  void *sampler = NULL, *generator = NULL;

  srand(time(0));

  char *usage = "Usage: [init group size] [num batches] [# proposals per batch left endpoint] [# proposals per batch right endpoint] [add weight] [update weight] [LBBT addition strategy] [LBBT truncation strategy] [B-tree addition strategy] [B-tree degree] [RB Tree addition strategy] [RB Tree mode] [operation distribution] [testing] [geometric distribution parameter] [testing tree] \n\nParameter Description:\n[init group size]: number of users in group at initialization\n[num batches]: number of group operations\n[# proposals per batch left endpoint]: Left endpoint for uniform distribution on # of propsals per batch. Must be greater than 0.\n[# proposals per batch right endpoint]: Right endpoint for uniform distribution on # of proposals per batch. Must be greater than left endpoint.\n[add weight]: probability that a given operation will be an add (in [0,1])\n[update weight]: probability that a given operation will be an update (in [0,1]). Note: add weight + update weight must be in [0,1], remove weight = 1 - add weight - remove weight (probability that a given operation will be a remove)\n[LBBT addition strategy]: The strategy used to add users in an LBBT tree (ONLY 0 IMPLEMENTED) -- 0 = greedy, 1 = random, 2 = append, 3 = compare all\n[LBBT truncation strategy]: The strategy used to truncate the LBBT tree after removals (ONLY 0 IMPLEMENTED) -- 0 = truncate, 1 = keep, 2 = compare all\n[B-tree addition strategy]: The strategy used to add users in B trees (ONLY 0 IMPLEMENTED) -- 0 = greedy, 1 = random, 2 = compare both\n[B-tree degree]: Degree of the B-tree -- 3 = 3, 4 = 4, 0 = compare both\n[RB Tree addition strategy]: The strategy used to add users in RB trees (ONLY 0 IMPLEMENTED) -- 0 = greedy, 1 = random, 2 = compare both\n[RB Tree mode]: RB Tree mode -- 3 = 2-3 mode, 4 = 2-3-4 mode\n[operation distribution]: The distribution for choosing which users to perform operations (and for the user to be removed, if the operation is a removal) -- 0 = uniform, 1 = geometric\n[testing]: whether or not to test actual key agreement -- 0 = no, 1 = yes\n[geometric distribution parameter]: The p parameter of the geometric distribution, if chosen\n[testing tree]: the tree to use for testing, if chosen -- 0 = LBBT, 1 = BTree, 2 = RB Tree\n";
  
  if (argc != 15 && argc != 16 && argc != 17)
    die_with_error(usage);


  n = atoi(argv[1]);
  left_endpoint = atoi(argv[3]);
  right_endpoint= atoi(argv[4]);
  if (left_endpoint <= 0 || left_endpoint > right_endpoint)
    die_with_error("Improper endpoints for # proposals per batch");
  distrib = atoi(argv[13]);
  
  add_wt = atof(argv[5]);
  upd_wt = atof(argv[6]);

  if(add_wt + upd_wt > 1 || add_wt < 0 || upd_wt < 0)
    die_with_error("Weights must not be negative nor sum to greater than 1");
  //rem_wt = 1 - add_wt - upd_wt;
  
  geo_param = 0;
  if (distrib) {
    if (argc != 16 && argc != 17)
      die_with_error("no geometric distribution parameter supplied");
    geo_param = atof(argv[15]);
    if(geo_param >= 1 || geo_param <=0)
      die_with_error("p has to be 0 < p < 1");
  } else if (argc == 17) {
    die_with_error(usage);
  }
  
  int lbbt_flags[2] = { atoi(argv[7]), atoi(argv[8]) };
  int btree_flags[2] = { atoi(argv[9]), atoi(argv[10]) };
  int rbtree_flags[2] = { atoi(argv[11]), atoi(argv[12]) };
  int max_id = n-1;

  int crypto_tree = -1;  
  int crypto = atoi(argv[14]);
  if (crypto) {
    if (distrib && argc != 17)
      die_with_error("crypto tree not specified\n");
    else if (!distrib && argc != 16)
      die_with_error("crypto tree not specified\n");      
    sampler_init(&sampler);
    prg_init(&generator);
    if (distrib)
      crypto_tree = atoi(argv[16]);
    else
      crypto_tree = atoi(argv[15]);  
  } else if ((distrib && argc == 17) || (!distrib && argc == 16)) {
    die_with_error(usage);
  }

  struct treeKEM *treeKEM_trees[3];
  struct treeKEMInitRet lbbt_init_ret;
  if (crypto && crypto_tree == 0)
    lbbt_init_ret = treeKEM_init(n, 1, lbbt_flags, 0, sampler, generator);
  else {
    lbbt_init_ret = treeKEM_init(n, 0, lbbt_flags, 0, sampler, generator);
  }
  treeKEM_trees[0] = lbbt_init_ret.treeKEM;
  struct treeKEMInitRet btree_init_ret;
  if (crypto && crypto_tree == 1)
    btree_init_ret = treeKEM_init(n, 1, btree_flags, 1, sampler, generator);
  else {
    btree_init_ret = treeKEM_init(n, 0, btree_flags, 1, sampler, generator);
  }
  treeKEM_trees[1] = btree_init_ret.treeKEM;
  //pretty_traverse_tree(btree_init_ret.treeKEM->tree, ((struct BTree *)btree_init_ret.treeKEM->tree)->root, 0, &printIntLine);
  //pretty_traverse_skeleton(btree_init_ret.skeleton, 0, &printSkeleton);

  struct treeKEMInitRet rbtree_init_ret;
  if (crypto && crypto_tree == 2)
    rbtree_init_ret = treeKEM_init(n, 1, rbtree_flags, 2, sampler, generator);
  else {
    rbtree_init_ret = treeKEM_init(n, 0, rbtree_flags, 2, sampler, generator);
  }
  treeKEM_trees[2] = rbtree_init_ret.treeKEM;

  struct treeKEMInitRet crypto_init_ret;
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
  struct treeKEM *crypto_treeKEM = NULL;
  if (crypto) { // for checking key agreement from crypto ops
    crypto_treeKEM = treeKEM_trees[crypto_tree];
    users = malloc_check(sizeof(struct List));
    initList(users);
    for (i = 0; i < n; i++) {
      struct User *user = init_user(i, crypto_treeKEM->prg_out_size, crypto_treeKEM->seed_size);
      addAfter(users, users->tail, (void *) user);
    }
    check_agreement(crypto_treeKEM, users, -1, crypto_init_ret.skeleton, generator);    
  }
  //free_skeleton(lbbt_init_ret.skeleton, 0, crypto); //TODO: free all of them

  //int ops[3] = { 0, 0, 0 };

  for (i = 0; i < atoi(argv[2]); i++) {
    struct List proposals;
    initList(&proposals);
    next_batch(treeKEM_trees, &proposals, left_endpoint, right_endpoint, add_wt, upd_wt);
    commit(treeKEM_trees, users, &proposals, &max_id, distrib, geo_param, crypto, crypto_tree, sampler, generator);
  }

  //printf("# adds: %d, # updates: %d, # rems %d\n", ops[0], ops[1], ops[2]);
  printf("lbbt: # PRGs: %d, # encs: %d\n", *(lbbt_init_ret.treeKEM->counts), *(lbbt_init_ret.treeKEM->counts + 1));
  printf("btree: # PRGs: %d, # encs: %d\n", *(btree_init_ret.treeKEM->counts), *(btree_init_ret.treeKEM->counts + 1));
  printf("rbtree: # PRGs: %d, # encs: %d\n", *(rbtree_init_ret.treeKEM->counts), *(rbtree_init_ret.treeKEM->counts + 1));    

  free_treeKEM(lbbt_init_ret.treeKEM);
  if (crypto) {
    free_users(users);
    free_sampler(sampler);
    free_prg(generator);
  }
  return 0;
}
