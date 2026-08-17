#ifndef INDEX_H__
#define INDEX_H__
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

enum RBColor { BLACK, RED };
struct IntrusiveList {
  void *next;
  void *previous;
};
struct RBNode {
  struct IntrusiveList list;
  _Atomic uintptr_t parent;
  _Atomic uintptr_t child[2];
  _Atomic enum RBColor color;
  uint64_t key[2]; /* we store indexes that can be uuid */
  _Atomic uint64_t data;
};

struct RBTree {
  void *front;
  void *back;
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
  int *item_count;
  int count;
  int length;
  pthread_mutex_t mutex;
} Indexes;

void index_init(Indexes *indexes);
int index_new(Indexes *indexes, const char *name);
bool index_has(Indexes *indexes, int id, uint64_t *key);
bool index_set(Indexes *indexes, int id, uint64_t *key, uint64_t data);
void index_delete(Indexes *indexes, int id, uint64_t *key);
void index_dump(Indexes *indexes, int id);
void index_traverse(Indexes *indexes, int id,
                    void (*cb)(void *userdata, struct RBNode *node),
                    void *userdata);
void index_destroy(Indexes *indexes);
uint64_t index_get(Indexes *indexes, int id, uint64_t *key);

struct RBTree *rbtree_create();
void rbtree_insert(struct RBTree *tree, struct RBNode *idx, uint64_t *oldata);
uintptr_t rbtree_search(struct RBTree *tree, uint64_t *key);
struct RBNode *rbtree_create_node(uint64_t key[2], uint64_t data);
struct RBNode *rbtree_delete(struct RBTree *tree, uint64_t key[2]);
void rbtree_print(struct RBNode *root, int level, char branch);
void rbtree_free_node(struct RBNode *idx);
#endif /* INDEX_H__ */
