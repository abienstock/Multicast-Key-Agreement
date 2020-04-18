#include <stdio.h>
#include <stdlib.h>
#include <math.h> // for now
#include "../trees/trees.h"
#include "multicast.h"
#include "../../skeleton.h"

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
  }*/

/*static void printNode(void *p)
{
  struct NodeData *data = (struct NodeData *) ((struct Node *) p)->data;
  if (data->blank == 1)
    printf("BLANK, id: %d", data->id);
  else {
    printf("id: %d, ", data->id);
    printf("key: %d, ", *((int *)data->key));
    printf("seed: %d.", *((int *)data->seed));
  }
  }*/

int ct_gen(struct Multicast *multicast, struct SkeletonNode *skeleton, struct NodeData *data, void *seed, void *(*encrypt)(void *, void *)) {
  if (skeleton->children_color != NULL) {
    struct Ciphertext **cts = malloc(sizeof(struct Ciphertext *) * 2);
    if (cts == NULL) {
      perror("malloc returned NULL");
      return -1;
    }
    struct Ciphertext *left_ct = NULL, *right_ct = NULL;
    if (*(skeleton->children_color) == 1) {
      (*(multicast->counts + 1))++;
      
      left_ct = malloc(sizeof(struct Ciphertext));
      if (left_ct == NULL) {
	perror("malloc returned NULL");
	return -1;
      }
      struct NodeData *left_data = (struct NodeData *) (*(skeleton->node->children))->data;
      left_ct->parent_id = data->id;
      left_ct->child_id = left_data->id;
      left_ct->ct = encrypt(left_data->key, seed);
    }
    if (*(skeleton->children_color + 1) == 1) {
      (*(multicast->counts + 1))++;
      
      right_ct = malloc(sizeof(struct Ciphertext));
      if (right_ct == NULL) {
	perror("malloc returned NULL");
	return -1;
      }
      struct NodeData *right_data = (struct NodeData *) (*(skeleton->node->children + 1))->data;
      right_ct->parent_id = data->id;
      right_ct->child_id = right_data->id;
      right_ct->ct = encrypt(right_data->key, seed);
    }
    *cts++ = left_ct;
    *cts-- = right_ct;
    skeleton->ciphertexts = cts;
  }
  return 0;
}

//TODO: make sure this generalizes to any multicast scheme
void *secret_gen(struct Multicast *multicast, struct SkeletonNode *skeleton, struct List *oob_seeds, void *(*gen_seed)(), void *(*prg)(void *), void **(*split)(void *), void *(*encrypt)(void *, void *)) {
  void *prev_seed, *next_seed, *seed, *key, *out;

  struct NodeData *data = (struct NodeData *) skeleton->node->data;
  if (data->key != NULL)
    free(data->key);
  if (data->seed != NULL)
    free(data->seed);
  
  if (skeleton->children_color != NULL) {
    if (*(skeleton->children_color) == 0) {
      (*(multicast->counts))++; // fix this
      prev_seed = secret_gen(multicast, *(skeleton->children), oob_seeds, gen_seed, prg, split, encrypt);
      if (skeleton->children != NULL && *(skeleton->children + 1) != NULL)
	free(secret_gen(multicast, *(skeleton->children + 1), oob_seeds, gen_seed, prg, split, encrypt));
    } else if (*(skeleton->children_color + 1) == 0) {
      (*(multicast->counts))++; // fix this
      prev_seed = secret_gen(multicast, *(skeleton->children + 1), oob_seeds, gen_seed, prg, split, encrypt);
      if (skeleton->children != NULL && *(skeleton->children) != NULL)
	free(secret_gen(multicast, *(skeleton->children), oob_seeds, gen_seed, prg, split, encrypt));
    } else {
      prev_seed = gen_seed();
    }
  } else {
    prev_seed = gen_seed();
    addAfter(oob_seeds, oob_seeds->tail, prev_seed);
  }

  out = prg(prev_seed);
  void **out_split = split(out);

  seed = out_split[0];
  key = out_split[1];
  next_seed = out_split[2];
  free(out_split);
  free(out);
  
  data->key = key;
  data->seed = seed;

  ct_gen(multicast, skeleton, data, prev_seed, encrypt);
  //free(prev_seed); // might need to do it here if not oob_seed
  
  return next_seed;
}

struct MultInitRet mult_init(int n, int *tree_flags, int tree_type, void *(*gen_seed)(), void *(*prg)(void *), void **(*split)(void *), void *(*encrypt)(void *, void *)) {
  struct MultInitRet ret = { NULL, NULL };
  
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
  struct Multicast multicast = { 1, users, tree, counts, tree_type };
  *lbbt_multicast = multicast;

  struct List *oob_seeds = malloc(sizeof(struct List));
  if (oob_seeds == NULL) {
    perror("malloc returned NULL");
    return ret;
  }
  initList(oob_seeds);
  free(secret_gen(lbbt_multicast, tree_ret.skeleton, oob_seeds, gen_seed, prg, split, encrypt));

  //pretty_traverse_tree(((struct LBBT *)lbbt_multicast->tree)->root, 0, &printNode);
  //pretty_traverse_skeleton(tree_ret.skeleton, 0, &printSkeleton);
  //destroy_skeleton(tree_ret.skeleton);

  ret.multicast = lbbt_multicast;
  ret.skeleton = tree_ret.skeleton;
  ret.oob_seeds = oob_seeds;
  
  return ret;
}

struct MultAddRet mult_add(struct Multicast *multicast, int id, void *(*gen_seed)(), void *(*prg)(void *), void **(*split)(void *), void *(*encrypt)(void *, void *)) {
  struct MultAddRet ret = { NULL, NULL, NULL };
  struct AddRet add_ret;
  if (multicast->tree_type == 0)
    add_ret = gen_tree_add(multicast->tree, id, &lbbt_add);
  //  else
  //    added = gen_tree_add(multicast->tree, data, &btree_add);
  
  ret.added = add_ret.added;
  ret.skeleton = add_ret.skeleton;
  struct List *oob_seeds = malloc(sizeof(struct List));
  if (oob_seeds == NULL) {
    perror("malloc returned NULL");
    return ret;
  }
  initList(oob_seeds);
  free(secret_gen(multicast, ret.skeleton, oob_seeds, gen_seed, prg, split, encrypt));

  ret.oob_seed = oob_seeds->head->data;

  //pretty_traverse_tree(((struct LBBT *)multicast->tree)->root, 0, &printNode);
  //pretty_traverse_skeleton(ret.skeleton, 0, &printSkeleton);
  //destroy_skeleton(ret.skeleton);
  
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

struct MultUpdRet mult_update(struct Multicast *multicast, struct Node *user, void *(*gen_seed)(), void *(*prg)(void *), void **(*split)(void *), void *(*encrypt)(void *, void *)) { //TODO: user should be node???
  struct MultUpdRet ret = { NULL, NULL };
  struct SkeletonNode *leaf_skeleton = malloc(sizeof(struct SkeletonNode));
  if (leaf_skeleton == NULL) {
    perror("malloc returned NULL");
    return ret;
  }
  leaf_skeleton->node = user;
  leaf_skeleton->children = NULL;
  leaf_skeleton->children_color = NULL;
  leaf_skeleton->ciphertexts = NULL;
  
  struct SkeletonNode *skeleton = gen_upd_skel(user->parent, user, leaf_skeleton);
  skeleton->parent = NULL;

  ret.skeleton = skeleton;

  struct List *oob_seeds = malloc(sizeof(struct List));
  if (oob_seeds == NULL) {
    perror("malloc returned NULL");
    return ret;
  }
  initList(oob_seeds);
  free(secret_gen(multicast, skeleton, oob_seeds, gen_seed, prg, split, encrypt));

  //pretty_traverse_tree(((struct LBBT *)multicast->tree)->root, 0, &printNode);
  //pretty_traverse_skeleton(skeleton, 0, &printSkeleton);
  
  //destroy_skeleton(skeleton);

  ret.oob_seed = oob_seeds->head->data;
  
  return ret;
}

struct RemRet mult_rem(struct Multicast *multicast, struct Node *user, void *(*gen_seed)(), void *(*prg)(void *), void **(*split)(void *), void *(*encrypt)(void *, void *)) { //TODO: user should be node??
  struct RemRet ret = { NULL, NULL };
  if (multicast->tree_type == 0)
    ret = gen_tree_rem(multicast->tree, user, &lbbt_rem);
  //    else
  //      gen_tree_rem(multicast->tree, user, &btree_rem);

  free(secret_gen(multicast, ret.skeleton, NULL, gen_seed, prg, split, encrypt));
  //pretty_traverse_skeleton(ret.skeleton, 0, &printSkeleton);
  //pretty_traverse_tree(((struct LBBT *)multicast->tree)->root, 0, &printNode);
  //destroy_skeleton(ret.skeleton);
  
  return ret;
}

void mult_destroy(struct Multicast *multicast) {
  destroy_tree(((struct LBBT *) multicast->tree)->root);
  removeAllNodes(((struct LBBT *) multicast->tree)->blanks);
  free(((struct LBBT *) multicast->tree)->blanks);
  free(multicast->tree);

  removeAllNodes(multicast->users);
  free(multicast->users);

  free(multicast->counts);

  free(multicast);
}
