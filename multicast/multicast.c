#include <stdio.h>
#include <stdlib.h>
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

/*static void printIntLine(void *p)
{
  struct NodeData *data = (struct NodeData *) ((struct Node *) p)->data;
  if (data->blank == 1)
    printf("BLANK, id: %d", data->id);
  else
    printf("%d", data->id);
    }*/

//TODO: make sure this generalizes to any multicast scheme
void secret_gen(struct Multicast *multicast, struct SkeletonNode *skeleton) {
  if (skeleton != NULL && skeleton->children_color != NULL) {
    if (skeleton->children != NULL) {
      if (*(skeleton->children) != NULL)
	secret_gen(multicast, *(skeleton->children));
      if (*(skeleton->children + 1) != NULL)
	secret_gen(multicast, *(skeleton->children + 1));
    }
    if (*(skeleton->children_color) == 1)
      (*(multicast->counts + 1))++;
    else if (*(skeleton->children_color) == 0)
      (*(multicast->counts))++;
    if (*(skeleton->children_color + 1) == 1)
      (*(multicast->counts + 1))++;
    else if (*(skeleton->children_color + 1) == 0)
      (*(multicast->counts))++;
  }
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

  secret_gen(lbbt_multicast, tree_ret.skeleton);

  //pretty_traverse_tree(((struct LBBT *)lbbt_multicast->tree)->root, 0, &printIntLine);
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
  secret_gen(multicast, ret.skeleton);

  //pretty_traverse_tree(((struct LBBT *)multicast->tree)->root, 0, &printIntLine);
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

void mult_update(struct Multicast *multicast, struct Node *user) { //TODO: user should be node???
  struct SkeletonNode *skeleton = gen_upd_skel(user->parent, user, NULL);
  secret_gen(multicast, skeleton);

  //pretty_traverse_skeleton(skeleton, 0, &printSkeleton);
  
  destroy_skeleton(skeleton);
}

void *mult_rem(struct Multicast *multicast, struct Node *user) { //TODO: user should be node??
  struct RemRet ret = { NULL, NULL };
  if (multicast->tree_type == 0)
    ret = gen_tree_rem(multicast->tree, user, &lbbt_rem);
  //    else
  //      gen_tree_rem(multicast->tree, user, &btree_rem);
  secret_gen(multicast, ret.skeleton);

  //pretty_traverse_tree(((struct LBBT *)multicast->tree)->root, 0, &printIntLine);
  //pretty_traverse_skeleton(ret.skeleton, 0, &printSkeleton);
  if (ret.skeleton != NULL)
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
