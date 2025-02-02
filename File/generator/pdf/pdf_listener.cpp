#include <stack>
#include "antlr4-runtime.h"
#include "generator.h"
#include "pdf_listener.h"

stack<IR *> ir_stack;

void pdf_listener::exitPdf(pdf_parser::PdfContext *ctx){
    
}


