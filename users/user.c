#include <stdlib.h>
#include <stdio.h>
#include "../ll/ll.h"
#include "../skeleton.h"
#include "user.h"

struct Entry {
  struct SkeletonNode *skel_node;
  struct ListNode *path_node;
  int child_pos;
};

struct ListNode *find_in_path(int id, struct List *secrets) {
  struct ListNode *curr = secrets->head;
  while (curr != NULL) {
    if (((struct PathData *) curr->data)->node_id == id)
      return curr;
    curr = curr->next;
  }
  return NULL;
}

//TODO: log^2 complexity with ll -- use Hash??
struct Entry find_entry(struct User *user, struct SkeletonNode *skeleton) {
  struct Entry entry = { skeleton, NULL, -1 };
  if (skeleton->ciphertexts != NULL) {
    struct ListNode *path_node;
    if (*skeleton->ciphertexts != NULL) {
      path_node = find_in_path((*skeleton->ciphertexts)->child_id, user->secrets);
      if (path_node != NULL) {
	entry.path_node = path_node;
	entry.child_pos = 0;
	return entry;
      }
    }
    if (*(skeleton->ciphertexts + 1) != NULL) {
      path_node = find_in_path((*(skeleton->ciphertexts + 1))->child_id, user->secrets);
      if (path_node != NULL) {
	entry.path_node = path_node;
	entry.child_pos = 1;
	return entry;
      }
    }
  }
  if (skeleton->children != NULL) {
    if (*skeleton->children != NULL && (entry = find_entry(user, *skeleton->children)).path_node != NULL)
      return entry;
    else if (*(skeleton->children + 1) != NULL && (entry = find_entry(user, *(skeleton->children + 1))).path_node != NULL)
      return entry;
  }
  return entry;
}

struct SkeletonNode *find_skel_node(int id, struct SkeletonNode *skeleton) {
  if (skeleton->children == NULL) {
    if(((struct NodeData *) skeleton->node->data)->id == id)
      return skeleton;
  } else {
    struct SkeletonNode *ret_skel;
    if (*skeleton->children != NULL && (ret_skel = find_skel_node(id, *skeleton->children)) != NULL)
      return ret_skel;
    if (*(skeleton->children + 1) != NULL && (ret_skel = find_skel_node(id, *(skeleton->children + 1))) != NULL)
      return ret_skel;
  }
  return NULL;
}

void write_to_path() {

}

struct ListNode *path_gen(struct User *user, void *prev_seed, void *prev_key, struct SkeletonNode *skel_node, struct SkeletonNode *child_skel, struct ListNode *path_node, void *(*prg)(void *), void **(*split)(void *), void *(*decrypt)(void *, void *)) {
  void *seed;
  if (child_skel == NULL) {
    seed = prev_seed;
  } else {
    int child_pos = 1;
    if (*skel_node->children == child_skel) {
      child_pos = 0;
    }
    if (*(skel_node->children_color + child_pos) == 0) {
      seed = prev_seed;
    } else { // TODO: make sure only other option is color = 1
      struct Ciphertext *ciphertext = *(skel_node->ciphertexts + child_pos);
      seed = decrypt(prev_key, ciphertext->ct);
    }
  }

  struct PathData *data = (struct PathData *) path_node->data;
  
  void *out = prg(seed);
  void **out_split = split(out);
  void *new_seed = out_split[0];
  void *key = out_split[1];
  void *next_seed = out_split[2];
  data->node_id = ((struct NodeData *) skel_node->node->data)->id;
  data->key = key;
  data->seed = new_seed;

  
  if (skel_node->parent != NULL) {
    struct ListNode *next;
    if (path_node->next == NULL) {
      struct PathData *data = malloc(sizeof(struct PathData));
      if (data == NULL) {
	perror("malloc returned NULL");
	return NULL;
      }
      next = addAfter(user->secrets, path_node, (void *) data);
    } else {
      next = path_node->next;
    }
    return path_gen(user, next_seed, key, skel_node->parent, skel_node, next, prg, split, decrypt);
  }
  return path_node;
}

void *proc_ct(struct User *user, int id, struct SkeletonNode *skeleton, void *oob_seed, void *(*prg)(void *), void **(*split)(void *), void *(*decrypt)(void *, void *)) {
  void *seed;
  struct SkeletonNode *skel_node;
  struct ListNode *path_node;
  if (id != -1 && id != user->id) { // -1 for create
    if (((struct NodeData *) skeleton->node->data)->id == user->id) {
      skel_node = skeleton;
      path_node = user->secrets->head;
      struct PathData *path_node_data = ((struct PathData *) path_node->data);
      struct Ciphertext *ciphertext = *skel_node->ciphertexts;
      seed = decrypt(path_node_data->key, ciphertext->ct);
    } else {
      struct Entry entry = find_entry(user, skeleton);
      skel_node = entry.skel_node;
      path_node = entry.path_node;
      struct PathData *path_node_data = ((struct PathData *) path_node->data);
      struct Ciphertext *ciphertext = *(skel_node->ciphertexts + entry.child_pos);
      seed = decrypt(path_node_data->key, ciphertext->ct);
      if (path_node->next == NULL) {
	struct PathData *data = malloc(sizeof(struct PathData));
	if (data == NULL) {
	  perror("malloc returned NULL");
	  return NULL;
	}
	path_node = addAfter(user->secrets, path_node, (void *) data);
      } else {
	path_node = path_node->next;
      }
    }
  } else {
    seed = oob_seed;
    skel_node = find_skel_node(user->id, skeleton);
    //if (skel_node == NULL) // TODO: if user is removed
    //  return NULL;
    if (user->secrets->head == NULL) {
      struct PathData *data = malloc(sizeof(struct PathData));
      if (data == NULL) {
	perror("malloc returned NULL");
	return NULL;
      }
      path_node = addFront(user->secrets, (void *) data);
    }
    path_node = user->secrets->head;
  }

  struct ListNode *end_node = path_gen(user, seed, NULL, skel_node, NULL, path_node, prg, split, decrypt);

  //free rest of path if not at tail
  while (end_node != user->secrets->tail) {
    struct PathData *data = (struct PathData *) popBack(user->secrets);
    free(data->key);
    free(data->seed);
  }

  return prg(((struct PathData *) end_node->data)->seed);
}

struct User *init_user(int id) {
  struct User *user = malloc(sizeof(struct User));
  if (user == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }
  struct List *secrets = malloc(sizeof(struct List));
  if (secrets == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }
  initList(secrets);

  user->secrets = secrets;
  user->id = id;

  return user;
}
