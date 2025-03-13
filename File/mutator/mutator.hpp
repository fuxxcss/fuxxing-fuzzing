#pragma once

#include "generator.hpp"
#include <functional>
#include <random>
#include <cassert>
#include <vector>
#include <string>
#include <stack>

#ifdef __AFL__
#include "afl-fuzz.h"
#include "afl-mutations.h"

static const unsigned int HAVOC_ALGOS = 16;

#endif

using std::hash;
using std::stack;
using std::vector;
using std::string;
using std::mt19937;
using std::uniform_int_distribution;

static const unsigned int ROUNDS = 64;
static const unsigned int MUTATE_ALGOS = 3;

static constexpr unsigned int hash_str(string &str){

    return hash<string>{}(str);
}

class Mutator {

    private:
        Generator *gen;
        unsigned int seed;
        vector<IR *> ir_freed;
        IR *locate(unsigned int);
        unsigned int rand_u32(unsigned int);
        /*  two rand_ir funcs   */
        IR *rand_ir();
        IR *rand_ir(IR *);
        IR *havoc_rand_ir();
        /* common mutation */
        void mutation_insert(unsigned int);
        void mutation_delete(unsigned int);
        void mutation_replace(unsigned int);
        /* havoc mutation */
        void mutation_havoc();
        
    public:
        string mutated;

        #ifdef __AFL__
        afl_state_t *afl;
        Mutator(afl_state_t *afl){
            this->afl = afl;
        }
        #endif

        #ifdef __HonggFuzz__
        run_t *hf;
        Mutator(run_t *hf){
            this->hf = hf;
        }
        #endif
        ~Mutator(){
            if(gen) delete gen;
            for(auto i : ir_freed) delete i;
        }

        void generator(string &);
        /*  max size as input   */
        bool mutate(size_t);
};

void Mutator::generator(string &str){

    string trgt = string(getenv(__TARGET__));
    if(!t) gen = nullptr;
    
    switch(hash_str(trgt)){
        #ifdef __PDF__
        case hash_str(__PDF_STR__): gen = new Generator<pdf>(str,__PDF_PATH__); break;
        #endif
        #ifdef __JSON__
        case hash_str(__JSON_STR__): gen = new Generator<json>(str,__JSON_PATH__); break;
        #endif
    }
}

bool Mutator::mutate(size_t max_len){
    
    gen->generate_ir();
    if(!gen->ir) return false;

    seed = hash_str(gen->text);
    
    ....ToDo


    /*  64 rounds common + havoc    */
    unsigned int havoc_steps = rand_u32(ROUNDS);
    unsigned int mutate_steps = ROUNDS - havoc_steps;
    unsigned int mutated_rand = 0;
    
    /*  common mutate  */
    size_t size = gen->ir_library.size();
    for(auto step : mutate_steps){
        mutated_rand = 1 + rand_u32(size);
        switch(step % MUTATE_ALGOS){
        case 0:
            mutation_insert(mutated_rand);
            break;
        case 1:
            mutation_delete(mutated_rand);
            break;
        case 2:
            mutation_replace(mutated_rand);
        }
    }

    /*  havoc mutate  */
    for(auto step : havoc_steps) {
        mutated_rand = 1 + rand_u32(HAVOC_ALGOS);
        /*  havoc dict mode   */
        if(mutated_rand > 14) gen->generate_dict();

        IR *to_mutate = havoc_rand_ir();
        if(!to_mutate) continue;
        u8 *input = nullptr;
        size_t len = 0;
        if(to_mutate->ir_type == IR_DATA_INT || to_mutate->ir_type == IR_DATA_REAL){
            input = (u8 *)to_mutate->ir_data;
            len = 4;
            max_len = len;
        }else{
            input = (u8 *)to_mutate->ir_data.c_str();
            len = to_mutate->ir_data.size();
        }
        afl_mutate(afl,(u8 *)to_mutate->ir_data,input,len,mutated_rand,false,true,NULL,0,max_len);
    }

    /*  mutated */
    mutated = gen->ir->ir_string();

    return true;
}

IR *Mutator::locate(unsigned int rand){

    /*  preorder    */
    stack<IR *> s;
    s.push(ir);
    unsigned int travel = 1;
    while(!s.empty()){
        IR *now = s.top();
        if(travel == rand) return now;
        s.pop();

        if(now->ir_left) s.push(now->ir_left);
        if(now->ir_right) s.push(now->ir_right);

        if(now->ir_type == IR_VECTOR) {
            for(auto i : now->ir_vector) s.push(i);
        }
        ++ travel;
    }

    return nullptr;
}

unsigned int Mutator::rand_u32(unsigned int below){

    mt19937 rand_gen(seed);

    uniform_int_distribution<int> distance(0,below - 1);

    return distance(rand_gen);
}

IR *Mutator::rand_ir(){

    size_t size = gen->ir_library->size();

    unsigned int rand = rand_u32(size);

    return gen->ir_library[rand];
}

IR *Mutator::rand_ir(IR *templ){

    vector<IR *> v;

    for(auto i : gen->ir_library){
        if(i->ir_type == templ->ir_type) v.push_back(i);
        /*  compare with ir_vector  */
        if(i->ir_type == IR_VECTOR)
            for(auto ii : i->ir_vector)
                if(ii->ir_type == templ->ir_type) v.push_back(i);
    }
    
    size_t size = v.size();
    unsigned int rand = rand_u32(size);
    if(v.empty()) return templ;

    return v[rand];
}

IR *Mutator::havoc_rand_ir(){

    vector<IR *> v;

    for(auto i : gen->ir_library){

        if(i->ir_type == IR_DATA_INT ||\
           i->ir_type == IR_DATA_REAL ||\
           i->ir_type == IR_DATA_STR) 
                v.push_back(i);

        /*  compare with ir_vector  */
        if(i->ir_type == IR_VECTOR)
            for(auto ii : i->ir_vector)
                if(i->ir_type == IR_DATA_INT ||\
                   i->ir_type == IR_DATA_REAL ||\
                   i->ir_type == IR_DATA_STR) 
                        v.push_back(i);
    }
    
    size_t size = v.size();
    unsigned int rand = rand_u32(size);
    if(v.empty()) return nullptr;

    return v[rand];
}

void Mutator::mutation_insert(unsigned int rand){

    IR *to_mutate = locate(rand);
    if(!to_mutate) return;

    IR *chosen = rand_ir();

    /*  maybe make synax error  */
    /*  deep copy   */
    if(!to_mutate->left) to_mutate->left = new IR(chosen);
    else if(!to_mutate->right) to_mutate->right = new IR(chosen);
    else if(to_mutate->left && to_mutate->left->ir_type == IR_VECTOR){

        /*  empty vector    */
        if(to_mutate->left->ir_vector->empty()){
            to_mutate->left->ir_vector->push_back(chosen);
            break;
        }

        size_t size = to_mutate->left->ir_vector->size();
        unsigned int index = rand_u32(size);

        /*  keep synax correct  */
        chosen = rand_ir(to_mutate->left->ir_vector->begin());

        to_mutate->left->ir_vector->insert(index,new IR(chosen));
    }
    else if(to_mutate->right && to_mutate->right->ir_type == IR_VECTOR){

        /*  empty vector    */
        if(to_mutate->right->ir_vector->empty()){
            to_mutate->right->ir_vector->push_back(new IR(chosen));
            break;
        }

        size_t size = to_mutate->right->ir_vector->size();
        unsigned int index = rand_u32(size);

        /*  keep synax correct  */
        chosen = rand_ir(to_mutate->right->ir_vector->begin());

        to_mutate->right->ir_vector->insert(index,new IR(chosen));
    }

}


void Mutator::mutation_delete(unsigned int rand){

    IR *to_mutate = locate(rand);
    if(!to_mutate) return;

    if(to_mutate == gen->ir) return;

    if(to_mutate->left){

        ir_freed.push_back(to_mutate->left);
        to_mutate->left = nullptr;
    }
    else if(to_mutate->right){

        ir_freed.push_back(to_mutate->right);
        to_mutate->right = nullptr;
    }
    else if(to_mutate->ir_type == IR_VECTOR){

        if(to_mutate->ir_vector->empty()) return;

        size_t size = to_mutate->ir_vector->size();
        unsigned int index = rand_u32(size);
        ir_freed.push_back(to_mutate->ir_vector[index]);
        to_mutate->ir_vector->erase(index);
    }

}

void Mutator::mutation_replace(unsigned int rand){

    IR *to_mutate = locate(rand);
    if(!to_mutate) return;

    IR *chosen = nullptr;
    if(to_mutate->left){

        chosen = rand_ir(to_mutate->left);
        /*  replace itself  */
        if(chosen == to_mutate) chosen = rand_ir();
        ir_freed.push_back(to_mutate->left);
        to_mutate->left = new IR(chosen);
    }
    else if(to_mutate->right){

        chosen = rand_ir(to_mutate->right);
        /*  replace itself  */
        if(chosen == to_mutate) chosen = rand_ir();
        ir_freed.push_back(to_mutate->right);
        to_mutate->right = new IR(chosen);
    }
    else if(to_mutate->ir_type == IR_VECTOR){

        size_t size = to_mutate->ir_vector->size();
        unsigned int index = rand_u32(size);
        chosen = rand_ir(to_mutate->ir_vector[index]);
        /*  replace itself  */
        if(chosen == to_mutate->ir_vector[index]) chosen = rand_ir();
        ir_freed.push_back(to_mutate->ir_vector[index]);
        to_mutate->ir_vector[index] = new IR(chosen);
    }

}