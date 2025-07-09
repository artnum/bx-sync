#ifndef INDEX_H__
#define INDEX_H__
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

enum RBColor { BLACK, RED };
struct Index {
  struct Index *parent;
  union {
    struct {
      struct Index *left;
      struct Index *right;
    };
    struct Index *child[2];
  };
  enum RBColor color;
  uint64_t key[2]; /* we store indexes that can be uuid */
  void *data;
};

struct RBTree {
  struct Index *root;
};

struct RBTree *rbtree_create();
void index_insert(struct RBTree *tree, struct Index *idx, void **oldata);
void *index_search(struct RBTree *tree, uint64_t *key);
struct Index *index_create(uint64_t key[2], void *data);
struct Index *index_delete(struct RBTree *tree, uint64_t key[2]);
void print_rb_tree(struct Index *root, int level, char branch);
void index_free_node(struct Index *idx);
#endif /* INDEX_H__ */
