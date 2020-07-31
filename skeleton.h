#ifndef _SKELETON_H_
#define _SKELETON_H_

#include <stdint.h>
#include "group_manager/trees/trees.h"

struct SkeletonNode {
  int node_id;
  struct Node *node;
  struct SkeletonNode *parent;
  int *children_color; // the color of the edge between the node and its children -- 0 = red (PRG), 1 = blue (enc)
  struct SkeletonNode **children; // children in the skeleton (possibly one or both NULL)
  //struct Ciphertext **ciphertexts; // associated ciphertexts for skeleton children (possibly for none - all of them)
  struct List **ciphertext_lists; // associated ciphertexts for skeleton children (possibly for none - all of them)
  // In MKA, each list contains at most one Ciphertext struct corresponding to that child
  // In TK, each list contains the Ciphertext structs for the entire resolution of that child
  int special; // 1 if special; 0 if not
};

struct Ciphertext {
  int child_id; // id of node ciphertext is encrypted to
  void *ct;
};

#endif /* #ifndef _SKELETON_H_ */
