// glthread.h

#pragma once

typedef struct glthread_t glthread_t;

struct glthread_t {
  struct glthread_t *left;
  struct glthread_t *right;
};

#define GLTHREAD_FOREACH_BEGIN(start, ptr) { \
  glthread_t *_ptr = nullptr; \
  ptr = (start)->right; \
  for (; ptr != nullptr; ptr = _ptr) { \
    _ptr = ptr->right;

#define GLTHREAD_FOREACH_END() }}

#define DEFINE_GLTHREAD_TO_STRUCT_FUNC(fn_name, struct_name, field_name) \
  static inline struct_name * fn_name(glthread_t *glthreadptr) { \
    return (struct_name *)((char *)(glthreadptr) - (char *)&(((struct_name *)0)->field_name)); \
  }

static inline void glthread_init(glthread_t *curr) {
  if (!curr) { return; }
  curr->left = nullptr;
  curr->right = nullptr;
}

static inline void glthread_add_next(glthread_t *curr, glthread_t *n) {
  if (!curr->right) {
    curr->right = n;
    n->left = curr;
    return;
  }
  glthread_t *temp = curr->right;
  curr->right = n;
  n->left = curr;
  n->right = temp;
  temp->left = n;
}

static inline void glthread_add_before(glthread_t *curr, glthread_t *n) {
  if (!curr->left) {
    curr->left = n;
    n->right = curr;
    return;
  }
  glthread_t *temp = curr->left;
  curr->left = n;
  n->right = curr;
  n->left = temp;
  temp->right = n;
}


