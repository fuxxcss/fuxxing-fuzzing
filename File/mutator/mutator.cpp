#include "generator.hpp"
#include "afl-fuzz.h"
#include "afl-mutations.h"
#include "mutator.h"
#include <functional>
#include <cassert>
#include <vector>
#include <string>
#include <stack>

using std::hash;
using std::stack;
using std::vector;
using std::string;

const unsigned int ROUNDS = 32;
const unsigned int HAVOC_STEPS = 20;
const unsigned int HAVOC_ALGOS = 16;
const unsigned int MUTATE_ALGOS = 3;

constexpr unsigned int hash_str(string str){

    return hash<string>(str);
}

void Mutator::generator(string &str){

    string t = string(getenv(__TARGET__));
    if(!t) gen = nullptr;
    
    switch(hash_str(t)){
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

    /*  32 rounds havoc + mutate    */
    unsigned int havoc_steps = 1 + rand_below(afl,HAVOC_STEPS);
    unsigned int mutate_steps = ROUNDS - havoc_steps;
    unsigned int mutated_rand = 0;
    /*  mutate  */
    size_t size = gen->ir_library.size();
    for(auto step : mutate_steps){
        mutated_rand = 1 + rand_below(afl,size);
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

    /*  havoc */
    for(auto step : havoc_steps) {
        mutated_rand = 1 + rand_below(afl,HAVOC_ALGOS);
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

IR *Mutator::rand_ir(){

    size_t size = gen->ir_library->size();

    unsigned int rand = rand_below(afl,size);

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
    unsigned int rand = rand_below(afl,size);
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
    unsigned int rand = rand_below(afl,size);
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
        unsigned int index = rand_below(afl,size);

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
        unsigned int index = rand_below(afl,size);

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
        unsigned int index = rand_below(afl,size);
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
        unsigned int index = rand_below(afl,size);
        chosen = rand_ir(to_mutate->ir_vector[index]);
        /*  replace itself  */
        if(chosen == to_mutate->ir_vector[index]) chosen = rand_ir();
        ir_freed.push_back(to_mutate->ir_vector[index]);
        to_mutate->ir_vector[index] = new IR(chosen);
    }

}