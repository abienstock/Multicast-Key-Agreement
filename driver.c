#include <stdio.h>
#include <stdlib.h>

int main()
{
	int n, k, lbbt_adds, lbbt_trunc, btree_adds, btree_deg, distrib;
	float add_wt, upd_wt, rem_wt, geo_param;
	printf("Please enter your sequence options: ([init group size] [num ops] [add weight] [update weight])\n");
	scanf("%d", &n);
	scanf("%d", &k);
	scanf("%f", &add_wt);
	scanf("%f", &upd_wt);
	if(add_wt + upd_wt > 1 || add_wt < 0 || upd_wt < 0) {
		perror("Weights must not be negative nor sum to greater than 1");
		exit(1);
	}
	rem_wt = 1 - add_wt - upd_wt;

	printf("n: %d, k: %d, aw: %f, uw: %f, rw: %f\n", n, k, add_wt, upd_wt, rem_wt);

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
}