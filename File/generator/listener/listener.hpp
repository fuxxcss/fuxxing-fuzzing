#pragma once

#include <stack>
#include <vector>
#include "antlr4-runtime.h"

using std::stack;
using std::vector;

class listener : public antlr4::tree::ParseTreeListener {

    public:
        IR *ir;
        stack<IR *> *ir_stack;
        vector<IR *> *ir_library;

        listener(){}
        ~listener(){

            delete ir_stack;
        }

        void collect();
        void pop();
};

void listener::collect(IR *to_collect){

    ir_stack->push(to_collect);
    ir_library->push_back(to_collect);
}

IR *listener::pop(){

    IR *to_pop = ir_stack->top();
    ir_stack->pop();
    return to_pop;
}