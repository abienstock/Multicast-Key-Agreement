#include <stdio.h>
#include <stdlib.h>
#include <string.h> // for memcpy
#include "../trees/trees.h"
#include "multicast.h"
#include "../../skeleton.h"
#include "../../crypto/crypto.h"

int split(uint8_t *out, uint8_t *seed, uint8_t *key, uint8_t *next_seed, size_t seed_size) {
  uint8_t *out_bytes = (uint8_t *) out;
  uint8_t *seed_bytes = (uint8_t *) seed;
  uint8_t *key_bytes = (uint8_t *) key;
  uint8_t *next_seed_bytes = (uint8_t *) next_seed;
  memcpy(seed_bytes, out_bytes, seed_size);
  memcpy(key_bytes, out_bytes + seed_size, seed_size);
  memcpy(next_seed_bytes, out_bytes + 2 * seed_size, seed_size);
  return 0;
}

int ct_gen(struct Multicast *multicast, struct SkeletonNode *skeleton, struct NodeData *data, uint8_t *seed, void *generator) {
  if (skeleton->children_color != NULL) {
    struct Ciphertext *left_ct = NULL, *right_ct = NULL;
    if (*(skeleton->children_color) == 1) {
      (*(multicast->counts + 1))++;
      if (!multicast->testing) {
	left_ct = malloc(sizeof(struct Ciphertext));
	if (left_ct == NULL) {
	  perror("malloc returned NULL");
	  return -1;
	}
	uint8_t *ct = malloc(multicast->seed_size);
	if (ct == NULL) {
	  perror("malloc returned NULL");
	  return -1;
	}
	struct NodeData *left_data = (struct NodeData *) (*(skeleton->node->children))->data;
	left_ct->parent_id = data->id;
	left_ct->child_id = left_data->id;
	enc(generator, left_data->key, left_data->seed, seed, ct, multicast->seed_size);
	left_ct->ct = ct;
	left_ct->num_bytes = multicast->seed_size;
      }
    }
    if (*(skeleton->children_color + 1) == 1) {
      (*(multicast->counts + 1))++;
      if (!multicast->testing) {
	right_ct = malloc(sizeof(struct Ciphertext));
	if (right_ct == NULL) {
	  perror("malloc returned NULL");
	  return -1;
	}
	uint8_t *ct = malloc(multicast->seed_size);
	if (ct == NULL) {
	  perror("malloc returned NULL");
	  return -1;
	}
	struct NodeData *right_data = (struct NodeData *) (*(skeleton->node->children + 1))->data;
	right_ct->parent_id = data->id;
	right_ct->child_id = right_data->id;
	enc(generator, right_data->key, right_data->seed, seed, ct, multicast->seed_size);
	right_ct->ct = ct;
	right_ct->num_bytes = multicast->seed_size;
      }
    }
    if (!multicast->testing) {
      struct Ciphertext **cts = malloc(sizeof(struct Ciphertext *) * 2);
      if (cts == NULL) {
	perror("malloc returned NULL");
	return -1;
      }    
      *cts++ = left_ct;
      *cts-- = right_ct;
      skeleton->ciphertexts = cts;
    }
  } else if (((struct LBBT *) multicast->tree)->root == skeleton->node) {
    (*(multicast->counts + 1))++;
    if (!multicast->testing) {
      struct Ciphertext **cts = malloc(sizeof(struct Ciphertext *) * 2);
      if (cts == NULL) {
	perror("malloc returned NULL");
	return -1;
      }
      struct Ciphertext *left_ct = malloc(sizeof(struct Ciphertext));
      if (left_ct == NULL) {
	perror("malloc returned NULL");
	return -1;
      }
      uint8_t *ct = malloc(multicast->seed_size);
      if (ct == NULL) {
	perror("malloc returned NULL");
	return -1;
      }          
      left_ct->parent_id = -1; // TODO: make sure IDs are never negative!
      struct NodeData *root_data = (struct NodeData *) skeleton->node->data;
      left_ct->child_id = root_data->id;
      enc(generator, root_data->key, root_data->seed, seed, ct, multicast->seed_size);
      left_ct->ct = ct;
      left_ct->num_bytes = multicast->seed_size;          
      *cts++ = left_ct;
      *cts-- = NULL;
      skeleton->ciphertexts = cts;
    }
  }
  return 0;
}

//TODO: make sure this generalizes to any multicast scheme
uint8_t *secret_gen(struct Multicast *multicast, struct SkeletonNode *skeleton, struct List *oob_seeds, void *sampler, void *generator) {
  uint8_t *prev_seed = NULL;
  uint8_t *next_seed = NULL;
  if (skeleton->children_color != NULL) {
    if (*(skeleton->children_color) == 0) {
      prev_seed = secret_gen(multicast, *(skeleton->children), oob_seeds, sampler, generator);
      if (skeleton->children != NULL && *(skeleton->children + 1) != NULL)
	secret_gen(multicast, *(skeleton->children + 1), oob_seeds, sampler, generator);
    } else if (*(skeleton->children_color + 1) == 0) {
      prev_seed = secret_gen(multicast, *(skeleton->children + 1), oob_seeds, sampler, generator);
      if (skeleton->children != NULL && *(skeleton->children) != NULL)
	secret_gen(multicast, *(skeleton->children), oob_seeds, sampler, generator);
    } else if (!multicast->testing) {
      prev_seed = malloc(sizeof(uint8_t) * multicast->seed_size);
      if (prev_seed == NULL) {
	perror("malloc returned NULL");
	return NULL;
      }
      sample(sampler, prev_seed);
    }
  } else if (!multicast->testing) {
    prev_seed = malloc(sizeof(uint8_t) * multicast->seed_size);
    if (prev_seed == NULL) {
      perror("malloc returned NULL");
      return NULL;
    }
    sample(sampler, prev_seed);
    if (skeleton->node->num_leaves == 1 && skeleton->parent != NULL) // if node is a leaf that is not the root
      addFront(oob_seeds, prev_seed);
  }

  *multicast->counts++;

  struct NodeData *data = (struct NodeData *) skeleton->node->data;  
  if (!multicast->testing) {
    next_seed = malloc(multicast->seed_size);
    if (next_seed == NULL) {
      perror("malloc returned NULL");
      return NULL;
    }
    uint8_t *seed = malloc(multicast->seed_size);
    if (seed == NULL) {
      perror("malloc returned NULL");
      return NULL;
    }
    uint8_t *key = malloc(multicast->seed_size);
    if (key == NULL) {
      perror("malloc returned NULL");
      return NULL;
    }
    uint8_t *out = malloc(multicast->prg_out_size);
    if (out == NULL) {
      perror("malloc returned NULL");
      return NULL;
    }
    
    prg(generator, prev_seed, out);
    split(out, seed, key, next_seed, multicast->seed_size);
    //printf("seed: %s\n", (char *) prev_seed);
    //free(out);
    
    /*if (data->key != NULL)
      free(data->key);
      if (data->nonce != NULL)
      free(data->nonce);
      if (data->seed != NULL)
      free(data->seed);*/  
    
    data->key = key;
    data->seed = seed;
  }
  
  ct_gen(multicast, skeleton, data, prev_seed, generator);
  //if (skeleton->node->num_leaves > 1 || skeleton->parent == NULL)
  //free(prev_seed); // need to do it here if not oob_seed
  
  return next_seed;
}

struct MultInitRet mult_init(int n, int testing, int *tree_flags, int tree_type, void *sampler, void *generator) {
  struct MultInitRet ret = { NULL, NULL, NULL };
  
  struct List *users = malloc(sizeof(struct List));
  if (users == NULL) {
    perror("malloc returned NULL");
    return ret;
  }
  initList(users);
  
  int ids[n];
  int i;
  for (i = 0; i < n; i++) {
    ids[i] = i;
  }

  int *counts = malloc(sizeof(int) * 2);
  if (counts == NULL) {
    perror("malloc returned NULL");
    return ret;
  }
  *counts++ = 0;
  *counts-- = 0;

  void *tree = NULL;
  struct InitRet tree_ret = { NULL, NULL };
  if (tree_type == 0) {
    tree_ret = gen_tree_init(ids, n, *tree_flags, *(tree_flags + 1), users, &lbbt_init);
    tree = tree_ret.tree;
  }
  //  else
  //    added = gen_tree_add(multicast->tree, data, &btree_add);
  
  struct Multicast *lbbt_multicast = malloc(sizeof(struct Multicast));
  if (lbbt_multicast == NULL) {
    perror("malloc returned NULL");
    return ret;
  }

  size_t prg_out_size, seed_size;
  get_prg_out_size(generator, &prg_out_size);
  get_seed_size(generator, &seed_size);
  struct Multicast multicast = { 1, users, tree, counts, tree_type, testing, prg_out_size, seed_size };
  *lbbt_multicast = multicast;

  ret.multicast = lbbt_multicast;
  ret.skeleton = tree_ret.skeleton;

  struct List *oob_seeds = NULL;
  if (!testing) {
    oob_seeds = malloc(sizeof(struct List));
    if (oob_seeds == NULL) {
      perror("malloc returned NULL");
      return ret;
    }
    initList(oob_seeds);
  }
  secret_gen(lbbt_multicast, tree_ret.skeleton, oob_seeds, sampler, generator);// free

  ret.oob_seeds = oob_seeds;
  
  return ret;
}

struct MultAddRet mult_add(struct Multicast *multicast, int id, void *sampler, void *generator) {
  struct MultAddRet ret = { NULL, NULL, NULL };
  struct AddRet add_ret;
  if (multicast->tree_type == 0)
    add_ret = gen_tree_add(multicast->tree, id, &lbbt_add);
  //  else
  //    added = gen_tree_add(multicast->tree, data, &btree_add);

  addFront(multicast->users, (void *) add_ret.added);
  
  ret.added = add_ret.added;
  ret.skeleton = add_ret.skeleton;

  struct List *oob_seeds = NULL;  
  if (!multicast->testing) {  
    oob_seeds = malloc(sizeof(struct List));
    if (oob_seeds == NULL) {
      perror("malloc returned NULL");
      return ret;
    }
    initList(oob_seeds);
  }
  secret_gen(multicast, ret.skeleton, oob_seeds, sampler, generator); //free

  ret.oob_seed = oob_seeds->head->data;
  removeAllNodes(oob_seeds);
  //free(oob_seeds);

  return ret;
}

struct SkeletonNode *gen_upd_skel(struct Node *node, struct Node *child, struct SkeletonNode *child_skel) {
  if (node != NULL) {
    struct SkeletonNode *skeleton = malloc(sizeof(struct SkeletonNode));
    if (skeleton == NULL) {
      perror("malloc returned NULL");
      return NULL;
    }
    int *children_color = malloc(sizeof(int) * 2);
    if (children_color == NULL) {
      perror("malloc returned NULL");
      return NULL;
    }
    skeleton->node_id = ((struct NodeData *) node->data)->id;
    skeleton->children_color = children_color;
    skeleton->node = node;
    skeleton->ciphertexts = NULL;
    
    int child_pos;
    if (child == *(node->children)) {
      child_pos = 0;
    } else {
      child_pos = 1;
    }
    *(children_color + child_pos) = 0;
    *(children_color + (1 - child_pos)) = 1;
    
    if (child_skel != NULL) {
      child_skel->parent = skeleton;
      struct SkeletonNode **skel_children = malloc(sizeof(struct skeletonNode *) * 2);
      if (skel_children == NULL) {
	perror("malloc returned NULL");
	return NULL;
      }
      skeleton->children = skel_children;
      
      *(skel_children + child_pos) = child_skel;
      *(skel_children + (1 - child_pos)) = NULL;
    } else {
      skeleton->children = NULL;
    }

    return gen_upd_skel(node->parent, node, skeleton);
  }
  return child_skel;
}

struct MultUpdRet mult_update(struct Multicast *multicast, struct Node *user, void *sampler, void *generator) { //TODO: user should be node???
  struct MultUpdRet ret = { NULL, NULL };
  struct SkeletonNode *leaf_skeleton = malloc(sizeof(struct SkeletonNode));
  if (leaf_skeleton == NULL) {
    perror("malloc returned NULL");
    return ret;
  }
  leaf_skeleton->node_id = ((struct NodeData *) user->data)->id;  
  leaf_skeleton->node = user;
  leaf_skeleton->children = NULL;
  leaf_skeleton->children_color = NULL;
  leaf_skeleton->ciphertexts = NULL;
  
  struct SkeletonNode *skeleton = gen_upd_skel(user->parent, user, leaf_skeleton);
  skeleton->parent = NULL;

  ret.skeleton = skeleton;

  struct List *oob_seeds = NULL;  
  if (!multicast->testing) {
    oob_seeds = malloc(sizeof(struct List));
    if (oob_seeds == NULL) {
      perror("malloc returned NULL");
      return ret;
    }
    initList(oob_seeds);
  }
  secret_gen(multicast, skeleton, oob_seeds, sampler, generator);//free

  ret.oob_seed = oob_seeds->head->data;
  removeAllNodes(oob_seeds);
  //free(oob_seeds);
  
  return ret;
}

struct RemRet mult_rem(struct Multicast *multicast, struct Node *user, void *sampler, void *generator) { //TODO: user should be node??
  struct RemRet ret = { NULL, NULL };
  if (multicast->tree_type == 0)
    ret = gen_tree_rem(multicast->tree, user, &lbbt_rem);
  //    else
  //      gen_tree_rem(multicast->tree, user, &btree_rem);

  secret_gen(multicast, ret.skeleton, NULL, sampler, generator); //free
  //pretty_traverse_skeleton(ret.skeleton, 0, &printSkeleton);
  //pretty_traverse_tree(((struct LBBT *)multicast->tree)->root, 0, &printNode);
  
  return ret;
}

void mult_destroy(struct Multicast *multicast) {
  destroy_tree(((struct LBBT *) multicast->tree)->root);
  removeAllNodes(((struct LBBT *) multicast->tree)->blanks);
  //free(((struct LBBT *) multicast->tree)->blanks); THIS WAS COMMENTED
  //free(multicast->tree); THIS WAS COMMENTED

  removeAllNodes(multicast->users);
  //free(multicast->users);

  //free(multicast->counts);

  //free(multicast);
}
