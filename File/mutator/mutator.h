#pragma once

#include "generator.h"
#include "afl-fuzz.h"

class Mutator {

    private:
        afl_state_t *afl;
        Generator *gen;
        vector<IR *> ir_freed;

        IR *locate(unsigned int);
        /*  two rand_ir funcs   */
        IR *rand_ir();
        IR *rand_ir(IR *);
        IR *havoc_rand_ir();
        void mutation_insert(unsigned int);
        void mutation_delete(unsigned int);
        void mutation_replace(unsigned int);
        
    public:
        string mutated;

        Mutator(afl_state_t *afl){
            this->afl = afl;
        }
        ~Mutator(){
            if(gen) delete gen;
            for(auto i : ir_freed) delete i;
        }

        void generator(string &);
        bool mutate();
};