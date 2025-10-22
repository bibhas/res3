// glthread.h

#pragma once

typedef struct __glthread {
  struct __glthread *left;
  struct __glthread *right;
} glthread_t;

static inline void init_glthread(glthread_t *curr) {
  if (!curr) { return; }
  curr->left = NULL;
  curr->right = NULL;
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


