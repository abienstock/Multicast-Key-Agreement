#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "trees/trees.h"
#include "trees/ll.h"
#include "multicast/multicast.h"

/*static void printIntLine(void *p)
{
  struct Node *node = (struct Node *) p;
  if (node->data == NULL)
    printf("blank");
  else
    printf("%d", *(int *)node->data);
    }*/

int rand_int(int n, int distrib, float geo_param) {
  int i;
  switch (distrib) {
  case 0: //uniform
    return rand() % n; // in range 0 to n-1
  case 1: // geo
    for (i = 0; i < n; i++) {
      if (((float) rand() / (float) RAND_MAX) < geo_param)
	return i;
    }
    return n-1; // 0-indexed
  default:
    return -1; //TODO: error checking!!
  }
}

int next_op(struct Multicast *multicast, float add_wt, float upd_wt, int distrib, float geo_param, int *max_id) {
  int num_users = multicast->users->len;
  float operation = (float) rand() / (float) RAND_MAX;
  if (operation < add_wt || num_users == 1) { //TODO: forcing add with n=1 correct??
    (*max_id)++; //so adding new users w.l.o.g.
    printf("add: %d\n", *max_id);
    int *newdata = malloc(sizeof(int));
    if (newdata == NULL) {
      perror("malloc returned NULL");
      return -1; // TODO: error checking!!
    }
    *newdata = *max_id;
    struct Node *added = mult_add(multicast, (void *) newdata);
    addAfter(multicast->users, multicast->users->tail, added);
    //pretty_traverse_tree(((struct LBBT *)multicast->tree)->root, 0, &printIntLine);
    return 0;
  } else if (operation < add_wt + upd_wt) {
    printf("upd\n");
    int user = rand_int(num_users, distrib, geo_param);
    struct Node *user_node = (struct Node *) findNode(multicast->users, user)->data;
    mult_update(multicast, user_node);
    //pretty_traverse_tree(((struct LBBT *)multicast->tree)->root, 0, &printIntLine);
    return 1;
  } else {
    int user = rand_int(num_users, distrib, geo_param);    
    printf("user: %d\n", user);
    void *user_node = findAndRemoveNode(multicast->users, user);
    int *remdata = (int *) mult_rem(multicast, user_node);
    //pretty_traverse_tree(((struct LBBT *)multicast->tree)->root, 0, &printIntLine);
    printf("rem: %d\n", *remdata);
    free(remdata);
    return 2;
  }
}

int main()
{
  int n, k, lbbt_adds, lbbt_trunc, btree_adds, btree_deg, distrib;
  float add_wt, upd_wt, geo_param;

  srand(time(0));
  
  printf("Please enter your sequence options: ([init group size] [num ops] [add weight] [update weight])\n");
  scanf("%d", &n);
  scanf("%d", &k);
  scanf("%f", &add_wt);
  scanf("%f", &upd_wt);
  if(add_wt + upd_wt > 1 || add_wt < 0 || upd_wt < 0) {
    perror("Weights must not be negative nor sum to greater than 1");
    exit(1);
  }
  //rem_wt = 1 - add_wt - upd_wt;
  
  printf("n: %d, k: %d, aw: %f, uw: %f\n", n, k, add_wt, upd_wt);
  
  printf("For LBBTs, select how you would like additions to be performed: (0: greedy, 1: random, 2: append, 3: compare all)\n Also how you would like truncation to be performed: (0: truncate, 1: keep, 2: compare all)\n");
  scanf("%d", &lbbt_adds);
  scanf("%d", &lbbt_trunc);
  
  printf("For B trees, select how you would like additions to be performed: (0: greedy, 1: random, 3: compare both); also select the degree of the tree: (3: 3, 4: 4, 0: compare both)\n");
  scanf("%d", &btree_adds);
  scanf("%d", &btree_deg);
  printf("lbbt adds: %d, lbbt trunc: %d, btree adds: %d, btree deg: %d\n", lbbt_adds, lbbt_trunc, btree_adds, btree_deg);
  
  printf("What would you like the distribution used for choosing users to perform operations (and for the user to be removed, if the operation is a removal) to be: (0: uniform, 1: geometric)\n");
  scanf("%d", &distrib);
  printf("distribution: %d\n", distrib);
  if(distrib == 1) {
    printf("What would you like the p parameter to be set to?\n");
    scanf("%f", &geo_param);
    if(geo_param >= 1 || geo_param <=0) {
      perror("p has to be 0 < p < 1");
      exit(1);
    }
    printf("%f\n", geo_param);
  }
  
  
  struct List *users = malloc(sizeof(struct List));
  if (users == NULL) {
    perror("malloc returned NULL");
    return -1;
  }
  initList(users); // new users added to front
  
  void **ids = malloc(sizeof(void *) * n);
  if (ids == NULL) {
    perror("malloc returned NULL");
    return -1;
  }
  
  int i;
  int *max_id = malloc(sizeof(int));
  if (max_id == NULL) {
    perror("malloc returned NULL");
    return -1;
  }
  
  for (i = 0; i < n; i++) {
    int *data = malloc(sizeof(int));
    if (data == NULL) {
      perror("malloc returned NULL");
      return -1;
    }
    *data = i;
    *(ids + i) = (void *) data;    
  }
  struct LBBT *lbbt = lbbt_init(ids, n, lbbt_adds, lbbt_trunc, users);
  *max_id = n-1;
  
  int *counts = malloc(sizeof(int) * 1); //TODO: just counting encs for now
  if (counts == NULL) {
    perror("malloc returned NULL");
    return -1;
  }
  *counts = 0;
  
  struct Multicast *lbbt_multicast = malloc(sizeof(struct Multicast));
  if (lbbt_multicast == NULL) {
    perror("malloc returned NULL");
    return -1;
  }
  struct Multicast multicast = { 1, users, (void *) lbbt, counts, 0 }; //TODO: fix this
  *lbbt_multicast = multicast;

  //pretty_traverse_tree(((struct LBBT *)lbbt_multicast->tree)->root, 0, &printIntLine);

  int ops[3] = { 0, 0, 0 };
  
  for (i=0; i<k; i++) {
    ops[next_op(lbbt_multicast, add_wt, upd_wt, distrib, geo_param, max_id)]++;
  }

  printf("# adds: %d, # updates: %d, # rems %d\n", ops[0], ops[1], ops[2]);

  destroy_tree(((struct LBBT *) lbbt_multicast->tree)->root);
  removeAllNodes(((struct LBBT *) lbbt_multicast->tree)->blanks);
  free(((struct LBBT *) lbbt_multicast->tree)->blanks);
  free(lbbt_multicast->tree);

  removeAllNodes(users);
  free(users);

  free(ids);
  free(counts);
  free(lbbt_multicast);

  free(max_id);

  return 0;
}
