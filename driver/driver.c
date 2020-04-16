#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "../group_manager/trees/trees.h"
#include "../group_manager/trees/ll.h"
#include "../group_manager/multicast/multicast.h"

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
    addFront(multicast->users, mult_add(multicast, *max_id));
    return 0;
  } else if (operation < add_wt + upd_wt) {
    int user = rand_int(num_users, distrib, geo_param);
    struct Node *user_node = (struct Node *) findNode(multicast->users, user)->data;
    printf("upd: %d\n", *((int *)user_node->data));
    mult_update(multicast, user_node);
    //pretty_traverse_tree(((struct LBBT *)multicast->tree)->root, 0, &printIntLine);
    return 1;
  } else {
    int user = rand_int(num_users, distrib, geo_param);    
    printf("user: %d\n", user);
    struct Node *user_node = (struct Node *) findAndRemoveNode(multicast->users, user);
    struct NodeData *user_data = (struct NodeData *) user_node->data;
    printf("rem: %d\n", user_data->id);
    mult_rem(multicast, user_node);
    return 2;
  }
}

int main(int argc, char *argv[]) {
  int n, distrib;
  float add_wt, upd_wt, geo_param;

  srand(time(0));

  if (argc != 10 && argc != 11) {
    printf("Usage: [init group size] [num ops] [add weight] [update weight] [LBBT addition strategy] [LBBT truncation strategy] [B-tree addition strategy] [B-tree degree] [operation distribution] [geometric distribution parameter] \n\nParameter Description:\n[init group size]: number of users in group at initialization\n[num ops]: number of group operations\n[add weight]: probability that a given operation will be an add (in [0,1])\n[update weight]: probability that a given operation will be an update (in [0,1]). Note: add weight + update weight must be in [0,1], remove weight = 1 - add weight - remove weight (probability that a given operation will be a remove)\n[LBBT addition strategy]: The strategy used to add users in an LBBT tree -- 0 = greedy, 1 = random, 2 = append, 3 = compare all\n[LBBT truncation strategy]: The strategy used to truncate the LBBT tree after removals -- 0 = truncate, 1 = keep, 2 = compare all\n[B-tree addition strategy]: The strategy used to add users in B trees -- 0 = greedy, 1 = random, 2 = compare both\n[B-tree degree]: Degree of the B-tree -- 3 = 3, 4 = 4, 0 = compare both\n[operation distribution]: The distribution for choosing which users to perform operations (and for the user to be removed, if the operation is a removal) -- 0 = uniform, 1 = geometric\n[geometric distribution parameter]: The p parameter of the geometric distribution, if chosen\n");
    exit(0);
  }

  n = atoi(argv[1]);
  distrib = atoi(argv[9]);
  add_wt = atof(argv[3]);
  upd_wt = atof(argv[4]);

  if(add_wt + upd_wt > 1 || add_wt < 0 || upd_wt < 0) {
    perror("Weights must not be negative nor sum to greater than 1");
    exit(1);
  }
  //rem_wt = 1 - add_wt - upd_wt;
  
  geo_param = 0;
  if(distrib == 1) {
    geo_param = atof(argv[10]);
    if(geo_param >= 1 || geo_param <=0) {
      perror("p has to be 0 < p < 1");
      exit(1);
    }
  }
  
  int lbbt_flags[2] = { atoi(argv[5]), atoi(argv[6]) };

  int *max_id = malloc(sizeof(int));
  if (max_id == NULL) {
    perror("malloc returned NULL");
    return -1;
  }
  *max_id = n-1;

  struct Multicast *lbbt_multicast = mult_init(n, lbbt_flags, 0);

  int ops[3] = { 0, 0, 0 };

  int i;
  for (i = 0; i < atoi(argv[2]); i++) {
    ops[next_op(lbbt_multicast, add_wt, upd_wt, distrib, geo_param, max_id)]++;
  }

  printf("# adds: %d, # updates: %d, # rems %d\n", ops[0], ops[1], ops[2]);
  printf("# PRGs: %d, # encs: %d\n", *(lbbt_multicast->counts), *(lbbt_multicast->counts + 1));

  mult_destroy(lbbt_multicast);
  free(max_id);

  return 0;
}
