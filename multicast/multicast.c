#include <stdio.h>
#include <stdlib.h>
#include "../trees/trees.h"
#include "multicast.h"

struct MultInitRet mult_init(int n, int *tree_flags, int tree_type) {
  struct MultInitRet ret = { NULL, NULL };
  struct List *users = malloc(sizeof(struct List));
  if (users == NULL) {
    perror("malloc returned NULL");
    return ret;
  }
  initList(users);
  
  void **ids = malloc(sizeof(void *) * n);
  if (ids == NULL) {
    perror("malloc returned NULL");
    return ret;
  }

  int i;
  for (i = 0; i < n; i++) {
    int *data = malloc(sizeof(int));
    if (data == NULL) {
      perror("malloc returned NULL");
      return ret;
    }
    *data = i;
    *(ids + i) = (void *) data;
  }

  void *tree = NULL;
  struct InitRet tree_ret;
  if (tree_type == 0) {
    tree_ret = gen_tree_init(ids, n, *tree_flags, *(tree_flags + 1), users, &lbbt_init);
    tree = tree_ret.tree;
  }
  //  else
  //    added = gen_tree_add(multicast->tree, data, &btree_add);
  
  free(ids);
  
  int *counts = malloc(sizeof(int) * 1); //TODO: just counting encs for now
  if (counts == NULL) {
    perror("malloc returned NULL");
    return ret;
  }
  *counts = 0;
  
  struct Multicast *lbbt_multicast = malloc(sizeof(struct Multicast));
  if (lbbt_multicast == NULL) {
    perror("malloc returned NULL");
    return ret;
  }
  struct Multicast multicast = { 1, users, tree, counts, tree_type };
  *lbbt_multicast = multicast;

  ret.multicast = lbbt_multicast;
  ret.skeleton = tree_ret.skeleton;
  return ret;
}

struct AddRet mult_add(struct Multicast *multicast, void *data) {
  struct AddRet ret = { NULL, NULL };
  if (multicast->tree_type == 0)
    ret = gen_tree_add(multicast->tree, data, &lbbt_add);
  //  else
  //    added = gen_tree_add(multicast->tree, data, &btree_add);
  return ret;
}

struct UpdRet mult_update(struct Multicast *multicast, struct Node *user) { //TODO: user should be node???
  return gen_tree_upd(multicast->tree, user);
}

struct RemRet mult_rem(struct Multicast *multicast, struct Node *user) { //TODO: user should be node??
    if (multicast->tree_type == 0)
      return gen_tree_rem(multicast->tree, user, &lbbt_rem);
    //    else
    //      gen_tree_rem(multicast->tree, user, &btree_rem);
    struct RemRet ret = { NULL, NULL };
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
