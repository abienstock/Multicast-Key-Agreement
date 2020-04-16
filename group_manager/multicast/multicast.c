#include <stdio.h>
#include <stdlib.h>
#include <math.h> // for now
#include "../trees/trees.h"
#include "multicast.h"

/*static void printSkeleton(void *p)
{
  struct SkeletonNode *node = (struct SkeletonNode *) p;
  if (node->children_color == NULL)
    printf("no children");
  else {
    printf("left: %d, ", *(node->children_color));
    printf("right: %d", *(node->children_color+1));
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

void *identity(void *key, void *data) {
  return data;
}

//TODO: make sure this generalizes to any multicast scheme
void *secret_gen(struct Multicast *multicast, struct SkeletonNode *skeleton, void *(*gen_seed)(), void *(*prg)(void *), void **(*split)(void *), void *(*encrypt)(void *, void *)) {
  void *next_seed, *seed, *key, *out;

  struct NodeData *data = (struct NodeData *) skeleton->node->data;
  if (data->key != NULL)
    free(data->key);
  if (data->seed != NULL)
    free(data->seed);
  
  if (skeleton->children_color != NULL) {
    if (*(skeleton->children_color) == 0) {
      (*(multicast->counts))++; // fix this
      void *child_seed = secret_gen(multicast, *(skeleton->children), &int_gen, &int_prg, &int_split, &identity);
      out = prg(child_seed);
      free(child_seed);
      if (skeleton->children != NULL && *(skeleton->children + 1) != NULL)
	free(secret_gen(multicast, *(skeleton->children + 1), &int_gen, &int_prg, &int_split, &identity));
    } else if (*(skeleton->children_color + 1) == 0) {
      (*(multicast->counts))++; // fix this
      void *child_seed = secret_gen(multicast, *(skeleton->children + 1), &int_gen, &int_prg, &int_split, &identity);
      out = prg(child_seed);
      free(child_seed);
      if (skeleton->children != NULL && *(skeleton->children) != NULL)
	free(secret_gen(multicast, *(skeleton->children), &int_gen, &int_prg, &int_split, &identity));
    } else {
      void *temp_seed = gen_seed();
      out = prg(temp_seed);
      free(temp_seed);
    }
    if (*(skeleton->children_color) == 1) {
      (*(multicast->counts + 1))++;
    }
    if (*(skeleton->children_color + 1) == 1) {
      (*(multicast->counts + 1))++;
    }
  } else {
    void *temp_seed = gen_seed();
    out = prg(temp_seed);
    free(temp_seed);
  }
  void **out_split = split(out);

  seed = out_split[0];
  key = out_split[1];
  next_seed = out_split[2];
  free(out_split);
  free(out);
  
  data->key = key;
  data->seed = seed;
  return next_seed;
}

struct Multicast *mult_init(int n, int *tree_flags, int tree_type) {
  struct List *users = malloc(sizeof(struct List));
  if (users == NULL) {
    perror("malloc returned NULL");
    return NULL;
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
    return NULL;
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
    return NULL;
  }
  struct Multicast multicast = { 1, users, tree, counts, tree_type };
  *lbbt_multicast = multicast;

  free(secret_gen(lbbt_multicast, tree_ret.skeleton, &int_gen, &int_prg, &int_split, &identity));

  //pretty_traverse_tree(((struct LBBT *)lbbt_multicast->tree)->root, 0, &printNode);
  //pretty_traverse_skeleton(tree_ret.skeleton, 0, &printSkeleton);
  destroy_skeleton(tree_ret.skeleton);
  
  return lbbt_multicast;
}

struct Node *mult_add(struct Multicast *multicast, int id) {
  struct AddRet ret = { NULL, NULL };
  if (multicast->tree_type == 0)
    ret = gen_tree_add(multicast->tree, id, &lbbt_add);
  //  else
  //    added = gen_tree_add(multicast->tree, data, &btree_add);
  free(secret_gen(multicast, ret.skeleton, &int_gen, &int_prg, &int_split, &identity));

  //pretty_traverse_tree(((struct LBBT *)multicast->tree)->root, 0, &printNode);
  //pretty_traverse_skeleton(ret.skeleton, 0, &printSkeleton);
  destroy_skeleton(ret.skeleton);
  
  return ret.added;
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
    
    int child_pos;
    if (child == *(node->children)) {
      child_pos = 0;
    } else {
      child_pos = 1;
    }
    *(children_color + child_pos) = 0;
    *(children_color + (1 - child_pos)) = 1;
    
    if (child_skel != NULL) {
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

int mult_update(struct Multicast *multicast, struct Node *user) { //TODO: user should be node???
  struct SkeletonNode *leaf_skeleton = malloc(sizeof(struct SkeletonNode));
  if (leaf_skeleton == NULL) {
    perror("malloc returned NULL");
    return 1;
  }
  leaf_skeleton->node = user;
  leaf_skeleton->children = NULL;
  leaf_skeleton->children_color = NULL;
  
  struct SkeletonNode *skeleton = gen_upd_skel(user->parent, user, leaf_skeleton);
  free(secret_gen(multicast, skeleton, &int_gen, &int_prg, &int_split, &identity));

  //pretty_traverse_tree(((struct LBBT *)multicast->tree)->root, 0, &printNode);
  //pretty_traverse_skeleton(skeleton, 0, &printSkeleton);
  
  destroy_skeleton(skeleton);
  return 0;
}

void *mult_rem(struct Multicast *multicast, struct Node *user) { //TODO: user should be node??
  struct RemRet ret = { NULL, NULL };
  if (multicast->tree_type == 0)
    ret = gen_tree_rem(multicast->tree, user, &lbbt_rem);
  //    else
  //      gen_tree_rem(multicast->tree, user, &btree_rem);

  //pretty_traverse_tree(((struct LBBT *)multicast->tree)->root, 0, &printNode);
  //pretty_traverse_skeleton(ret.skeleton, 0, &printSkeleton);
  free(secret_gen(multicast, ret.skeleton, &int_gen, &int_prg, &int_split, &identity));
  destroy_skeleton(ret.skeleton);
  
  return ret.data;
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