/**
 * Malloc
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void *start_of_heap = NULL;
void *last_meta = NULL;
void *free_start = NULL;

typedef struct _metadata_t {
  unsigned int size;     // The size of the memory block.
  unsigned char is_used;  // 0 if the block is free; 1 if the block is used.
  struct _metadata_t *next_free;
  struct _metadata_t *next;
} metadata_t;

void *sbrk_wrapper(size_t size) {
  void *ptr = sbrk(size);
  if (!ptr) return NULL;
  return ptr;
}

void print_heap() {
  metadata_t *curMeta = start_of_heap;
  void *endOfHeap = sbrk(0);
  printf("-- Start of Heap (%p) --\n", start_of_heap);
  while ((void *)curMeta < endOfHeap) {   // While we're before the end of the heap...
    printf("metadata for memory %p: (%p, size=%d, is_used=%d)\n", (void *)curMeta + sizeof(metadata_t), curMeta, curMeta->size, curMeta->is_used);
    curMeta = (void *)curMeta + curMeta->size + sizeof(metadata_t);
  }
  printf("-- End of Heap (%p) --\n\n", endOfHeap);
}

void print_next() {
  metadata_t *curMeta = start_of_heap;
  void *endOfHeap = sbrk(0);
  printf("-- Start of NEXT (%p) --\n", start_of_heap);
  while (curMeta) {
    printf("metadata for memory %p: (%p, size=%d, is_used=%d)\n", (void *)curMeta + sizeof(metadata_t), curMeta, curMeta->size, curMeta->is_used);
    curMeta = curMeta->next;
  }
  printf("-- End of NEXT (%p) --\n\n", endOfHeap);
}

void print_free() {
  metadata_t *curMeta = free_start;
  printf("-- Start of Free List (%p) --\n", free_start);
  metadata_t *endOfHeap = NULL;
  while (curMeta) {
    printf("metadata for memory %p: (%p, size=%d, is_used=%d)\n", (void *)curMeta + sizeof(metadata_t), curMeta, curMeta->size, curMeta->is_used);
    endOfHeap = curMeta;
    curMeta = curMeta->next_free;
  }
  printf("-- End of Free List (%p) --\n\n", endOfHeap);
}

/**
 * Allocate space for array in memory
 *
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size) bytes.
 *
 * @param num
 *    Number of elements to be allocated.
 * @param size
 *    Size of elements.
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */
void *calloc(size_t num, size_t size) {
  int n = num * size;
  void *ptr = malloc(n);
  if (ptr) memset(ptr, 0, n);
  return ptr;
}


/**
 * Allocate memory block
 *
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size
 *    Size of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */

void *malloc(size_t size) {
  if (size == 0) return NULL;
  if (start_of_heap == NULL) {
    start_of_heap = sbrk(sizeof(metadata_t));
    last_meta = start_of_heap;
    metadata_t *start = start_of_heap;
    start->is_used = 1;
    start->size = 0;
    start->next = NULL;
  }
  // No free list yet
  if (!free_start) {
    metadata_t *meta = sbrk(sizeof(metadata_t));
    meta->size = size;
    meta->is_used = 1;
    void *ptr = sbrk(size);
    metadata_t *last = last_meta;
    last->next = meta;
    last_meta = (void *)meta;
    return ptr;
  }
  // Traverse free list
  metadata_t *curr = free_start;
  metadata_t *prev = NULL;
  metadata_t *prev_free = NULL;
  while (curr) {
    // First fit algorithm
    if (curr->size >= size) {
      curr->is_used = 1;
      void *ptr = (void *)curr + sizeof(metadata_t);
      int remaining_size = curr->size - size;
      // Block splitting
      if (remaining_size > sizeof(metadata_t)) {
        metadata_t *new_block = (void *)curr + sizeof(metadata_t) + size;
        new_block->is_used = 0;
        new_block->size = remaining_size - sizeof(metadata_t);
        new_block->next = curr->next;
        new_block->next_free = curr->next_free;
        curr->next = new_block;
        curr->next_free = NULL;
        curr->size = size;
        if (prev) {
          prev->next_free = new_block;
        } else {
          free_start = new_block;
        }
      } else {
        if (prev) prev->next_free = curr->next_free;
        curr->next_free = NULL;
      }
      return ptr;
    }
    prev_free = prev;
    prev = curr;
    curr = curr->next_free;
  }
  // If no free block fits the size, add size to last free block
  if (prev) {
    prev->is_used = 1;
    int prev_size = prev->size;
    void *ptr = sbrk(size - prev_size);
    ptr -= prev->size;
    prev->size = size;
    if (!prev_free) {
      free_start = NULL;
    } else {
      prev_free->next_free = NULL;
    }
    return ptr;
  } else {
    return NULL;
  }
}

/**
 * Deallocate space in memory
 *
 * A block of memory previously allocated using a call to malloc(),
 * calloc() or realloc() is deallocated, making it available again for
 * further allocations.
 *
 * Notice that this function leaves the value of ptr unchanged, hence
 * it still points to the same (now invalid) location, and not to the
 * null pointer.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(),
 *    calloc() or realloc() to be deallocated.  If a null pointer is
 *    passed as argument, no action occurs.
 */
void free(void *ptr) {
  if (!ptr) return;
  metadata_t *meta = ptr - sizeof(metadata_t);
  if (meta->is_used == 0) return;
  meta->is_used = 0;
  if (!free_start) {
    free_start = meta;
  } else {
    // Coalescing
    metadata_t *curr = start_of_heap;
    curr = curr->next;
    metadata_t *prev = NULL;
    metadata_t *prev_free = NULL;
    while (curr && curr != meta) {
      if (!curr->is_used) {
        prev_free = curr;
      }
      if (prev && !prev->is_used && !curr->is_used) {
        prev->size += sizeof(metadata_t) + curr->size;
        prev->next = curr->next;
        prev->next_free = curr->next_free;
      }
      prev = curr;
      curr = curr->next;
    }
    if (prev && curr && !prev->is_used && !curr->is_used) {
      prev->size += sizeof(metadata_t) + curr->size;
      prev->next = curr->next;
      prev->next_free = curr->next_free;
    } else if (curr && !curr->is_used) {
      metadata_t *next = curr->next;
      if (next && !next->is_used) {
        curr->size += sizeof(metadata_t) + next->size;
        curr->next = next->next;
        curr->next_free = next->next_free;
      }
    }
    if (!prev_free) {
      free_start = meta;
    } else {
      prev_free->next_free = meta;
    }
  }
}


/**
 * Reallocate memory block
 *
 * The size of the memory block pointed to by the ptr parameter is changed
 * to the size bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old sizes, even if the block is moved. If
 * the new size is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is
 * deallocated as if a call to free was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(), calloc()
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size
 *    New size for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */
void *realloc(void *ptr, size_t size) {
  if (!ptr) return malloc(size);
  if (size == 0) {
    free(ptr);
    return NULL;
  }
  metadata_t *meta = ptr - sizeof(metadata_t);
  if (meta->size >= size) return ptr;
  void *new_ptr = malloc(size);
  memcpy(new_ptr, ptr, meta->size);
  free(ptr);
  return new_ptr;
}

