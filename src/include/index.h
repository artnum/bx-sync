#ifndef INDEX_H__
#define INDEX_H__
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

enum RBColor { BLACK, RED };
struct Index {
  _Atomic uintptr_t parent;
  _Atomic uintptr_t child[2];
  _Atomic enum RBColor color;
  uint64_t key[2]; /* we store indexes that can be uuid */
  void *data;
};

struct RBTree {
  _Atomic uintptr_t root;
  pthread_mutex_t write;
};

struct RBTree *rbtree_create();
void index_insert(struct RBTree *tree, struct Index *idx, void **oldata);
void *index_search(struct RBTree *tree, uint64_t *key);
struct Index *index_create(uint64_t key[2], void *data);
struct Index *index_delete(struct RBTree *tree, uint64_t key[2]);
void print_rb_tree(struct Index *root, int level, char branch);
void index_free_node(struct Index *idx);
#endif /* INDEX_H__ */
