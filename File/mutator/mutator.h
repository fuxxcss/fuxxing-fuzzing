#pragma once

#include "generator.h"
#include "afl-fuzz.h"

class Mutator {

    private:
        afl_state_t *afl;
        Generator *gen;
        unsigned int mutate_rand;

        IR *locate();
        /*  two rand_ir funcs   */
        IR *rand_ir();
        IR *rand_ir(IR *);
        void mutation_insert();
        void mutation_delete();
        void mutation_replace();
        
    public:
        string mutated;

        Mutator(afl_state_t *afl){
            this->afl = afl;
        }
        ~Mutator(){
            if(gen) delete gen;
        }

        void generator(string &);
        bool mutate();
};