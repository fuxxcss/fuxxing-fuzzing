#include "afl/afl-fuzz.h"

typedef struct {
    u8 *buf;
}Mutator;


Mutator *afl_custom_init(afl_state_t *afl, unsigned int seed) {
  Mutator *m= (Mutator*)malloc(sizeof(Mutator));
  return m;
}

void afl_custom_deinit(Mutator *data) {}

u8 afl_custom_queue_new_entry(Mutator *mutator,
                              const unsigned char *filename_new_queue,
                              const unsigned char *filename_orig_queue) {
  printf("new queue entry filename %s\n",filename_new_queue);
  return false;
}

size_t afl_custom_fuzz(Mutator *mutator, uint8_t *buf, size_t buf_size,
                       u8 **out_buf, uint8_t *add_buf,
                       size_t add_buf_size,  // add_buf can be NULL
                       size_t max_size) {
  buf[buf_size-1] ++;
  *out_buf = buf;
  return 0x100000;
}
