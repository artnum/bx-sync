#ifndef INDEX_H__
#define INDEX_H__
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

enum RBColor { BLACK, RED };
struct RBNode {
  _Atomic uintptr_t parent;
  _Atomic uintptr_t child[2];
  _Atomic enum RBColor color;
  uint64_t key[2]; /* we store indexes that can be uuid */
  uintptr_t data;
};

struct RBTree {
  _Atomic uintptr_t root;
  pthread_mutex_t write;
};

typedef struct {
  char *name;
  int id;
  struct RBTree *tree;
} Index;

typedef struct {
  Index *idxs;
  int count;
  int length;
  pthread_mutex_t mutex;
} Indexes;

void index_init(Indexes *indexes);
int index_new(Indexes *indexes, const char *name);
bool index_has(Indexes *indexes, int id, uint64_t *key);
bool index_set(Indexes *indexes, int id, uint64_t *key);
void index_delete(Indexes *indexes, int id, uint64_t *key);
void index_dump(Indexes *indexes, int id);

struct RBTree *rbtree_create();
void rbtree_insert(struct RBTree *tree, struct RBNode *idx, uintptr_t *oldata);
uintptr_t rbtree_search(struct RBTree *tree, uint64_t *key);
struct RBNode *rbtree_create_node(uint64_t key[2], uintptr_t data);
struct RBNode *rbtree_delete(struct RBTree *tree, uint64_t key[2]);
void rbtree_print(struct RBNode *root, int level, char branch);
void rbtree_free_node(struct RBNode *idx);
#endif /* INDEX_H__ */
