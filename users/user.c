#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "user.h"
#include "../ll/ll.h"
#include "../skeleton.h"
#include "../crypto/crypto.h"
#include "../utils.h"

struct Entry {
  struct SkeletonNode *skel_node;
  struct ListNode *path_node;
  int child_pos;
  int height;
};

void free_path_data(void *data) {
  struct PathData *path_data = (struct PathData *) data;
  free(path_data->key);
  free(path_data->seed);
  free(path_data);
}

/*
 * initialize a user with id
 */
struct User *init_user(int id, size_t prg_out_size, size_t seed_size) {
  struct User *user = malloc_check(sizeof(struct User));
  struct List *secrets = malloc_check(sizeof(struct List));
  initList(secrets);
  user->secrets = secrets;
  user->id = id;
  user->in_group = 0;
  user->prg_out_size = prg_out_size;
  user->seed_size = seed_size;
  return user;
}

struct find_in_path_ret {
  struct ListNode *path_node;
  int height;
};

/*
 * find the node with id in direct path secrets list
 */
struct find_in_path_ret find_in_path(int id, struct List *secrets) {
  struct ListNode *curr = secrets->head;
  int i = 0;
  while (curr != NULL) {
    if (((struct PathData *) curr->data)->node_id == id) {
      struct find_in_path_ret ret = { curr, i };
      return ret;
    }
    curr = curr->next;
    i++;
  }
  struct find_in_path_ret ret = { NULL, -1 };
  return ret;
}

/*
 * recursively find the path node of user and corresponding parent skeleton node that is in the frontier of skeleton
 * TODO: log^2 complexity with ll -- use Hash??
 */
struct Entry find_entry(struct User *user, struct SkeletonNode *skeleton) {
  int i;
  struct Entry candidate_entry = { skeleton, NULL, -1, INT_MAX  };
  if (skeleton->ciphertext_lists != NULL) {
    struct find_in_path_ret ret;
    for (i = 0; i < skeleton->node->num_children; i++) {
      if (*(skeleton->ciphertext_lists + i) != NULL && (skeleton->children == NULL || *(skeleton->children + i) == NULL)) { // TODO: do this check before looping?
	struct Ciphertext *ciphertext = popFront(*(skeleton->ciphertext_lists + i));
	ret = find_in_path(ciphertext->child_id, user->secrets);
	if (ret.path_node != NULL) {
	  candidate_entry.path_node = ret.path_node;
	  candidate_entry.child_pos = i;
	  candidate_entry.height = ret.height;
	  //return candidate_entry;
	  break;
	}
      }
    }
  }
  struct Entry recursive_entry;
  if (skeleton->children != NULL) {
    for (i = 0; i < skeleton->node->num_children; i++) {
      if (*(skeleton->children + i) != NULL && (recursive_entry = find_entry(user, *(skeleton->children + i))).path_node != NULL && recursive_entry.height < candidate_entry.height) {
	candidate_entry = recursive_entry;
      }
    }
  }
  return candidate_entry;
}

/*
 * recursively find the skelton node with id (meant to be used to find leaf nodes in tree)
 */
struct SkeletonNode *find_skel_node(int id, struct SkeletonNode *skeleton) {
  if (skeleton->children == NULL) {
    if (skeleton->node_id == id)
      return skeleton;
  } else {
    struct SkeletonNode *ret_skel;
    int i;
    for (i = 0; i < skeleton->node->num_children; i++) {
      if (*(skeleton->children + i) != NULL && (ret_skel = find_skel_node(id, *(skeleton->children + i))) != NULL)
	return ret_skel;
    }
  }
  return NULL; // TODO: error checking
}

/*
 * generate the secrets on the direct path of path_node for user with proposed_seed and the ciphetexts of skel_node.
 */
struct ListNode *path_gen(struct User *user, void *proposed_seed, void *child_seed, void *child_key, struct SkeletonNode *skel_node, struct SkeletonNode *child_skel, struct ListNode *path_node, void *generator) {
  void *seed;
  void *next_seed = NULL, *new_seed = NULL, *key = NULL, *out = NULL;
  
  if (child_skel == NULL) {
    seed = proposed_seed;
  } else {
    int i, child_pos = 0;
    for (i = 0; i < skel_node->node->num_children; i++) {
      if (*(skel_node->children + i) == child_skel)
	child_pos = i;
    }
    if (*(skel_node->children_color + child_pos) == 0) {
      seed = proposed_seed;
    } else { 
      free(proposed_seed);
      struct Ciphertext *ciphertext = popFront(*(skel_node->ciphertext_lists + child_pos));
      seed = malloc_check(user->seed_size);
      dec(generator, child_key, child_seed, ciphertext->ct, seed, user->seed_size);
    }
  }

  struct PathData *data = (struct PathData *) path_node->data;

  alloc_prg_out(&out, &new_seed, &key, &next_seed, user->prg_out_size, user->seed_size);
  prg(generator, seed, out);
  split(out, new_seed, key, next_seed, user->seed_size);
  free(seed);
  free(out);

  data->node_id = skel_node->node_id;
  if (data->key != NULL)
    free(data->key);
  data->key = key;
  if (data->seed != NULL)
    free(data->seed);
  data->seed = new_seed;

  if (skel_node->parent != NULL) {
    struct ListNode *next;
    if (path_node->next == NULL) { // direct path has been extended by operation
      struct PathData *data = malloc_check(sizeof(struct PathData));
      data->key = NULL;
      data->seed = NULL;
      next = addAfter(user->secrets, path_node, (void *) data);
    } else {
      next = path_node->next;
    }
    return path_gen(user, next_seed, new_seed, key, skel_node->parent, skel_node, next, generator);
  }
  free(next_seed);
  return path_node;
}

/*
 * function for user to process a skeleton generated by an operation on id (-1 if create) with optional oob_seed
 */
void *proc_ct(struct User *user, int id, struct SkeletonNode *skeleton, void *oob_seed, void *generator) {
  void *seed; // generating seed of first node on the direct path of user in skeleton
  struct SkeletonNode *skel_node;
  struct ListNode *path_node;
  if (id != -1 && id != user->id) { // -1 for create
    if (skeleton->node_id == user->id) { // user is root
      skel_node = skeleton;
      path_node = user->secrets->head;
      struct PathData *path_node_data = ((struct PathData *) path_node->data);
      struct Ciphertext *ciphertext = popFront(*skel_node->ciphertext_lists);
      seed = malloc_check(user->seed_size);
      dec(generator, path_node_data->key, path_node_data->seed, ciphertext->ct, seed, user->seed_size);
    } else {
      struct Entry entry = find_entry(user, skeleton); // find node on direct path that is in frontier of skeleton
      skel_node = entry.skel_node;
      path_node = entry.path_node;
      if (entry.path_node != NULL) {
	struct PathData *path_node_data = ((struct PathData *) path_node->data);
	struct Ciphertext *ciphertext = popFront(*(skel_node->ciphertext_lists + entry.child_pos));
	seed = malloc_check(user->seed_size);
	dec(generator, path_node_data->key, path_node_data->seed, ciphertext->ct, seed, user->seed_size);
	if (path_node->next == NULL) { // direct path has been extended by operation
	  struct PathData *data = malloc_check(sizeof(struct PathData));
	  data->seed = NULL;
	  data->key = NULL;
	  path_node = addAfter(user->secrets, path_node, (void *) data);
	} else {
	  path_node = path_node->next;
	}
      } else {
	seed = NULL; // TODO: error checking
      }
    }
  } else {
    seed = oob_seed;
    skel_node = find_skel_node(user->id, skeleton);
    if (user->secrets->head == NULL) {
      struct PathData *data = malloc_check(sizeof(struct PathData));
      data->seed = NULL;
      data->key = NULL;
      path_node = addFront(user->secrets, (void *) data);
    }
    path_node = user->secrets->head;
  }

  // generate new secrets on direct path
  struct ListNode *end_node = path_gen(user, seed, NULL, NULL, skel_node, NULL, path_node, generator);

  //free rest of path if not at tail
  while (end_node != user->secrets->tail) {
    free_path_data(popBack(user->secrets));
  }

  // generate group secret
  void *out = malloc_check(user->prg_out_size);
  prg(generator, ((struct PathData *) end_node->data)->seed, out);
  return out;
}

void free_users(struct List *users) {
  struct ListNode *user_curr = users->head;
  while (user_curr != 0) {
    struct User *user = ((struct User *) user_curr->data);
    free_user(user);
    struct ListNode *old_user = user_curr;
    user_curr = user_curr->next;
    free(old_user);
  }
  free(users);
}

void free_user(struct User *user) {
  struct List *secrets = user->secrets;
  struct ListNode *secret_curr = secrets->head;
  while (secret_curr != 0) {
    free_path_data(secret_curr->data);
    struct ListNode *old_sec = secret_curr;
    secret_curr = secret_curr->next;
    free(old_sec);
  }
  free(secrets);
  free(user);
}
