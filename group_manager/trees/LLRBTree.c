#include <stdlib.h>
#include <math.h>

#include "../../utils.h"
#include "trees.h"

// LLRB types

typedef struct Node SNode;
typedef struct NodeData SNodeData;
typedef enum LLRBTreeColor Color;
typedef struct LLRBTreeNodeData SLLRBTreeData;

static SLLRBTreeData *getData(SNode *node) {
    return (SLLRBTreeData *) ((SNodeData *) node->data)->tree_node_data;
}

static SNode *LLRBTree_new(int id, SNode *childL, Color colorL, SNode *childR, Color colorR) {
    SNode *node = malloc_check(sizeof(SNode));
    node->data = malloc_check(sizeof(SNodeData));
    ((SNodeData *) node->data)->id = id;
    ((SNodeData *) node->data)->key = NULL;
    ((SNodeData *) node->data)->seed = NULL;
    ((SNodeData *) node->data)->tree_node_data = malloc_check(sizeof(SLLRBTreeData));
    node->parent = NULL;
    if (childL == NULL) {
        assert(childR == NULL, "LLRB: not binary.");
        node->num_children = 0;
        node->children = NULL;
        node->num_leaves = 1;
        assert(colorL == BLACK && colorR == BLACK, "LLRB: red NULL.");
        getData(node)->colorL = colorL;
        getData(node)->colorR = colorR;
        getData(node)->num_optimal = 1;
        getData(node)->heightBlack = 0;
        return node;
    }
    node->num_children = 2;
    node->children = malloc_check(sizeof(SNode * [2]));
    node->children[0] = childL;
    childL->parent = node;
    node->children[1] = childR;
    childR->parent = node;
    node->num_leaves = childL->num_leaves + childR->num_leaves;
    assert(colorL == RED || colorR == BLACK, "LLRB: invalid color.");
    if (colorL == RED) {
        assert(childL->num_children == 2 && getData(childL)->colorL == BLACK && getData(childL)->colorR == BLACK, "LLRB: invalid red node (left).");
    }
    if (colorR == RED) {
        assert(childR->num_children == 2 && getData(childR)->colorL == BLACK && getData(childR)->colorR == BLACK, "LLRB: invalid red node (right).");
    }
    getData(node)->colorL = colorL;
    getData(node)->colorR = colorR;
    if (childL->num_leaves < childR->num_leaves) {
        getData(node)->num_optimal = getData(childL)->num_optimal;
    } else if (childL->num_leaves > childR->num_leaves) {
        getData(node)->num_optimal = getData(childR)->num_optimal;
    } else {
        getData(node)->num_optimal = getData(childL)->num_optimal + getData(childR)->num_optimal;
    }
    int heightBlackL = getData(childL)->heightBlack + (int)(colorL == BLACK), heightBlackR = getData(childR)->heightBlack + (int)(colorR == BLACK);
    assert(heightBlackL == heightBlackR, "LLRB: unbalanced.");
    getData(node)->heightBlack = heightBlackL;
    return node;
}

// LLRB utils

static Color getColorWithHint(SNode *node, SNode *parent) {
    if (parent == NULL) {
        return BLACK;
    }
    if (parent->children[0] == node) {
        return getData(parent)->colorL;
    } else if (parent->children[1] == node) {
        return getData(parent)->colorR;
    } else {
        assert(false, "LLRB: broken parent-child relationship (@color).");
        return 0;
    }
}
static Color getColor(SNode *node) {
    return getColorWithHint(node, node->parent);
}

static SNode *getRoot(SNode *node) {
    SNode *parent = node->parent;
    if (parent == NULL) {
        return node;
    }
    return getRoot(parent);
}
static SNode *getSibling(SNode *node) {
    SNode *parent = node->parent;
    assert(parent != NULL, "LLRB: invalid access to sibling.");
    if (parent->children[0] == node) {
        return parent->children[1];
    } else if (parent->children[1] == node) {
        return parent->children[0];
    } else {
        assert(false, "LLRB: broken parent-child relationship (@sibling).");
        return NULL;
    }
}

static SNode *getChildL(SNode *node) {
    if (node->num_children == 0) {
        return NULL;
    }
    return node->children[0];
}
static SNode *getChildR(SNode *node) {
    if (node->num_children == 0) {
        return NULL;
    }
    return node->children[1];
}
static SNode *getRandomLeaf(SNode *node) {
    if (node->num_children == 0) {
        return node;
    }
    int r = rand() % node->num_leaves;
    if (r < node->children[0]->num_leaves) {
        return getRandomLeaf(node->children[0]);
    } else {
        return getRandomLeaf(node->children[1]);
    }
}

static void LLRBTree_replaceChild(SNode *parent, SNode *node, SNode *nodeReplace) {
    nodeReplace->parent = parent;
    if (parent == NULL) {
        return;
    }
    assert(getData(nodeReplace)->heightBlack == getData(node)->heightBlack, "LLRB: unbalanced replace.");
    if (parent->children[0] == node) {
        parent->children[0] = nodeReplace;
    } else if (parent->children[1] == node) {
        parent->children[1] = nodeReplace;
    } else {
        assert(false, "LLRB: broken parent-child relationship (@replace).");
    }
    do {
        parent->num_leaves = parent->children[0]->num_leaves + parent->children[1]->num_leaves;
        if (parent->children[0]->num_leaves < parent->children[1]->num_leaves) {
            getData(parent)->num_optimal = getData(parent->children[0])->num_optimal;
        } else if (parent->children[0]->num_leaves > parent->children[1]->num_leaves) {
            getData(parent)->num_optimal = getData(parent->children[1])->num_optimal;
        } else {
            getData(parent)->num_optimal = getData(parent->children[0])->num_optimal + getData(parent->children[1])->num_optimal;
        }
        parent = parent->parent;
    } while (parent != NULL);
}
static void LLRBTree_replaceSelf(SNode *node, SNode *nodeReplace) {
    LLRBTree_replaceChild(node->parent, node, nodeReplace);
}

// skeleton types

typedef struct SkeletonNode SSkeletonNode;

static SSkeletonNode *Skeleton_new(SNode *node, SSkeletonNode *childL, SSkeletonNode *childR, SNode *childSpecial) {
    SSkeletonNode *skeleton = malloc_check(sizeof(SSkeletonNode));
    skeleton->ciphertexts = NULL;
    assert(node != NULL, "Skeleton: invalid skeleton of NULL.");
    skeleton->node = node;
    skeleton->node_id = ((SNodeData *) node->data)->id;
    skeleton->parent = NULL;
    if (node->num_children == 0) {
        assert(childL == NULL && childR == NULL && childSpecial == NULL, "Skeleton: invalid child of leaf.");
        skeleton->children = NULL;
        skeleton->children_color = NULL;
        return skeleton;
    }
    skeleton->children = malloc_check(sizeof(SSkeletonNode * [2]));
    skeleton->children[0] = childL;
    if (childL != NULL) {
        childL->parent = skeleton;
    }
    skeleton->children[1] = childR;
    if (childR != NULL) {
        childR->parent = skeleton;
    }
    skeleton->children_color = malloc_check(sizeof(int [2]));
    skeleton->children_color[0] = (int)(childL == NULL || childL->node != childSpecial);
    skeleton->children_color[1] = (int)(childR == NULL || childR->node != childSpecial);
    assert(skeleton->children_color[0] + skeleton->children_color[1] > 0, "Skeleton: multiple special child.");
    return skeleton;
}

// skeleton utils

static SSkeletonNode *Skeleton_newInternal(SNode *node, SSkeletonNode *childL, SSkeletonNode *childR, SNode *childSpecial) { // only create internal skeleton node i.e. only create skeleton node if either child is not NULL
    if (childL == NULL && childR == NULL) {
        return NULL;
    }
    return Skeleton_new(node, childL, childR, childSpecial);
}

static SSkeletonNode *Skeleton_wholeDirectPath(SSkeletonNode *skeleton) {
    assert(skeleton != NULL, "Skeleton: invalid access to direct path.");
    SNode *node = skeleton->node;
    SNode *parent = node->parent;
    if (parent == NULL) {
        return skeleton;
    }
    assert(node == parent->children[0] || node == parent->children[1], "LLRB: broken parent-child relationship (@directpath).");
    return Skeleton_wholeDirectPath(Skeleton_new(parent,
        node == parent->children[0] ? skeleton : NULL,
        node == parent->children[1] ? skeleton : NULL,
    node));
}

static SSkeletonNode *Skeleton_wholeTree(SNode *root) {
    if (root == NULL) {
        return NULL;
    }
    return Skeleton_new(root,
        Skeleton_wholeTree(getChildL(root)),
        Skeleton_wholeTree(getChildR(root)),
    getChildL(root)); // @Note: Prefer left child as it is deeper.
}

// LLRB recursive operations

typedef struct {
    SNode *node;
    SSkeletonNode *skeleton;
} TreeResult;

static TreeResult LLRBTree_addSibling_23(SNode *node, SNode *nodeAdd, SSkeletonNode *skeletonAdd, SNode *nodeReplace, SSkeletonNode *skeletonReplace) {
    SNode *parent = node->parent;
    if (parent == NULL) { // B1 + B' => B'' -> B1 B'
        SNode *nodeNew = LLRBTree_new(rand(), nodeReplace != NULL ? nodeReplace : node, BLACK, nodeAdd, BLACK);
        TreeResult result = {
            nodeNew,
            Skeleton_new(nodeNew, skeletonReplace, skeletonAdd, nodeAdd),
        };
        return result;
    }
    if (getColor(parent) == BLACK) {
        SNode *sibling = getSibling(node);
        if (getColor(sibling) == BLACK) { // B1 -> B2* B3* + B' => B'' -> (R1 -> B2 B3) B'
            if (nodeReplace != NULL) {
                LLRBTree_replaceChild(parent, node, nodeReplace); // @Note: `nodeReplace` inherits the color `BLACK` of `node`.
            }
            SNode *grandparent = parent->parent; // @Note: Should cache `grandparent` before `parent` becomes child of `nodeNew`.
            SNode *nodeNew = LLRBTree_new(rand(), parent, RED, nodeAdd, BLACK);
            LLRBTree_replaceChild(grandparent, parent, nodeNew);
            TreeResult result = {
                getRoot(nodeNew),
                Skeleton_wholeDirectPath(Skeleton_new(nodeNew,
                    Skeleton_newInternal(parent,
                        nodeReplace == parent->children[0] ? skeletonReplace : NULL,
                        nodeReplace == parent->children[1] ? skeletonReplace : NULL,
                    nodeReplace),
                    skeletonAdd,
                nodeAdd)),
            };
            return result;
        } else { // B1 -> (R2 -> B4 B5) B3* + B' => B2 -> B4 B5 + B'' -> B3 B'
            assert(node == parent->children[1], "LLRB: invalid (black,red) children.");
            SNode *nodeNew = LLRBTree_new(rand(), nodeReplace != NULL ? nodeReplace : node, BLACK, nodeAdd, BLACK);
            return LLRBTree_addSibling_23(parent, nodeNew, Skeleton_new(nodeNew, skeletonReplace, skeletonAdd, nodeAdd), sibling, NULL); // @Note: The color is automatically flipped as `nodeReplace` is always inserted into the tree with color `BLACK` in every branch.
        }
    } else { // B1 -> (R2 -> B4* B5*) B3 + B' => B2 -> B4 B5 + B'' -> B3 B'
        SNode *grandparent = parent->parent;
        assert(grandparent != NULL, "LLRB: invalid red root.");
        /* for comparing with 2-3-4 mode more clearly, it is equivalent to hoist the following code snippet: */
        // if (nodeReplace != NULL) {
        //     LLRBTree_replaceChild(parent, node, nodeReplace); // @Note: `nodeReplace` inherits the color `BLACK` of `node`.
        // }
        // SSkeletonNode *skeletonParent = Skeleton_newInternal(parent,
        //     nodeReplace == parent->children[0] ? skeletonReplace : NULL,
        //     nodeReplace == parent->children[1] ? skeletonReplace : NULL,
        // nodeReplace);
        /* end hoist */
        assert(parent == grandparent->children[0], "LLRB: invalid red right child.");
        SNode *parentSibling = grandparent->children[1];
        assert(getColor(parentSibling) == BLACK, "LLRB: invalid red right sibling.");
        SNode *nodeNew = LLRBTree_new(rand(), parentSibling, BLACK, nodeAdd, BLACK);
        if (nodeReplace != NULL) {
            LLRBTree_replaceChild(parent, node, nodeReplace); // @Note: `nodeReplace` inherits the color `BLACK` of `node`.
        }
        return LLRBTree_addSibling_23(grandparent, nodeNew, Skeleton_new(nodeNew, NULL, skeletonAdd, nodeAdd), parent, Skeleton_newInternal(parent,
            nodeReplace == parent->children[0] ? skeletonReplace : NULL,
            nodeReplace == parent->children[1] ? skeletonReplace : NULL,
        nodeReplace)); // @Note: The color is automatically flipped as `nodeReplace` is always inserted into the tree with color `BLACK` in every branch.
    }
}
static TreeResult LLRBTree_addSibling_234(SNode *node, SNode *nodeAdd, SSkeletonNode *skeletonAdd, SNode *nodeReplace, SSkeletonNode *skeletonReplace) {
/****************************************
 * begin same code as 2-3 mode
 ****************************************/
    SNode *parent = node->parent;
    if (parent == NULL) { // B1 + B' => B'' -> B1 B'
        SNode *nodeNew = LLRBTree_new(rand(), nodeReplace != NULL ? nodeReplace : node, BLACK, nodeAdd, BLACK);
        TreeResult result = {
            nodeNew,
            Skeleton_new(nodeNew, skeletonReplace, skeletonAdd, nodeAdd),
        };
        return result;
    }
    if (getColor(parent) == BLACK) {
        SNode *sibling = getSibling(node);
        if (getColor(sibling) == BLACK) { // B1 -> B2* B3* + B' => B'' -> (R1 -> B2 B3) B'
            if (nodeReplace != NULL) {
                LLRBTree_replaceChild(parent, node, nodeReplace); // @Note: `nodeReplace` inherits the color `BLACK` of `node`.
            }
            SNode *grandparent = parent->parent; // @Note: Should cache `grandparent` before `parent` becomes child of `nodeNew`.
            SNode *nodeNew = LLRBTree_new(rand(), parent, RED, nodeAdd, BLACK);
            LLRBTree_replaceChild(grandparent, parent, nodeNew);
            TreeResult result = {
                getRoot(nodeNew),
                Skeleton_wholeDirectPath(Skeleton_new(nodeNew,
                    Skeleton_newInternal(parent,
                        nodeReplace == parent->children[0] ? skeletonReplace : NULL,
                        nodeReplace == parent->children[1] ? skeletonReplace : NULL,
                    nodeReplace),
                    skeletonAdd,
                nodeAdd)),
            };
            return result;
        } else { // B1 -> (R2 -> B4 B5) B3* + B' => B2 -> B4 B5 + B'' -> B3 B'
            assert(node == parent->children[1], "LLRB: invalid (black,red) children.");
            SNode *nodeNew = LLRBTree_new(rand(), nodeReplace != NULL ? nodeReplace : node, BLACK, nodeAdd, BLACK);
            return LLRBTree_addSibling_234(parent, nodeNew, Skeleton_new(nodeNew, skeletonReplace, skeletonAdd, nodeAdd), sibling, NULL); // @Note: The color is automatically flipped as `nodeReplace` is always inserted into the tree with color `BLACK` in every branch.
        }
    } else {
        SNode *grandparent = parent->parent;
        assert(grandparent != NULL, "LLRB: invalid red root.");
        if (nodeReplace != NULL) {
            LLRBTree_replaceChild(parent, node, nodeReplace); // @Note: `nodeReplace` inherits the color `BLACK` of `node`.
        }
        SSkeletonNode *skeletonParent = Skeleton_newInternal(parent,
            nodeReplace == parent->children[0] ? skeletonReplace : NULL,
            nodeReplace == parent->children[1] ? skeletonReplace : NULL,
        nodeReplace);
/****************************************
 * end same code as 2-3 mode
 ****************************************/
        SNode *parentSibling = getSibling(parent);
        if (getColor(parentSibling) == BLACK) { // B1 -> (R2 -> B4* B5*) B3 => B''' -> (R2 -> B4 B5) (R'' -> B3 B')
            assert(parent == grandparent->children[0], "LLRB: invalid red right child.");
            SNode *nodeNew = LLRBTree_new(rand(), parentSibling, BLACK, nodeAdd, BLACK);
            SNode *grandparentNew = LLRBTree_new(rand(), parent, RED, nodeNew, RED);
            LLRBTree_replaceSelf(grandparent, grandparentNew);
            TreeResult result = {
                getRoot(grandparentNew),
                Skeleton_wholeDirectPath(Skeleton_new(grandparentNew,
                    skeletonParent,
                    Skeleton_new(nodeNew,
                        NULL,
                        skeletonAdd,
                    nodeAdd),
                nodeNew)),
            };
            return result;
        } else { // B1 -> (R2 -> B4* B5*) (R3 -> B6* B7*) => (B2 -> B4 B5) (B'' (R3 -> B6 B7) B') or 2,4,5 <-> 3,6,7
            SNode *nodeNew = LLRBTree_new(rand(), parent, RED, nodeAdd, BLACK);
            return LLRBTree_addSibling_234(grandparent, nodeNew, Skeleton_new(nodeNew, skeletonParent, skeletonAdd, nodeAdd), parentSibling, NULL); // @Note: The color is automatically flipped as `nodeReplace` is always inserted into the tree with color `BLACK` in every branch.
        }
    }
}

static TreeResult LLRBTree_removeSelf_23(SNode *node, SNode *nodeToReplace, SNode *nodeReplace, SSkeletonNode *skeletonReplace) {
    // @Warn: Need to use `getColorWithHint` instead of `getColor` when querying the color of certain siblings due to broken parent-child relationship of certain sibling; see the @Warn at the end of this function.
    SNode *parent = node->parent;
    assert(parent != NULL, "LLRB: removing last node.");
    SNode *sibling = getSibling(node);
    if (getColor(parent) == RED) { // B1 -> (R2 -> B4* B5*) B3 => B' -> B3 B4 or 4 <-> 5
        SNode *grandparent = parent->parent;
        assert(grandparent != NULL, "LLRB: invalid red root.");
        assert(parent == grandparent->children[0], "LLRB: invalid red right child.");
        SNode *parentSibling = grandparent->children[1];
        assert(getColorWithHint(parentSibling, grandparent) == BLACK, "LLRB: invalid red right sibling.");
        assert(nodeToReplace == NULL || nodeToReplace == parentSibling || nodeToReplace == sibling, "LLRB: node to replace is not sibling.");
        SNode *nodeNew = LLRBTree_new(rand(), nodeToReplace == parentSibling ? nodeReplace : parentSibling, BLACK, nodeToReplace == sibling ? nodeReplace : sibling, BLACK);
        LLRBTree_replaceSelf(grandparent, nodeNew);
        TreeResult result = {
            getRoot(nodeNew),
            Skeleton_wholeDirectPath(Skeleton_new(nodeNew,
                nodeReplace == nodeNew->children[0] ? skeletonReplace : NULL,
                nodeReplace == nodeNew->children[1] ? skeletonReplace : NULL,
            nodeReplace)),
        };
        return result;
    } else {
        if (getColorWithHint(sibling, parent) == RED) { // B1 -> (R2 -> B4 B5) B3* => B2 -> B4 B5
            assert(sibling == parent->children[0], "LLRB: invalid right red sibling.");
            if (nodeReplace != NULL) {
                LLRBTree_replaceChild(sibling, nodeToReplace, nodeReplace);
            }
            LLRBTree_replaceSelf(parent, sibling); // @Note: The color is automatically flipped as `sibling` inherits the color `BLACK` of `parent`.
            SSkeletonNode *skeletonSibling = Skeleton_newInternal(sibling,
                nodeReplace == sibling->children[0] ? skeletonReplace : NULL,
                nodeReplace == sibling->children[1] ? skeletonReplace : NULL,
            nodeReplace);
            if (skeletonSibling == NULL) {
                SNode *grandparent = sibling->parent;
                if (grandparent == NULL) {
                    TreeResult result = {
                        sibling, // @Note: == getRoot(sibling)
                        NULL,
                    };
                    return result;
                }
                TreeResult result = {
                    getRoot(grandparent),
                    Skeleton_wholeDirectPath(Skeleton_new(grandparent,
                        NULL,
                        NULL,
                    sibling)), // @Note: `sibling` is equivalent to NULL and both children are NULL.
                };
                return result;
            }
            TreeResult result = {
                getRoot(sibling),
                Skeleton_wholeDirectPath(skeletonSibling),
            };
            return result;
        } else { // B1 -> B2* B3* => shrink or borrow or merge
            assert(nodeToReplace == NULL || nodeToReplace == sibling, "LLRB: node to replace is not sibling.");
            SNode *grandparent = parent->parent;
            if (grandparent == NULL) { // shrink: B1 -> B2* B3* => B2 or 2 <-> 3
                if (nodeToReplace == NULL) {
                    sibling->parent = NULL;
                }
                TreeResult result = {
                    nodeToReplace != NULL ? nodeReplace : sibling,
                    nodeToReplace != NULL ? skeletonReplace : NULL,
                };
                return result;
            }
            SNode *parentSiblingsIsomorph[2] = {NULL, NULL};
            SNode *grandparentIsomorph = grandparent;
            {
                SNode *parentSibling = getSibling(parent);
                if (getColorWithHint(parentSibling, grandparent) == BLACK) {
                    parentSiblingsIsomorph[0] = parentSibling;
                    if (getColor(grandparent) == RED) {
                        parentSiblingsIsomorph[1] = getSibling(grandparent);
                        grandparentIsomorph = grandparent->parent;
                    }
                } else {
                    parentSiblingsIsomorph[0] = parentSibling->children[0];
                    parentSiblingsIsomorph[1] = parentSibling->children[1];
                }
            }
            for (int i = 0; i < 2; ++i) { // try borrow cousin from some parent's sibling (under isomorphism): B1 -> B2* B3* ++ B4 -> (R5 -> B7 B8) RB6) => B' -> RB6 B2 ++ B5 -> B7 B8 or 2 <-> 3
                if (parentSiblingsIsomorph[i] == NULL) {
                    continue;
                }
                SNode *parentSibling = parentSiblingsIsomorph[i];
                if (getColor(parentSibling->children[0]) != RED) {
                    continue;
                }
                assert(getColor(parentSibling->children[1]) == BLACK, "LLRB: invalid red right child.");
                SNode *parentNew = LLRBTree_new(rand(), parentSibling->children[1], BLACK, nodeToReplace != NULL ? nodeReplace : sibling, BLACK);
                SNode *grandparentNew = LLRBTree_new(rand(), parentNew, BLACK, parentSibling->children[0], BLACK);
                SSkeletonNode *skeletonGrandparentNew = Skeleton_new(grandparentNew,
                    Skeleton_new(parentNew,
                        NULL,
                        nodeToReplace != NULL ? skeletonReplace : NULL,
                    nodeReplace),
                    NULL,
                parentNew);
                SNode *parentSiblingOther = parentSiblingsIsomorph[1 - i];
                if (parentSiblingOther == NULL) {
                    LLRBTree_replaceSelf(grandparentIsomorph, grandparentNew);
                    TreeResult result = {
                        getRoot(grandparentNew),
                        Skeleton_wholeDirectPath(skeletonGrandparentNew),
                    };
                    return result;
                } else {
                    SNode *nodeNew = LLRBTree_new(rand(), grandparentNew, RED, parentSiblingOther, BLACK);
                    LLRBTree_replaceSelf(grandparentIsomorph, nodeNew);
                    TreeResult result = {
                        getRoot(nodeNew),
                        Skeleton_wholeDirectPath(Skeleton_new(nodeNew,
                            skeletonGrandparentNew,
                            NULL,
                        grandparentNew)),
                    };
                    return result;
                }
            }
            SNode *parentSibling = parentSiblingsIsomorph[0]; // if fail to borrow then merge into parent's arbirtary sibling (under isomorphism): B1 -> B2* B3* ++ B4 -> B5 B6 => B' -> (R4 -> B5 B6) B2 or 2 <-> 3
            SNode *nodeNew = LLRBTree_new(rand(), parentSibling, RED, nodeToReplace != NULL ? nodeReplace : sibling, BLACK);
            return LLRBTree_removeSelf_23(parent, parentSibling, nodeNew, Skeleton_new(nodeNew,
                NULL,
                nodeToReplace != NULL ? skeletonReplace : NULL,
            nodeReplace)); // @Warn: `parentSibling` becomes child of `nodeNew` while being sibling (under isomorphism) of `parent` at the beginning of recursion, which forms a (temporary) broken parent-child relationship.
        }
    }
}
static TreeResult LLRBTree_removeSelf_234(SNode *node, SNode *nodeToReplace, SNode *nodeReplace, SSkeletonNode *skeletonReplace) {
/****************************************
 * begin same code as 2-3 mode
 ****************************************/
    // @Warn: Need to use `getColorWithHint` instead of `getColor` when querying the color of certain siblings due to broken parent-child relationship of certain sibling; see the @Warn at the end of this function.
    SNode *parent = node->parent;
    assert(parent != NULL, "LLRB: removing last node.");
    SNode *sibling = getSibling(node);
    if (getColor(parent) == RED) {
        SNode *grandparent = parent->parent;
        assert(grandparent != NULL, "LLRB: invalid red root.");
/****************************************
 * end same code as 2-3 mode
 ****************************************/
        SNode *parentSibling = getSibling(parent);
        if (getColorWithHint(parentSibling, grandparent) == RED) { // B1 -> (R2 -> B4* B5*) (R3 -> B6* B7*) => B' -> (R2 -> B4 B5) B6 or 2,4,5 <-> 3,6,7
            SNode *nodeNew = LLRBTree_new(rand(), parentSibling, RED, nodeToReplace == sibling ? nodeReplace : sibling, BLACK);
            if (nodeToReplace != NULL && nodeToReplace != sibling) {
                LLRBTree_replaceChild(parentSibling, nodeToReplace, nodeReplace);
            }
            LLRBTree_replaceSelf(grandparent, nodeNew);
            TreeResult result = {
                getRoot(nodeNew),
                Skeleton_wholeDirectPath(Skeleton_new(nodeNew,
                    Skeleton_newInternal(parentSibling,
                        nodeReplace == parentSibling->children[0] ? skeletonReplace : NULL,
                        nodeReplace == parentSibling->children[1] ? skeletonReplace : NULL,
                    nodeReplace),
                    nodeToReplace == sibling ? skeletonReplace : NULL,
                nodeReplace)),
            };
            return result;
/****************************************
 * begin same code as 2-3 mode
 *
 * except:
 * - `parentSiblingsIsomorph` has at most length 3 instead of 2
 * - color of "RB6" is not necessarily `BLACK`
 * and the code changes only in accordance with these differences
 ****************************************/
        } else { // B1 -> (R2 -> B4* B5*) B3 => B' -> B3 B4 or 4 <-> 5
            assert(parent == grandparent->children[0], "LLRB: invalid red right child.");
            assert(nodeToReplace == NULL || nodeToReplace == parentSibling || nodeToReplace == sibling, "LLRB: node to replace is not sibling.");
            SNode *nodeNew = LLRBTree_new(rand(), nodeToReplace == parentSibling ? nodeReplace : parentSibling, BLACK, nodeToReplace == sibling ? nodeReplace : sibling, BLACK);
            LLRBTree_replaceSelf(grandparent, nodeNew);
            TreeResult result = {
                getRoot(nodeNew),
                Skeleton_wholeDirectPath(Skeleton_new(nodeNew,
                    nodeReplace == nodeNew->children[0] ? skeletonReplace : NULL,
                    nodeReplace == nodeNew->children[1] ? skeletonReplace : NULL,
                nodeReplace)),
            };
            return result;
        }
    } else {
        if (getColorWithHint(sibling, parent) == RED) { // B1 -> (R2 -> B4 B5) B3* => B2 -> B4 B5
            assert(sibling == parent->children[0], "LLRB: invalid right red sibling.");
            if (nodeReplace != NULL) {
                LLRBTree_replaceChild(sibling, nodeToReplace, nodeReplace);
            }
            LLRBTree_replaceSelf(parent, sibling); // @Note: The color is automatically flipped as `sibling` inherits the color `BLACK` of `parent`.
            SSkeletonNode *skeletonSibling = Skeleton_newInternal(sibling,
                nodeReplace == sibling->children[0] ? skeletonReplace : NULL,
                nodeReplace == sibling->children[1] ? skeletonReplace : NULL,
            nodeReplace);
            if (skeletonSibling == NULL) {
                SNode *grandparent = sibling->parent;
                if (grandparent == NULL) {
                    TreeResult result = {
                        sibling, // @Note: == getRoot(sibling)
                        NULL,
                    };
                    return result;
                }
                TreeResult result = {
                    getRoot(grandparent),
                    Skeleton_wholeDirectPath(Skeleton_new(grandparent,
                        NULL,
                        NULL,
                    sibling)), // @Note: `sibling` is equivalent to NULL and both children are NULL.
                };
                return result;
            }
            TreeResult result = {
                getRoot(sibling),
                Skeleton_wholeDirectPath(skeletonSibling),
            };
            return result;
        } else { // B1 -> B2* B3* => shrink or borrow or merge
            assert(nodeToReplace == NULL || nodeToReplace == sibling, "LLRB: node to replace is not sibling.");
            SNode *grandparent = parent->parent;
            if (grandparent == NULL) { // shrink: B1 -> B2* B3* => B2 or 2 <-> 3
                if (nodeToReplace == NULL) {
                    sibling->parent = NULL;
                }
                TreeResult result = {
                    nodeToReplace != NULL ? nodeReplace : sibling,
                    nodeToReplace != NULL ? skeletonReplace : NULL,
                };
                return result;
            }
            SNode *parentSiblingsIsomorph[3] = {NULL, NULL, NULL};
            SNode *grandparentIsomorph = grandparent;
            {
                SNode *parentSibling = getSibling(parent);
                if (getColorWithHint(parentSibling, grandparent) == BLACK) {
                    parentSiblingsIsomorph[0] = parentSibling;
                    if (getColor(grandparent) == RED) {
                        SNode *grandgrandParent = grandparent->parent;
                        SNode *grandparentSibling = getSibling(grandparent);
                        if (getColor(grandparentSibling) == RED) {
                            parentSiblingsIsomorph[1] = grandparentSibling->children[0];
                            parentSiblingsIsomorph[2] = grandparentSibling->children[1];
                        } else {
                            parentSiblingsIsomorph[1] = grandparentSibling;
                        }
                        grandparentIsomorph = grandgrandParent;
                    }
                } else {
                    parentSiblingsIsomorph[0] = parentSibling->children[0];
                    parentSiblingsIsomorph[1] = parentSibling->children[1];
                }
            }
            for (int i = 0; i < 3; ++i) { // try borrow cousin from some parent's sibling (under isomorphism): B1 -> B2* B3* ++ B4 -> (R5 -> B7 B8) RB6) => B' -> RB6 B2 ++ B5 -> B7 B8 or 2 <-> 3
                if (parentSiblingsIsomorph[i] == NULL) {
                    continue;
                }
                SNode *parentSibling = parentSiblingsIsomorph[i];
                if (getColor(parentSibling->children[0]) != RED) {
                    continue;
                }
                SNode *parentNew = LLRBTree_new(rand(), parentSibling->children[1], getColor(parentSibling->children[1]), nodeToReplace != NULL ? nodeReplace : sibling, BLACK);
                SNode *grandparentNew = LLRBTree_new(rand(), parentNew, BLACK, parentSibling->children[0], BLACK);
                SSkeletonNode *skeletonGrandparentNew = Skeleton_new(grandparentNew,
                    Skeleton_new(parentNew,
                        NULL,
                        nodeToReplace != NULL ? skeletonReplace : NULL,
                    nodeReplace),
                    NULL,
                parentNew);
                SNode *parentSiblingOther1 = parentSiblingsIsomorph[(int)(i == 0)], *parentSiblingOther2 = parentSiblingsIsomorph[2 - (int)(i == 2)];
                if (parentSiblingOther1 == NULL && parentSiblingOther2 == NULL) {
                    LLRBTree_replaceSelf(grandparentIsomorph, grandparentNew);
                    TreeResult result = {
                        getRoot(grandparentNew),
                        Skeleton_wholeDirectPath(skeletonGrandparentNew),
                    };
                    return result;
                } else if (parentSiblingOther2 == NULL) {
                    SNode *nodeNew = LLRBTree_new(rand(), grandparentNew, RED, parentSiblingOther1, BLACK);
                    LLRBTree_replaceSelf(grandparentIsomorph, nodeNew);
                    TreeResult result = {
                        getRoot(nodeNew),
                        Skeleton_wholeDirectPath(Skeleton_new(nodeNew,
                            skeletonGrandparentNew,
                            NULL,
                        grandparentNew)),
                    };
                    return result;
                } else {
                    SNode *grandparentSiblingNew = LLRBTree_new(rand(), parentSiblingOther1, BLACK, parentSiblingOther2, BLACK);
                    SNode *nodeNew = LLRBTree_new(rand(), grandparentNew, RED, grandparentSiblingNew, RED);
                    LLRBTree_replaceSelf(grandparentIsomorph, nodeNew);
                    TreeResult result = {
                        getRoot(nodeNew),
                        Skeleton_wholeDirectPath(Skeleton_new(nodeNew,
                            skeletonGrandparentNew,
                            Skeleton_new(grandparentSiblingNew,
                                NULL,
                                NULL,
                            NULL),
                        grandparentNew)),
                    };
                    return result;
                }
            }
            SNode *parentSibling = parentSiblingsIsomorph[0]; // if fail to borrow then merge into parent's arbirtary sibling (under isomorphism): B1 -> B2* B3* ++ B4 -> B5 B6 => B' -> (R4 -> B5 B6) B2 or 2 <-> 3
            SNode *nodeNew = LLRBTree_new(rand(), parentSibling, RED, nodeToReplace != NULL ? nodeReplace : sibling, BLACK);
            return LLRBTree_removeSelf_234(parent, parentSibling, nodeNew, Skeleton_new(nodeNew,
                NULL,
                nodeToReplace != NULL ? skeletonReplace : NULL,
            nodeReplace)); // @Warn: `parentSibling` becomes child of `nodeNew` while being sibling (under isomorphism) of `parent` at the beginning of recursion, which forms a (temporary) broken parent-child relationship.
        }
    }
/****************************************
 * end same code as 2-3 mode
 ****************************************/
}

static SNode *LLRBTree_init_23(int *ids, int n, int h) {
    if (n == 1) {
        assert(h == 0, "LLRB: impossible height.");
        return LLRBTree_new(ids[0], NULL, BLACK, NULL, BLACK);
    }
    if (n < 3 * ((int) pow(2, h - 1))) {
        int n1 = n - n / 2; // ceil(n / 2.0)
        return LLRBTree_new(rand(),
            LLRBTree_init_23(ids, n1, h - 1), BLACK,
            LLRBTree_init_23(ids + n1, n - n1, h - 1), BLACK);
    }
    int n1 = n / 3 + (int)(n % 3 >= 1), n2 = n / 3 + (int)(n % 3 >= 2);
    return LLRBTree_new(rand(),
        LLRBTree_new(rand(),
            LLRBTree_init_23(ids, n1, h - 1), BLACK,
            LLRBTree_init_23(ids + n1, n2, h - 1), BLACK), RED,
        LLRBTree_init_23(ids + n1 + n2, n - n1 - n2, h - 1), BLACK);
}
static SNode *LLRBTree_init_234(int *ids, int n, int h) {
/****************************************
 * begin same code as 2-3 mode
 ****************************************/
    if (n == 1) {
        assert(h == 0, "LLRB: impossible height.");
        return LLRBTree_new(ids[0], NULL, BLACK, NULL, BLACK);
    }
    int N = (int) pow(2, h - 1);
    if (n < 3 * N) {
        int n1 = n - n / 2; // ceil(n / 2.0)
        return LLRBTree_new(rand(),
            LLRBTree_init_234(ids, n1, h - 1), BLACK,
            LLRBTree_init_234(ids + n1, n - n1, h - 1), BLACK);
    } else if (n < 4 * N) {
        int n1 = n / 3 + (int)(n % 3 >= 1), n2 = n / 3 + (int)(n % 3 >= 2);
        return LLRBTree_new(rand(),
            LLRBTree_new(rand(),
                LLRBTree_init_234(ids, n1, h - 1), BLACK,
                LLRBTree_init_234(ids + n1, n2, h - 1), BLACK), RED,
            LLRBTree_init_234(ids + n1 + n2, n - n1 - n2, h - 1), BLACK);
/****************************************
 * end same code as 2-3 mode
 ****************************************/
    }
    int n1 = n / 4 + (int)(n % 4 >= 1), n2 = n / 4 + (int)(n % 4 >= 2), n3 = n / 4 + (int)(n % 4 >= 3);
    return LLRBTree_new(rand(),
        LLRBTree_new(rand(),
            LLRBTree_init_234(ids, n1, h - 1), BLACK,
            LLRBTree_init_234(ids + n1, n2, h - 1), BLACK), RED,
        LLRBTree_new(rand(),
            LLRBTree_init_234(ids + n1 + n2, n3, h - 1), BLACK,
            LLRBTree_init_234(ids + n1 + n2 + n3, n - n1 - n2 - n3, h - 1), BLACK), RED);
}

// LLRB interface operations

static void LLRBTree_init_users(SNode *root, struct List *users) {
    if (root->num_children == 0) {
        addAfter(users, users->tail, (void *) root);
        return;
    }
    LLRBTree_init_users(root->children[0], users);
    LLRBTree_init_users(root->children[1], users);
}
struct InitRet LLRBTree_init(int *ids, int n, int add_strat, int mode_order, struct List *users) {
    SNode *root;
    int h = ceil(log(n) / log(mode_order));
    switch (mode_order) {
        case LLRBTree_MODE_23: {
            root = LLRBTree_init_23(ids, n, h);
        } break;
        case LLRBTree_MODE_234: {
            root = LLRBTree_init_234(ids, n, h);
        } break;
        default: {
            assert(false, "LLRB: invalid mode.");
        }
    }
    struct InitRet result;
    result.tree = malloc_check(sizeof(struct LLRBTree));
    ((struct LLRBTree *) result.tree)->root = root;
    ((struct LLRBTree *) result.tree)->add_strat = add_strat;
    ((struct LLRBTree *) result.tree)->mode_order = mode_order;
    result.skeleton = Skeleton_wholeTree(root);
    LLRBTree_init_users(root, users);
    return result;
}

static SNode *getRandomOptimalLeaf(SNode *node) {
    if (node->num_children == 0) {
        return node;
    }
    if (node->children[0]->num_leaves < node->children[1]->num_leaves) {
        return getRandomOptimalLeaf(node->children[0]);
    } else if (node->children[0]->num_leaves > node->children[1]->num_leaves) {
        return getRandomOptimalLeaf(node->children[1]);
    } else {
        int r = rand() % getData(node)->num_optimal;
        if (r < getData(node->children[0])->num_optimal) {
            return getRandomOptimalLeaf(node->children[0]);
        } else {
            return getRandomOptimalLeaf(node->children[1]);
        }
    }
}
struct AddRet LLRBTree_add(void *tree, int id) {
    SNode *root = ((struct LLRBTree *) tree)->root;
    int add_strat = ((struct LLRBTree *) tree)->add_strat;
    int mode_order = ((struct LLRBTree *) tree)->mode_order;
    SNode *nodeAdd = LLRBTree_new(id, NULL, BLACK, NULL, BLACK);
    SSkeletonNode *skeletonAdd = Skeleton_new(nodeAdd, NULL, NULL, NULL);
    SNode *nodeAddPos;
    switch (add_strat) {
        case LLRBTree_STRAT_GREEDY: {
            nodeAddPos = getRandomOptimalLeaf(root);
        } break;
        case LLRBTree_STRAT_RANDOM: {
            nodeAddPos = getRandomLeaf(root);
        } break;
        default: {
            assert(false, "LLRB: invalid add strategy.");
        }
    }
    TreeResult resultAdd;
    switch (mode_order) {
        case LLRBTree_MODE_23: {
            resultAdd = LLRBTree_addSibling_23(nodeAddPos, nodeAdd, skeletonAdd, NULL, NULL);
        } break;
        case LLRBTree_MODE_234: {
            resultAdd = LLRBTree_addSibling_234(nodeAddPos, nodeAdd, skeletonAdd, NULL, NULL);
        } break;
        default: {
            assert(false, "LLRB: invalid mode.");
        }
    }
    struct AddRet result;
    result.added = nodeAdd;
    ((struct LLRBTree *) tree)->root = resultAdd.node;
    result.skeleton = resultAdd.skeleton;
    return result;
}

struct RemRet LLRBTree_rem(void *tree, SNode *node) {
    int mode_order = ((struct LLRBTree *) tree)->mode_order;
    int id = ((SNodeData *) node->data)->id;
    TreeResult resultRemove;
    switch (mode_order) {
        case LLRBTree_MODE_23: {
            resultRemove = LLRBTree_removeSelf_23(node, NULL, NULL, NULL);
        } break;
        case LLRBTree_MODE_234: {
            resultRemove = LLRBTree_removeSelf_234(node, NULL, NULL, NULL);
        } break;
        default: {
            assert(false, "LLRB: invalid mode.");
        }
    }
    struct RemRet result;
    result.id = id;
    ((struct LLRBTree *) tree)->root = resultRemove.node;
    if (resultRemove.skeleton != NULL) {
        result.skeleton = resultRemove.skeleton;
    } else {
        result.skeleton = Skeleton_new(resultRemove.node, NULL, NULL, NULL);
    }
    return result;
}
