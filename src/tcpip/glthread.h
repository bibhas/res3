// glthread.h

#pragma once

#include "utils.h"

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
  EXPECT_RETURN(curr != nullptr, "Empty curr ptr param");
  EXPECT_RETURN(n != nullptr, "Empty next ptr param");
  if (curr->right == nullptr) {
    curr->right = n;
    n->left = curr;
  }
  else {
    glthread_t *temp = curr->right;
    curr->right = n;
    n->left = curr;
    n->right = temp;
    temp->left = n;
  }
}

static inline void glthread_add_before(glthread_t *curr, glthread_t *b) {
  EXPECT_RETURN(curr != nullptr, "Empty curr ptr param");
  EXPECT_RETURN(b != nullptr, "Empty before ptr param");
  if (!curr->left) {
    curr->left = b;
    b->right = curr;
    return;
  }
  glthread_t *temp = curr->left;
  curr->left = b;
  b->right = curr;
  b->left = temp;
  temp->right = b;
}

static inline bool glthread_remove(glthread_t *curr) {
  EXPECT_RETURN_BOOL(curr != nullptr, "Empty curr ptr param", false);
  if (curr->left == nullptr && curr->right != nullptr) {
    // nullptr <- (curr) -> (...)
    curr->right->left = nullptr;
  }
  else if (curr->left != nullptr && curr->right == nullptr) {
    // (...) <- (curr) -> nullptr
    curr->left->right = nullptr;
  }
  else if (curr->left == nullptr && curr->right == nullptr) {
    // noop
  }
  else {
    // (...) <- (curr) -> (...)
    curr->left->right = curr->right;
    curr->right->left  = curr->left;
  }
  curr->left = nullptr;
  curr->right = nullptr;
  return true;
}


