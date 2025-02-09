
#include "generator.h"
#include "mutator.h"
#include "afl-fuzz.h"
#include "afl-mutations.h"
#include <stdlib.h>
#include <string.h>

extern "C" {

void *afl_custom_init(afl_state_t *afl, unsigned int seed){ return new Mutator; }

void afl_custom_deinit(Mutator *m) { delete m; }

size_t afl_custom_fuzz(Mutator *mutator, uint8_t *buf, size_t buf_size,
  u8 **out_buf, uint8_t *add_buf, // add_buf can be NULL
  size_t add_buf_size, size_t max_size){

    mutator->generator(string((char*)buf));
    
    if(mutator->mutate(max_size)){
      /*  mutate successfully */
        *out_buf = (u8 *)mutator->mutated.c_str();
        return mutator->mutated.size();
    }else{
      /*  drop it */
      return buf_size;
    }
}

}