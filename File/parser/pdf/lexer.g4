/*
 *  PDF Lexer for Parser
 */

lexer grammar pdf_lexer;

options {
    superClass = PDFLexerBase;
    caseInsensitive = true;
}

@lexer::postinclude {
#include <PDFLexerBase.h>
}

OBJ             :'obj';
OBJ_END         :'endobj';
OBJ_NULL        :'null';
OBJ_BOOL        :'true'|'false'|'True'|'False'|'TRUE'|'FALSE';

REFERENCE       :'R';

STREAM          :'stream';
STREAM_END      :'endstream';

XREF            :'xref';
XREF_USE        :'n';
XREF_FREE       :'f';

TRAIL           :'trailer';
TRAIL_XREF      :'startxref';
TRAIL_END       :'%%EOF';

SYM_MOD         :'%';
SYM_DOT         :'.';
SYM_LAB         :'<';
SYM_RAB         :'>';
SYM_LDAB        :'<<';
SYM_RDAB        :'>>';
SYM_FS          :'/';
SYM_LP          :'(';
SYM_RP          :')';
SYM_LSP         :'[';
SYM_RSP         :']';

DATA_INT        :('-' | '+')?[0-9]+;
DATA_REAL       :('-' | '+')?[0-9]+SYM_DOT[0-9]+
                |('-' | '+')?SYM_DOT[0-9]+
                ;
DATA_STR        :.+;

SPACE           :' ' -> skip;
NEWLINE         :'\r'?'\n' -> skip;

