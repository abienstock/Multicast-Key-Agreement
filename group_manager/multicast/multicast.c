#include <stdio.h>
#include <stdlib.h>
#include "../trees/trees.h"
#include "multicast.h"
#include "../../skeleton.h"
#include "../../crypto/crypto.h"
#include "../../utils.h"

/*
 * create ciphertexts for a node in a skeleton generated by an op, skeleton_node
 */
int ct_gen(struct Multicast *multicast, struct SkeletonNode *skeleton_node, void *seed, void *generator) {
  if (skeleton_node->children_color != NULL) {
    struct Ciphertext **cts = NULL;
    if (multicast->crypto) {
      cts = malloc_check(sizeof(struct Ciphertext *) * skeleton_node->node->num_children);
      skeleton_node->ciphertexts = cts;
    }
    int i;
    for (i = 0; i < skeleton_node->node->num_children; i++) {
      struct Ciphertext *ct_struct = NULL;
      if (*(skeleton_node->children_color + i) == 1) {
	(*(multicast->counts))++; // one-time pad refresh      
	(*(multicast->counts + 1))++;
	if (multicast->crypto) {
	  ct_struct = malloc_check(sizeof(struct Ciphertext));
	  void *ct = malloc_check(multicast->seed_size);
	  struct NodeData *data = (struct NodeData *) (*(skeleton_node->node->children + i))->data;
	  ct_struct->child_id = data->id;
	  enc(generator, data->key, data->seed, seed, ct, multicast->seed_size);
	  ct_struct->ct = ct;
	  *cts++ = ct_struct;	  
	}
      } else if (multicast->crypto) 
	*cts++ = ct_struct;
    }
  } else if (((struct LBBT *) multicast->tree)->root == skeleton_node->node) {
    (*(multicast->counts))++; // one-time pad refresh
    (*(multicast->counts + 1))++;
    if (multicast->crypto) {
      struct Ciphertext **cts = malloc_check(sizeof(struct Ciphertext *));
      struct Ciphertext *ct_struct = malloc_check(sizeof(struct Ciphertext));
      void *ct = malloc_check(multicast->seed_size);
      struct NodeData *root_data = (struct NodeData *) skeleton_node->node->data;
      ct_struct->child_id = root_data->id;
      enc(generator, root_data->key, root_data->seed, seed, ct, multicast->seed_size);
      ct_struct->ct = ct;
      *cts = ct_struct;
      skeleton_node->ciphertexts = cts;
    }
  }
  return 0;
}

/*
 * recursively generates the secrets in a skeleton for a MKA op and the corresponding ciphertexts
 */
void *secret_gen(struct Multicast *multicast, struct SkeletonNode *skeleton, struct List *oob_seeds, void *sampler, void *generator) {
  void *prev_seed = NULL;
  void *next_seed = NULL;
  if (skeleton->children_color != NULL) {
    int i, no_prg_edge = 1;
    for (i = 0; i < skeleton->node->num_children; i++) {
      if (*(skeleton->children_color + i) == 0) {
	prev_seed = secret_gen(multicast, *(skeleton->children + i), oob_seeds, sampler, generator);
	no_prg_edge = 0;
      }
      else if (skeleton->children != NULL && *(skeleton->children + i) != NULL)
	  free(secret_gen(multicast, *(skeleton->children + i), oob_seeds, sampler, generator));
    }
    if (no_prg_edge && multicast->crypto) {
      prev_seed = malloc_check(multicast->seed_size);
      sample(sampler, prev_seed);
    }
  } else if (multicast->crypto) { // leaf node
    prev_seed = malloc_check(multicast->seed_size);
    sample(sampler, prev_seed);
    if (skeleton->node->num_leaves == 1 && skeleton->parent != NULL) // if node is a leaf that is not the root
      addAfter(oob_seeds, oob_seeds->tail, prev_seed);
  }

  ct_gen(multicast, skeleton, prev_seed, generator);
  (*(multicast->counts))++;  
  struct NodeData *data = (struct NodeData *) skeleton->node->data;  
  if (multicast->crypto) {
    void *seed = NULL, *key = NULL, *out = NULL;
    alloc_prg_out(&out, &seed, &key, &next_seed, multicast->prg_out_size, multicast->seed_size);
    prg(generator, prev_seed, out);
    split(out, seed, key, next_seed, multicast->seed_size);
    free(out);
    //printf("seed: %s\n", (char *) prev_seed);
    
    if (data->key != NULL)
      free(data->key);
      if (data->seed != NULL)
      free(data->seed);
    data->key = key;
    data->seed = seed;
  }

  if (skeleton->node->num_leaves > 1 || skeleton->parent == NULL)
    free(prev_seed); // need to free here if not oob_seed
  
  return next_seed;
}

/*
 * initialize group with n users for tree type specified by tree_type
 */
struct MultInitRet mult_init(int n, int crypto, int *tree_flags, int tree_type, void *sampler, void *generator) {
  struct MultInitRet ret = { NULL, NULL, NULL };
  struct List *users = malloc_check(sizeof(struct List));
  initList(users);
  
  int ids[n];
  int i;
  for (i = 0; i < n; i++) {
    ids[i] = i;
  }

  int *counts = malloc_check(sizeof(int) * 2);
  *counts++ = 0;
  *counts-- = 0;

  void *tree = NULL;
  struct InitRet tree_ret = { NULL, NULL };
  if (tree_type == 0) {
    tree_ret = lbbt_init(ids, n, *tree_flags, *(tree_flags + 1), users);
    tree = tree_ret.tree;
  } else if (tree_type == 1) {
    tree_ret = btree_init(ids, n, *tree_flags, *(tree_flags + 1), users);
    tree = tree_ret.tree;
  } /*else if (tree_type == 2) {
    tree_ret = rbtree_init(ids, n, *tree_flags, *(tree_flags + 1), users);
    tree = tree_ret.tree;
    }*/
  
  struct List *oob_seeds = NULL;
  size_t prg_out_size = 0, seed_size = 0;
  if (crypto) {
    oob_seeds = malloc_check(sizeof(struct List));
    initList(oob_seeds);
    get_prg_out_size(generator, &prg_out_size);
    get_seed_size(generator, &seed_size);
  }

  struct Multicast *multicast_ptr = malloc_check(sizeof(struct Multicast));
  struct Multicast multicast = { 1, users, tree, counts, tree_type, crypto, prg_out_size, seed_size };
  *multicast_ptr = multicast;
  ret.multicast = multicast_ptr;
  ret.skeleton = tree_ret.skeleton;
  free(secret_gen(multicast_ptr, ret.skeleton, oob_seeds, sampler, generator));
  ret.oob_seeds = oob_seeds;
  return ret;
}

/*
 * add id to the group
 */
struct MultAddRet mult_add(struct Multicast *multicast, int id, void *sampler, void *generator) {
  struct MultAddRet ret = { NULL, NULL, NULL };
  struct AddRet add_ret = { NULL, NULL };
  if (multicast->tree_type == 0)
    add_ret = lbbt_add(multicast->tree, id);
  else if (multicast->tree_type == 1)
    add_ret = btree_add(multicast->tree, id); // TODO: bring back gen funcs!
    //add_ret = gen_tree_add(multicast->tree, data, &btree_add);

  addAfter(multicast->users, multicast->users->tail, (void *) add_ret.added);
  ret.added = add_ret.added;
  ret.skeleton = add_ret.skeleton;
  struct List *oob_seeds = NULL;  
  if (multicast->crypto) {  
    oob_seeds = malloc_check(sizeof(struct List));
    initList(oob_seeds);
    ret.oob_seeds = oob_seeds;
  }
  free(secret_gen(multicast, ret.skeleton, oob_seeds, sampler, generator));
  return ret;
}

/*
 * creates skeleton for updates consisting of direct path with prgs along path cts to copath
 */
struct SkeletonNode *gen_upd_skel(struct Node *node, struct Node *child, struct SkeletonNode *child_skel) {
  if (node != NULL) {
    struct SkeletonNode *skeleton = malloc_check(sizeof(struct SkeletonNode));
    skeleton->node_id = ((struct NodeData *) node->data)->id;
    skeleton->node = node;
    skeleton->ciphertexts = NULL;

    int i, child_pos = 0;
    if (child != NULL) {
      int *children_color = malloc_check(sizeof(int) * node->num_children);
      for (i = 0; i < node->num_children; i++) {
	if (child == *(node->children + i)) {
	  child_pos = i;
	  *(children_color + i) = 0;
	} else
	  *(children_color + i) = 1;
      }
      skeleton->children_color = children_color;      
    } else
      skeleton->children_color = NULL;
    
    if (child_skel != NULL) {
      struct SkeletonNode **skel_children = malloc_check(sizeof(struct skeletonNode *) * node->num_children);
      for (i = 0; i < node->num_children; i++)
	*(skel_children + i) = NULL;
      *(skel_children + child_pos) = child_skel;
      skeleton->children = skel_children;
      child_skel->parent = skeleton;      
    } else
      skeleton->children = NULL;

    return gen_upd_skel(node->parent, node, skeleton);
  }
  return child_skel;
}

/*
 * update the id that was the user-th added id of the group
 */
struct MultUpdRet mult_update(struct Multicast *multicast, int user, void *sampler, void *generator) { 
  struct MultUpdRet ret = { NULL, NULL, NULL };
  struct Node *user_node = (struct Node *) findNode(multicast->users, user)->data;
  ret.updated = user_node;
  struct SkeletonNode *skeleton = gen_upd_skel(user_node, NULL, NULL);
  skeleton->parent = NULL;
  ret.skeleton = skeleton;

  struct List *oob_seeds = NULL;  
  if (multicast->crypto) {
    oob_seeds = malloc_check(sizeof(struct List));
    initList(oob_seeds);
    ret.oob_seeds = oob_seeds;
  }
  free(secret_gen(multicast, skeleton, oob_seeds, sampler, generator));

  return ret;
}

/*
 * remove the id that was the user-th added id of the group
 */
struct RemRet mult_rem(struct Multicast *multicast, int user, void *sampler, void *generator) { 
  struct RemRet ret = { -1, NULL };
  struct Node *user_node = (struct Node *) findAndRemoveNode(multicast->users, user);
  if (multicast->tree_type == 0)
    ret = lbbt_rem(multicast->tree, user_node);
  else if (multicast->tree_type == 1)
    ret = btree_rem(multicast->tree, user_node);
    //ret = gen_tree_rem(multicast->tree, user, &btree_rem);

  free(secret_gen(multicast, ret.skeleton, NULL, sampler, generator));
  return ret;
}

void free_mult(struct Multicast *multicast) {
  free_tree(((struct LBBT *) multicast->tree)->root);
  removeAllNodes(((struct LBBT *) multicast->tree)->blanks);
  free(((struct LBBT *) multicast->tree)->blanks);
  free(multicast->tree);
  removeAllNodes(multicast->users);
  free(multicast->users);
  free(multicast->counts);
  free(multicast);
}
