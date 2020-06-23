#ifndef _SKELETON_H_
#define _SKELETON_H_

#include <stdint.h>
#include "group_manager/trees/trees.h"

// TODO: store children ids, parent skel node
struct SkeletonNode {
  int node_id;
  struct Node *node;
  struct SkeletonNode *parent;
  int *children_color; // the color of the edge between the node and its children -- 0 = red (PRG), 1 = blue (enc)
  struct SkeletonNode **children; // children in the skeleton (possibly one or both NULL)
  struct Ciphertext **ciphertexts;
};

struct Ciphertext {
  int parent_id;
  int child_id;
  uint8_t *ct;
  size_t num_bytes;
};

#endif /* #ifndef _SKELETON_H_ */
