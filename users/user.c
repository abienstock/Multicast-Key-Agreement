#include <stdlib.h>
#include <stdio.h>
#include "../ll/ll.h"
#include "../skeleton.h"
#include "user.h"
#include "../crypto/crypto.h"

struct Entry {
  struct SkeletonNode *skel_node;
  struct ListNode *path_node;
  int child_pos;
};

struct User *init_user(int id, size_t prg_out_size, size_t seed_size) {
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
  user->in_group = 0;
  user->prg_out_size = prg_out_size;
  user->seed_size = seed_size;

  return user;
}

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
    if (skeleton->node_id == id)
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

struct ListNode *path_gen(struct User *user, void *prop_seed, void *prev_seed, void *prev_key, struct SkeletonNode *skel_node, struct SkeletonNode *child_skel, struct ListNode *path_node, void *generator) {
  void *seed;
  void *next_seed = malloc(user->seed_size);
  if (next_seed == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }
  void *new_seed = malloc(user->seed_size);
  if (new_seed == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }
  void *key = malloc(user->seed_size);
  if (key == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }
  void *out = malloc(user->prg_out_size);  
  if (out == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }
  
  if (child_skel == NULL) {
    seed = prop_seed;
  } else {
    int child_pos = 1;
    if (*skel_node->children == child_skel) {
      child_pos = 0;
    }
    if (*(skel_node->children_color + child_pos) == 0) {
      seed = prop_seed;
    } else { // TODO: make sure only other option is color = 1
      //free(prop_seed);
      struct Ciphertext *ciphertext = *(skel_node->ciphertexts + child_pos);
      seed = malloc(user->seed_size);
      if (seed == NULL) {
	perror("malloc returned NULL");
	return NULL;
      }
      dec(generator, prev_key, prev_seed, ciphertext->ct, seed, user->seed_size);
    }
  }

  struct PathData *data = (struct PathData *) path_node->data;

  prg(generator, seed, out);
  split(out, new_seed, key, next_seed, user->seed_size);
  //printf("seed: %s\n", (char *) seed);  
  /*void *out = prg(seed);
  void **out_split = split(out);
  void *new_seed = out_split[0];
  void *key = out_split[1];
3  void *next_seed = out_split[2];*/
  //free(seed);
  //free(out);

  data->node_id = skel_node->node_id;
  //if (data->key != NULL)
  //free(data->key);
  data->key = key;
  //if (data->nonce != NULL)
  //free(data->nonce);
  //if (data->seed != NULL)
  //free(data->seed);
  data->seed = new_seed;

  if (skel_node->parent != NULL) {
    struct ListNode *next;
    if (path_node->next == NULL) {
      struct PathData *data = malloc(sizeof(struct PathData));
      if (data == NULL) {
	perror("malloc returned NULL");
	return NULL;
      }
      data->key = NULL;
      data->seed = NULL;
      next = addAfter(user->secrets, path_node, (void *) data);
    } else {
      next = path_node->next;
    }
    return path_gen(user, next_seed, new_seed, key, skel_node->parent, skel_node, next, generator);
  }
  //free(next_seed);
  return path_node;
}

void *proc_ct(struct User *user, int id, struct SkeletonNode *skeleton, void *oob_seed, void *generator) {
  void *seed;
  struct SkeletonNode *skel_node;
  struct ListNode *path_node;
  if (id != -1 && id != user->id) { // -1 for create
    if (skeleton->node_id == user->id) { // user is root
      skel_node = skeleton;
      path_node = user->secrets->head;
      struct PathData *path_node_data = ((struct PathData *) path_node->data);
      struct Ciphertext *ciphertext = *skel_node->ciphertexts;
      seed = malloc(user->seed_size);
      if (seed == NULL) {
	perror("malloc returned NULL");
	return NULL;
      }
      dec(generator, path_node_data->key, path_node_data->seed, ciphertext->ct, seed, user->seed_size);
    } else {
      struct Entry entry = find_entry(user, skeleton);
      skel_node = entry.skel_node;
      path_node = entry.path_node;
      if (entry.path_node != NULL) {
	struct PathData *path_node_data = ((struct PathData *) path_node->data);
	struct Ciphertext *ciphertext = *(skel_node->ciphertexts + entry.child_pos);
	seed = malloc(user->seed_size);
	if (seed == NULL) {
	  perror("malloc returned NULL");
	  return NULL;
	}
	dec(generator, path_node_data->key, path_node_data->seed, ciphertext->ct, seed, user->seed_size);
	if (path_node->next == NULL) {
	  struct PathData *data = malloc(sizeof(struct PathData));
	  if (data == NULL) {
	    perror("malloc returned NULL");
	    return NULL;
	  }
	  data->seed = NULL;
	  data->key = NULL;
	  path_node = addAfter(user->secrets, path_node, (void *) data);
	} else {
	  path_node = path_node->next;
	}
      } else { // TODO: make sure compatible w/ non network drivers
	return NULL;
      }
    }
  } else {
    seed = oob_seed;
    skel_node = find_skel_node(user->id, skeleton);
    if (skel_node == NULL) { // TODO: make sure compatible w/ non network drivers; destroy user path secrest??
      return NULL;
    }
    if (user->secrets->head == NULL) {
      struct PathData *data = malloc(sizeof(struct PathData));
      if (data == NULL) {
	perror("malloc returned NULL");
	return NULL;
      }
      data->seed = NULL;
      data->key = NULL;
      path_node = addFront(user->secrets, (void *) data);
    }
    path_node = user->secrets->head;
  }

  struct ListNode *end_node = path_gen(user, seed, NULL, NULL, skel_node, NULL, path_node, generator);

  //free rest of path if not at tail
  while (end_node != user->secrets->tail) {
    popBack(user->secrets);    
    //struct PathData *data = (struct PathData *)
    //free(data->key);
    //free(data->seed);
    //free(data);
  }

  void *out = malloc(user->prg_out_size);
  if (out == NULL) {
    perror("malloc returned NULL");
    return NULL;
  }
  prg(generator, ((struct PathData *) end_node->data)->seed, out);
  return out;
}

int proc_broadcast(struct User *user, void **buf, void *generator) {
  struct ListNode *root_node = user->secrets->tail;
  struct PathData *data = (struct PathData *) root_node->data;
  void *out = malloc(user->prg_out_size);
  if (out == NULL) {
    perror("malloc returned NULL");
    return -1;
  }  
  prg(generator, data->seed, out);
  void *new_seed = malloc(user->seed_size);
  void *key = malloc(user->seed_size);
  void *next_seed = malloc(user->seed_size);
  split(out, new_seed, key, next_seed, user->seed_size);
  void *pltxt = malloc(5);
  dec(generator, key, new_seed, *buf, pltxt, 5);
  printf("decrypted: %s\n", (char *) pltxt);
  return 5;
}

void destroy_users(struct List *users) {
  struct ListNode *user_curr = users->head;
  while (user_curr != 0) {
    //struct User *user = ((struct User *) user_curr->data);
    //destroy_user(user);
    //struct ListNode *old_user = user_curr;
    user_curr = user_curr->next;
    //free(old_user);
  }
  //free(users);
}

void destroy_user(struct User *user) {
  struct List *secrets = user->secrets;
  struct ListNode *secret_curr = secrets->head;
  while (secret_curr != 0) {
    //struct PathData *path_data = ((struct PathData *) secret_curr->data);
    //free(path_data->key);
    //free(path_data->seed);
    //free(path_data);
    //struct ListNode *old_sec = secret_curr;
    secret_curr = secret_curr->next;
    //free(old_sec);
  }
  //free(secrets);
  //free(user);
}
