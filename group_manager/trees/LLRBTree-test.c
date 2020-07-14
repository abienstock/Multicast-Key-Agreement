#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#include "../../utils.h"
#include "trees.h"

static struct Node *traceSkeleton(struct SkeletonNode *skeleton) {
    assert(skeleton != NULL, "NULL skeleton.");
    if (skeleton->node->num_children == 0) {
        return skeleton->node;
    }
    if (skeleton->children_color[0] == 0) {
        return traceSkeleton(skeleton->children[0]);
    }
    if (skeleton->children_color[1] == 0) {
        return traceSkeleton(skeleton->children[1]);
    }
    assert(false, "trace failed.");
    return NULL;
}

static void printTree(struct Node *root, int depth) {
    for (int i = 0; i < depth; ++i) {
        printf("___");
    }
    printf("\\__");
    struct NodeData *dataNode = (struct NodeData *) root->data;
    struct LLRBTreeNodeData *data = (struct LLRBTreeNodeData *) dataNode->tree_node_data;
    const char colors[] = {'B', 'R'};
    printf("%d (%c, %c) [%d]\n", data->heightBlack, colors[data->colorL], colors[data->colorR], dataNode->id);
    if (root->num_children == 0) {
        return;
    }
    printTree(root->children[0], depth + 1);
    printTree(root->children[1], depth + 1);
}

static void LLRBTree_test(int add_strat, int mode_order, int n, int T, int verbose) {
    int *ids = malloc_check(sizeof(int) * n);
    for (int i = 0; i < n; ++i) {
        ids[i] = i;
    }
    struct List users;
    initList(&users);
    struct InitRet resultInit = LLRBTree_init(ids, n, add_strat, mode_order, &users);
    void *tree = resultInit.tree;
    assert(((struct LLRBTree *) tree)->root->num_leaves == users.len, "incorrect leaves.");
    if (verbose >= 1) printf("init: %d\n", n);
    if (verbose >= 3) printTree(((struct LLRBTree *) tree)->root, 0);
    int id = n;
    for (int t = 0; t < T; ++t) {
        int addN = rand() % (n * 2 - users.len);
        if (verbose >= 1) printf("add count: %d\n", addN);
        for (int i = 0; i < addN; ++i) {
            struct AddRet resultAdd = LLRBTree_add(tree, id++);
            addAfter(&users, users.tail, (void *) resultAdd.added);
            assert(traceSkeleton(resultAdd.skeleton) == resultAdd.added, "incorrect trace.");
            assert(((struct LLRBTree *) tree)->root->num_leaves == users.len, "incorrect leaves.");
            if (verbose >= 2) printf("add: %d\n", id);
            if (verbose >= 3) printTree(((struct LLRBTree *) tree)->root, 0);
        }
        int removeN = rand() % (users.len - 1);
        if (verbose >= 1) printf("remove count: %d\n", removeN);
        for (int i = 0; i < removeN; ++i) {
            struct Node *nodeRemove = (struct Node *) findAndRemoveNode(&users, rand() % users.len);
            struct RemRet resultRemove = LLRBTree_rem(tree, nodeRemove);
            assert(((struct LLRBTree *) tree)->root->num_leaves == users.len, "incorrect leaves.");
            if (verbose >= 2) printf("remove: %d\n", resultRemove.id);
            if (verbose >= 3) printTree(((struct LLRBTree *) tree)->root, 0);
        }
    }
}

int main() {
    srand(time(NULL));

    int add_strat_list[] = {LLRBTree_STRAT_GREEDY, LLRBTree_STRAT_RANDOM};
    int mode_order_list[] = {LLRBTree_MODE_23};//{LLRBTree_MODE_23, LLRBTree_MODE_234};
    for (int i = 0; i < sizeof(add_strat_list) / sizeof(int); ++i) {
        int add_strat = add_strat_list[i];
        for (int i = 0; i < sizeof(mode_order_list) / sizeof(int); ++i) {
            int mode_order = mode_order_list[i];
            printf("test small RB tree (strat: %d, mode: %d)\n", add_strat, mode_order);
            LLRBTree_test(add_strat, mode_order, 30, 1000, 0);
            printf("test RB tree (strat: %d, mode: %d)\n", add_strat, mode_order);
            LLRBTree_test(add_strat, mode_order, 100, 100, 0);
            printf("test large RB tree (strat: %d, mode: %d)\n", add_strat, mode_order);
            LLRBTree_test(add_strat, mode_order, 3000, 100, 0);
        }
    }
}
