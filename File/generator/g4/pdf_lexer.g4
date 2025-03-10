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

OBJ             :'obj'(SPACE | NEWLINE)?;
OBJ_END         :'endobj'(SPACE | NEWLINE)?;
OBJ_NULL        :'null'(SPACE | NEWLINE)?;
OBJ_BOOL        :'true'|'false'|'True'|'False'|'TRUE'|'FALSE'(SPACE | NEWLINE)?;

REFERENCE       :'R'(SPACE | NEWLINE)?;

STREAM          :'stream'(SPACE | NEWLINE)?;
STREAM_END      :'endstream'(SPACE | NEWLINE)?;

XREF            :'xref'(SPACE | NEWLINE)?;
XREF_USE        :'n'(SPACE | NEWLINE)?;
XREF_FREE       :'f'(SPACE | NEWLINE)?;

TRAIL           :'trailer'(SPACE | NEWLINE)?;
TRAIL_XREF      :'startxref'(SPACE | NEWLINE)?;
TRAIL_END       :'%%EOF'(SPACE | NEWLINE)?;

SYM_MOD         :'%'(SPACE | NEWLINE)?;
SYM_DOT         :'.'(SPACE | NEWLINE)?;
SYM_LAB         :'<'(SPACE | NEWLINE)?;
SYM_RAB         :'>'(SPACE | NEWLINE)?;
SYM_LDAB        :'<<'(SPACE | NEWLINE)?;
SYM_RDAB        :'>>'(SPACE | NEWLINE)?;
SYM_FS          :'/'(SPACE | NEWLINE)?;
SYM_LP          :'('(SPACE | NEWLINE)?;
SYM_RP          :')'(SPACE | NEWLINE)?;
SYM_LSP         :'['(SPACE | NEWLINE)?;
SYM_RSP         :']'(SPACE | NEWLINE)?;

DATA_INT        :('-' | '+')?[0-9]+(SPACE | NEWLINE)?;
DATA_REAL       :('-' | '+')?[0-9]+SYM_DOT[0-9]+(SPACE | NEWLINE)?
                |('-' | '+')?SYM_DOT[0-9]+(SPACE | NEWLINE)?
                ;
DATA_STR        :.+?(SPACE | NEWLINE)?;

SPACE           :' ';
NEWLINE         :'\r'?'\n';

