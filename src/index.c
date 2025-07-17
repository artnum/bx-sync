#include "include/index.h"
#include "include/bx_ids_cache.h"
#include <assert.h>
#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <threads.h>

struct RBTraversalQueueNode {
  struct RBNode *node;
  struct RBTraversalQueueNode *next;
};

struct RBTraversalQueue {
  struct RBTraversalQueueNode *front;
  struct RBTraversalQueueNode *back;
};

/* red-black tree based on postgresql implementation
 * https://doxygen.postgresql.org/rbtree_8c_source.html
 */

inline static int _rbt_cmp_key(uint64_t *a, uint64_t *b) {
  if (a[0] < b[0])
    return -1;
  if (a[0] > b[0])
    return 1;
  if (a[1] < b[1])
    return -1;
  if (a[1] > b[1])
    return 1;
  return 0;
}

#define IS_NIL(n) (((void *)n) == ((void *)&__sentinel) || ((void *)n) == NULL)
#define SENTINEL ((uintptr_t)&__sentinel)
#define IS_BLACK(n) (atomic_load(&(n)->color) == BLACK)
#define IS_RED(n) (!IS_BLACK(n))
#define SET_BLACK(n) atomic_store(&(n)->color, BLACK)
#define SET_RED(n) atomic_store(&(n)->color, RED)
#define LEFT_CHILD(n) atomic_load(&(n)->child[LEFT])
#define RIGHT_CHILD(n) atomic_load(&(n)->child[RIGHT])
#define SET_LEFT_CHILD(n, v) atomic_store(&(n)->child[LEFT], (uintptr_t)v)
#define SET_RIGHT_CHILD(n, v) atomic_store(&(n)->child[RIGHT], (uintptr_t)v)
#define CAST(n) ((struct RBNode *)(n))
#define UNCAST(n) ((uintptr_t)(n))
enum RBDirection { LEFT, RIGHT };

static struct RBNode __sentinel = {.color = BLACK,
                                   .child[LEFT] = SENTINEL,
                                   .child[RIGHT] = SENTINEL,
                                   .parent = 0};

void rbtree_free_node(struct RBNode *idx) { free(idx); }

void rbtree_print(struct RBNode *root, int level, char branch) {
  if (IS_NIL(root)) {
    return;
  }

  for (int i = 0; i < level; i++) {
    printf("  ");
  }
  printf("%05lu -- %c (%s)\n", root->key[0], branch,
         IS_RED(root) ? "RED" : "BLACK");
  rbtree_print(CAST(LEFT_CHILD(root)), level + 1, 'L');
  rbtree_print(CAST(RIGHT_CHILD(root)), level + 1, 'R');
}

#define IS_QUEUE_EMPTY(q) ((q)->front == NULL)

void _rbtt_enq(struct RBTraversalQueue *q, struct RBNode *node) {
  struct RBTraversalQueueNode *n = calloc(1, sizeof(*n));
  if (n) {
    n->node = node;
    n->next = NULL;
    if (IS_QUEUE_EMPTY(q)) {
      q->front = n;
      q->back = n;
    } else {
      q->back->next = n;
      q->back = n;
    }
  }
}

struct RBNode *_rbtt_deq(struct RBTraversalQueue *q) {
  struct RBTraversalQueueNode *n = NULL;
  if (IS_QUEUE_EMPTY(q)) {
    return NULL;
  }
  n = q->front;
  if (q->front == q->back) {
    q->back = n->next;
  }
  q->front = n->next;
  struct RBNode *node = n->node;
  free(n);
  return node;
}

void rbtree_traverse(struct RBTree *tree,
                     void (*callback)(void *userdata, struct RBNode *node),
                     void *userdata) {
  struct RBTraversalQueue queue = {.back = NULL, .front = NULL};

  pthread_mutex_lock(&tree->write);
  struct RBNode *node = (struct RBNode *)atomic_load(&tree->root);
  _rbtt_enq(&queue, node);
  while ((node = _rbtt_deq(&queue)) != NULL) {
    if (node == NULL) {
      break;
    }
    callback(userdata, node);
    struct RBNode *child[2] = {
        (struct RBNode *)atomic_load(&node->child[0]),
        (struct RBNode *)atomic_load(&node->child[1]),
    };

    if (!IS_NIL(child[0])) {
      _rbtt_enq(&queue, child[0]);
    }
    if (!IS_NIL(child[1])) {
      _rbtt_enq(&queue, child[1]);
    }
  }
  pthread_mutex_unlock(&tree->write);
}

struct RBNode *_rbt_index_search(struct RBTree *tree, uint64_t *key,
                                 struct RBNode **parent) {
  assert(tree != NULL);
  assert(key != NULL);

  if (parent) {
    *parent = NULL;
  }
  if (CAST(tree->root) == NULL) {
    return CAST(SENTINEL);
  }

  struct RBNode *node = CAST(tree->root);
  for (;;) {
    if (IS_NIL(node)) {
      return CAST(SENTINEL);
    }
    int cmp = _rbt_cmp_key(key, node->key);
    if (cmp == 0) {
      return node;
    } else if (cmp < 0) {
      if (parent) {
        *parent = node;
      }
      node = CAST(LEFT_CHILD(node));
    } else {
      if (parent) {
        *parent = node;
      }
      node = CAST(RIGHT_CHILD(node));
    }
  }
}

inline static void _rbt_rotate(struct RBTree *tree, struct RBNode *x,
                               enum RBDirection direction) {
  struct RBNode *y = CAST(x->child[!direction]);

  /* disconnect the subtree to be rotated from the tree */
  struct RBNode *x_parent = CAST(x->parent);
  enum RBDirection x_direction;
  bool root_parent = false;
  if (!x->parent) {
    x_parent = NULL;
    root_parent = true;
    atomic_store(&tree->root, 0);
  } else {
    x_direction = x == CAST(RIGHT_CHILD(CAST(x->parent)));
    atomic_store(&x_parent->child[x_direction], (uintptr_t)SENTINEL);
  }

  /* rotate */
  x->child[!direction] = y->child[direction];
  if (!IS_NIL(y->child[direction])) {
    CAST(y->child[direction])->parent = UNCAST(x);
  }

  if (!IS_NIL(y)) {
    y->parent = UNCAST(x_parent);
  }

  y->child[direction] = UNCAST(x);
  if (!IS_NIL(x)) {
    x->parent = UNCAST(y);
  }

  /* reconnect rotated subtree */
  if (root_parent) {
    atomic_store(&tree->root, (uintptr_t)y);
  } else {
    atomic_store(&x_parent->child[x_direction], (uintptr_t)y);
  }
}

inline static void _rbt_insert_fixup(struct RBTree *tree, struct RBNode *x) {
  while (x != CAST(tree->root) && IS_RED(CAST(x->parent))) {
    enum RBDirection direction =
        x->parent == RIGHT_CHILD(CAST(CAST(x->parent)->parent));
    struct RBNode *y = CAST(CAST(CAST(x->parent)->parent)->child[!direction]);
    if (IS_RED(y)) {
      SET_BLACK(CAST(x->parent));
      SET_BLACK(y);
      SET_RED(CAST(CAST(x->parent)->parent));

      x = CAST(CAST(x->parent)->parent);
    } else {
      if (x == CAST(CAST(x->parent)->child[!direction])) {
        x = CAST(x->parent);
        _rbt_rotate(tree, x, direction);
      }
      SET_BLACK(CAST(x->parent));
      SET_RED(CAST(CAST(x->parent)->parent));
      _rbt_rotate(tree, CAST(CAST(x->parent)->parent), !direction);
    }
  }
  SET_BLACK(CAST(tree->root));
}
void rbtree_insert(struct RBTree *tree, struct RBNode *node,
                   uintptr_t *olddata) {
  struct RBNode *parent = NULL;
  struct RBNode *found = _rbt_index_search(tree, node->key, &parent);

  pthread_mutex_lock(&tree->write);
  if (IS_NIL(found) && parent == NULL) {
    tree->root = UNCAST(node);
    SET_BLACK(CAST(tree->root));
    pthread_mutex_unlock(&tree->write);
    return;
  }

  if (IS_NIL(found)) {
    SET_RED(node);
    node->parent = UNCAST(parent);
    if (_rbt_cmp_key(node->key, parent->key) < 0) {
      SET_LEFT_CHILD(parent, node);
    } else {
      SET_RIGHT_CHILD(parent, node);
    }
    _rbt_insert_fixup(tree, node);
  } else {
    if (olddata != NULL) {
      *olddata = atomic_exchange(&found->data, node->data);
    } else {
      atomic_store(&found->data, node->data);
    }
  }
  /* move or set at the front of the list */
  if (node->list.next) {
    ((struct IntrusiveList *)node->list.next)->previous = node->list.previous;
  }
  if (node->list.previous) {
    ((struct IntrusiveList *)node->list.previous)->next = node->list.next;
  }
  node->list.next = tree->front;
  if (tree->front) {
    ((struct IntrusiveList *)tree->front)->previous = node;
  }
  if (tree->back == NULL) {
    tree->back = node;
  }
  tree->front = node;
  node->list.previous = NULL;

  pthread_mutex_unlock(&tree->write);
}

struct RBTree *rbtree_create() {
  struct RBTree *tree = calloc(1, sizeof(struct RBTree));
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);

  pthread_mutex_init(&tree->write, &attr);
  pthread_mutexattr_destroy(&attr);
  return tree;
}

/* TODO */
static inline void _rbt_destroy_node(struct RBNode *node) {}

/* TODO */
void rbtree_destroy(struct RBTree *tree) {
  struct RBNode *node = NULL;
  node = CAST(tree->root);
  if (!node) {
    free(tree);
    return;
  }

  while (!IS_NIL(LEFT_CHILD(node))) {
    node = CAST(LEFT_CHILD(node));
  }
  while (node->parent != 0) {
    struct RBNode *tmp = CAST(node->parent);
    rbtree_free_node(node);
    node = tmp;
    SET_LEFT_CHILD(tmp, NULL);
  }
}

struct RBNode *rbtree_create_node(uint64_t key[2], uintptr_t data) {
  struct RBNode *new = calloc(1, sizeof(*new));
  if (new) {
    new->list.next = NULL;
    new->list.previous = NULL;
    new->key[0] = key[0];
    new->key[1] = key[1];
    new->data = data;

    SET_LEFT_CHILD(new, SENTINEL);
    SET_RIGHT_CHILD(new, SENTINEL);
    atomic_store(&new->parent, (uintptr_t)NULL);
    SET_RED(new);
  }

  return new;
}

uintptr_t rbtree_search(struct RBTree *tree, uint64_t *key) {
  struct RBNode *node = _rbt_index_search(tree, key, NULL);
  if (IS_NIL(node)) {
    return (uintptr_t)NULL;
  }

  /* move to the front as it's being accessed so it is fresh */
  pthread_mutex_lock(&tree->write);

  if (node->list.previous) {
    ((struct IntrusiveList *)node->list.previous)->next = node->list.next;
  }
  if (node->list.next) {
    ((struct IntrusiveList *)node->list.next)->previous = node->list.previous;
  }
  node->list.next = tree->front;
  node->list.previous = NULL;
  if (tree->back == NULL) {
    tree->back = node;
  }
  tree->front = node;

  pthread_mutex_unlock(&tree->write);
  return node->data;
}

inline static void _rbt_overwrite_node(struct RBNode *a, struct RBNode *b) {
  memcpy(a->key, b->key, sizeof(a->key));
  a->data = b->data;
}

inline static void _rbt_delete_fixup(struct RBTree *tree, struct RBNode *x) {
  while (x != CAST(tree->root) && IS_BLACK(x)) {
    enum RBDirection direction = x == CAST(RIGHT_CHILD(CAST(x->parent)));
    struct RBNode *w = CAST(CAST(x->parent)->child[!direction]);
    if (IS_RED(w)) {
      SET_BLACK(w);
      SET_RED(CAST(x->parent));
      _rbt_rotate(tree, CAST(x->parent), direction);
      w = CAST(CAST(x->parent)->child[!direction]);
    }
    if (IS_BLACK(CAST(w->child[direction])) &&
        IS_BLACK(CAST(w->child[!direction]))) {
      SET_RED(w);
      x = CAST(x->parent);
    } else {
      if (IS_BLACK(CAST(w->child[!direction]))) {
        SET_BLACK(CAST(w->child[!direction]));
        SET_RED(w);
        _rbt_rotate(tree, w, !direction);
      }
      w->color = CAST(x->parent)->color;
      SET_BLACK(CAST(x->parent));
      SET_BLACK(CAST(LEFT_CHILD(w)));
      _rbt_rotate(tree, CAST(x->parent), direction);
      x = CAST(tree->root);
    }
  }
  SET_BLACK(x);
}

struct RBNode *_rbtree_delete(struct RBTree *tree, struct RBNode *z,
                              bool locked) {
  if (!locked) {
    pthread_mutex_lock(&tree->write);
  }
  struct RBNode *y, *x;

  /* any children */
  if (IS_NIL(LEFT_CHILD(z)) || IS_NIL(RIGHT_CHILD(z))) {
    y = z;
  } else {
    /* two children, get successor */
    y = CAST(RIGHT_CHILD(z));
    while (!IS_NIL(LEFT_CHILD(y))) {
      y = CAST(LEFT_CHILD(y));
    }
    /* overwrite target node with successor */
    _rbt_overwrite_node(z, y);
  }

  /* from here, we deal only with single or no child node */
  if (!IS_NIL(LEFT_CHILD(y))) {
    x = CAST(LEFT_CHILD(y));
  } else {
    x = CAST(RIGHT_CHILD(y));
  }

  /* disconnect node */
  atomic_store(&x->parent, y->parent);
  if (atomic_load(&y->parent)) {
    if (y == CAST(LEFT_CHILD(CAST(y->parent)))) {
      SET_LEFT_CHILD(CAST(y->parent), x);
    } else {
      SET_RIGHT_CHILD(CAST(y->parent), x);
    }
  } else {
    atomic_store(&tree->root, (uintptr_t)x);
  }

  if (IS_BLACK(y)) {
    _rbt_delete_fixup(tree, x);
  }
  /* remove from the list */
  if (tree->front == y) {
    tree->front = y->list.next;
  }
  if (tree->back == y) {
    tree->back = y->list.previous;
  }
  if (y->list.next) {
    ((struct IntrusiveList *)y->list.next)->previous = y->list.previous;
  }
  if (y->list.previous) {
    ((struct IntrusiveList *)y->list.previous)->next = y->list.next;
  }

  if (!locked) {
    pthread_mutex_unlock(&tree->write);
  }
  y->parent = 0;
  memset(y->child, 0, sizeof(y->child));
  return y;
}

struct RBNode *rbtree_delete(struct RBTree *tree, uint64_t *key) {
  pthread_mutex_lock(&tree->write);
  struct RBNode *z = _rbt_index_search(tree, key, NULL);
  if (!z || IS_NIL(z)) {
    return NULL;
  }
  z = _rbtree_delete(tree, z, true);
  pthread_mutex_unlock(&tree->write);
  return z;
}

void index_init(Indexes *indexes) {
  assert(indexes != NULL);

  pthread_mutex_init(&indexes->mutex, NULL);
  indexes->count = 0;
  indexes->length = 0;
  indexes->idxs = NULL;
  indexes->item_count = NULL;
}

#define IDXS_CHUNK 20
bool _grow_indexes(Indexes *i) {
  void *tmp = realloc(i->idxs, sizeof(*i->idxs) * (i->length + IDXS_CHUNK));
  void *tmp2 =
      realloc(i->item_count, sizeof(*i->item_count) * (i->length + IDXS_CHUNK));
  if (tmp == NULL || tmp2 == NULL) {
    /* at least maintain what we have */
    if (tmp) {
      i->idxs = (Index *)tmp;
    }
    if (tmp2) {
      i->item_count = (int *)tmp2;
    }
    return false;
  }
  memset(tmp + (i->length * sizeof(*i->idxs)), 0,
         sizeof(*i->idxs) * IDXS_CHUNK);
  memset(tmp2 + (i->length * sizeof(*i->item_count)), 0,
         sizeof(*i->item_count) * IDXS_CHUNK);
  i->idxs = (Index *)tmp;
  i->item_count = (int *)tmp2;
  i->length += IDXS_CHUNK;
  return true;
}

#define INDEX_MAX_SIZE 1000
int index_new(Indexes *indexes, const char *name) {
  pthread_mutex_lock(&indexes->mutex);
  if (indexes->count + 1 >= indexes->length) {
    if (!_grow_indexes(indexes)) {
      pthread_mutex_unlock(&indexes->mutex);
      return -1;
    }
  }
  Index *i = &indexes->idxs[indexes->count];
  i->id = indexes->count;
  i->name = strdup(name);
  if (i->name == NULL) {
    pthread_mutex_unlock(&indexes->mutex);
    return -1;
  }
  i->tree = rbtree_create();
  if (i->tree == NULL) {
    free(i->name);
    pthread_mutex_unlock(&indexes->mutex);
    return -1;
  }
  indexes->count++;
  pthread_mutex_unlock(&indexes->mutex);
  return i->id;
}

bool index_has(Indexes *indexes, int id, uint64_t *key) {
  pthread_mutex_lock(&indexes->mutex);
  if (id >= indexes->count) {
    pthread_mutex_unlock(&indexes->mutex);
    return false;
  }
  struct RBTree *t = indexes->idxs[id].tree;
  pthread_mutex_unlock(&indexes->mutex);
  if (IS_NIL(_rbt_index_search(t, key, NULL))) {
    return false;
  }
  return true;
}

bool index_set(Indexes *indexes, int id, uint64_t *key) {
  uintptr_t data = (uintptr_t)NULL;
  pthread_mutex_lock(&indexes->mutex);
  if (id >= indexes->count) {
    pthread_mutex_unlock(&indexes->mutex);
    return false;
  }
  struct RBTree *t = indexes->idxs[id].tree;
  int count = indexes->item_count[id];
  pthread_mutex_unlock(&indexes->mutex);
  struct RBNode *new = rbtree_create_node(key, data);
  bool inc_count = true;
  if (count + 1 > INDEX_MAX_SIZE) {
    pthread_mutex_lock(&t->write);
    struct IntrusiveList *root = t->back;
    _rbtree_delete(t, (struct RBNode *)t->root, true);
    inc_count = false;
    pthread_mutex_unlock(&t->write);
  }
  data = 0;
  rbtree_insert(t, new, &data);
  if (data != 0) {
    rbtree_free_node(new);
  }
  pthread_mutex_lock(&indexes->mutex);
  if (inc_count) {
    indexes->item_count[id]++;
  }
  pthread_mutex_unlock(&indexes->mutex);

  return true;
}

void index_delete(Indexes *indexes, int id, uint64_t *key) {
  pthread_mutex_lock(&indexes->mutex);
  if (id >= indexes->count) {
    return;
  }
  struct RBTree *t = indexes->idxs[id].tree;
  pthread_mutex_unlock(&indexes->mutex);
  struct RBNode *n = rbtree_delete(t, key);
  if (!IS_NIL(n)) {
    pthread_mutex_lock(&indexes->mutex);
    indexes->item_count[id]--;
    pthread_mutex_unlock(&indexes->mutex);
    rbtree_free_node(n);
  }
}

void index_dump(Indexes *indexes, int id) {
  pthread_mutex_lock(&indexes->mutex);
  if (id >= indexes->count) {
    pthread_mutex_unlock(&indexes->mutex);
    return;
  }
  struct RBTree *t = indexes->idxs[id].tree;
  pthread_mutex_unlock(&indexes->mutex);
  rbtree_print((struct RBNode *)t->root, 0, '#');
  return;
}

void _prune_callaback(void *userdata, struct RBNode *node) {
  struct RBTraversalQueue *queue = (struct RBTraversalQueue *)userdata;
  if (node->data != (uintptr_t)NULL) {
    CacheItem *item = (CacheItem *)node->data;
  }
}

void index_prune(Indexes *indexes, int id) {
  pthread_mutex_lock(&indexes->mutex);
  if (id >= indexes->count) {
    return;
  }
  struct RBTree *tree = indexes->idxs[id].tree;
  pthread_mutex_unlock(&indexes->mutex);

  struct RBTraversalQueue queue = {.front = NULL, .back = NULL};
  index_traverse(indexes, id, _prune_callaback, &queue);
  struct RBNode *current = NULL;
  while ((current = _rbtt_deq(&queue)) != NULL) {
    current = _rbtree_delete(tree, current, false);
    rbtree_free_node(current);
  }
}

void index_traverse(Indexes *indexes, int id,
                    void (*cb)(void *userdata, struct RBNode *node),
                    void *userdata) {
  pthread_mutex_lock(&indexes->mutex);
  if (id >= indexes->count) {
    pthread_mutex_unlock(&indexes->mutex);
    return;
  }
  struct RBTree *t = indexes->idxs[id].tree;
  pthread_mutex_unlock(&indexes->mutex);
  rbtree_traverse(t, cb, userdata);
}
