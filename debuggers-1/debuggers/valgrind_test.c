/* valgrind_test.c
 *
 * This is a test program for use with Valgrind. It demonstrates a few of the
 * error cases Valgrind catches.
 *
 *   Author: Jamie Robinson
 *
 *   Compile with:  make valgrind-test-build
 *       Run with:  valgrind ./valgrind-test
 */


#include <stdlib.h>

void* still_reachable;  // Global variable to hold a heap-allocated block that is not freed.
void* possibly_lost;    // Global variable to hold a heap-allocated block with unusual behavior.

int main() {
  int uninitialized_variable = 0;  // A variable that remains uninitialized.

  for (; uninitialized_variable < 100; uninitialized_variable++) {
    void** definitely_lost = (void**) malloc(sizeof(void*));  // Allocate a pointer on the heap.
    free(definitely_lost);  // Free the allocated pointer.
    definitely_lost = (void*) malloc(7);  // Allocate another block on the heap.
    free(definitely_lost);  // Free the second allocated block. The first block is now indirectly lost.
  }

  // At this point, definitely_lost is out of scope, and we can no longer free it.
  // The pointer pointed to by definitely_lost is indirectly lost.

  still_reachable = malloc(42);  // Allocate a block on the heap that remains reachable globally.
  free(still_reachable);  // Free the allocated block.

  possibly_lost = malloc(10);  // Allocate a block on the heap.
  possibly_lost += 4;  // Move the pointer to the middle of the allocated block.
                      // This is unusual behavior and could lead to a memory leak.
  
  free(possibly_lost - 4);  // Free the block, accounting for the pointer adjustment.

  return 0;
}
/*

Explanation:
- The code includes the necessary header for memory allocation and deallocation (`stdlib.h`).
- The global variables `still_reachable` and `possibly_lost` are used to hold pointers to heap-allocated blocks.
- The `main` function starts by declaring an uninitialized integer variable.
- Inside the loop, a pointer `definitely_lost` is allocated, freed, and then allocated again. The first allocated block becomes indirectly lost because it's not freed after the second allocation.
- After the loop, `still_reachable` is allocated a heap block and freed later.
- `possibly_lost` is allocated and its pointer is adjusted to the middle of the allocated block. This behavior is unusual and could lead to a memory leak.
- The code tries to free the `possibly_lost` block while accounting for the pointer adjustment.
- The `main` function returns 0 to indicate successful completion.
*/
