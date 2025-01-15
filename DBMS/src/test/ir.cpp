/*  override parserbaselistener */
#include "antlr4-runtime.h"
#include "oracle_parserBaseListener.h"
#include "oracle_parserListener.h"
#include "oracle_parser.h"
#include "oracle_lexer.h"
#include "ir.h"
using namespace std;

ir_vector *irv=NULL;

void add_irv(IR *ir,ir_vector *irv){
  if(irv->num == MAX_NUM+1) return;
  irv->vector[irv->num++]=ir;
}
void preorder(IR *ir){
    if(ir){
        printf("ir type:%d\n",ir->type_);
        if(ir->left_) preorder(ir->left_);
        if(ir->right_) preorder(ir->right_);
    }
}

stack<IR *> ir_stack;

#define PUSH(tmp) {ir_stack.push(tmp); /*printf("push %s\n",#tmp);*/}
#define POP(tmp) if(ir_stack.empty()) return; /*printf("pop %s\n",#tmp); */tmp=ir_stack.top(); ir_stack.pop();
#define COLLECT(ir)  add_irv(ir,irv);

ir_vector::ir_vector(){
  num=0;
}

ir_vector return_ir_vector(const std::string &sql){
  ir_vector *new_irv=new ir_vector;
  delete irv;
  irv=new_irv;
  antlr4::ANTLRInputStream input(sql);
  oracle_lexer lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);
  oracle_parser parser(&tokens);
  parser.removeErrorListeners();
  oracle_parserBaseListener listener;
  antlr4::tree::ParseTreeWalker walker;
  walker.walk(&listener,parser.program());
  if(parser.getNumberOfSyntaxErrors() > 0){
    ir_vector *err=new ir_vector;
    irv=err;
    irv->num=-1;
  }
  return *irv;
}

string gen_id_name() { return "v" + to_string(g_id_counter++); }

typedef oracle_parserBaseListener Listener;
typedef oracle_parser Parser;

void Listener::exitProgram(Parser::ProgramContext *ctx){
    IR *_stmtlist;
    POP(_stmtlist)
    IR *ir= new IR(Program, OP3("", "", ""),_stmtlist,NULL);
    COLLECT(ir)
}

void Listener::exitStmtlist(Parser::StmtlistContext * ctx){
    IR *stmtlist,*_stmtlist,*_stmt;
    string middle=";";
    if(ctx->stmtlist()){
      POP(_stmtlist)
      POP(_stmt)
      stmtlist = new IR(Stmtl, OP3("", middle, ""), _stmt, _stmtlist);
      COLLECT(stmtlist)
    }
    else{
      POP(_stmt)
      stmtlist = new IR(Stmtl, OP3("", middle, ""),_stmt, NULL);
      COLLECT(stmtlist)
    }
    PUSH(stmtlist)
  }

void Listener::exitStmt(Parser::StmtContext * ctx){ 
    IR *_stmt_one;
    POP(_stmt_one)
    IR *stmt= new IR(Stmt, OP3("", "", ""),_stmt_one,NULL);
    COLLECT(stmt)
    PUSH(stmt)
}

void Listener::exitCreate_stmt(Parser::Create_stmtContext * ctx){
    IR *_create_one;
    POP(_create_one)
    IR *create_stmt= new IR(Createstmt, OP3("", "", ""),_create_one,NULL);
    COLLECT(create_stmt)
    PUSH(create_stmt)
}

void Listener::exitCreate_database(Parser::Create_databaseContext *ctx){
    string prefix="CREATE DATABASE";
    IR *create_db,*_pwd_v,*_id,*_char_set_name,*_dt;
    if(ctx->USER()){
      POP(_pwd_v)
      POP(_id)
      string middle;
      if(ctx->SYS()) middle="USER SYS IDENTIFIED BY";
      else middle="USER SYSTEM IDENTIFIED BY";
      create_db=new IR(Createdb, OP3(prefix, middle, ""),_id,_pwd_v);
    }
    else if(ctx->CONTROLFILE()){
      POP(_id)
      string middle="CONTROLFILE REUSE";
      create_db=new IR(Createdb, OP3(prefix,middle,""),_id,NULL);
    }
    else if(ctx->datatype()){
      POP(_dt)
      POP(_id)
      string middle;
      if(ctx->MAXDATAFILES()) middle="MAXDATAFILES";
      else middle="MAXINSTANCES";
      create_db=new IR(Createdb, OP3(prefix,middle,""),_id,_dt);
    }
    else if(ctx->CHARACTER()){
      POP(_char_set_name)
      POP(_id)
      string middle;
      middle="NATIONAL CHARACTER SET";
      create_db=new IR(Createdb, OP3(prefix,middle,""),_id,_char_set_name);
    }
    else if(ctx->DEFAULT()){
      POP(_id)
      string middle;
      if(ctx->BIGFILE()) middle="SET DEFAULT BIGFILE TABLESPACE";
      else middle="SET DEFAULT SMALLFILE TABLESPACE";
      create_db=new IR(Createdb, OP3(prefix,middle,""),_id,NULL);
    }
    COLLECT(create_db)
    PUSH(create_db)
}

void Listener::exitPassword_value(Parser::Password_valueContext *ctx){
  IR *_value;
  POP(_value)
  IR *passwd_v=new IR(Pwdv,OP3("","",""),_value,NULL);
  COLLECT(passwd_v)
  PUSH(passwd_v)
}

/*  模板  */
void Listener::exitChar_set_name(Parser::Char_set_nameContext *ctx){
  IR *char_set_name,*_char_set_name,*_id;
  if(ctx->char_set_name()){
    POP(_char_set_name)
    POP(_id)
    string middle=".";
    char_set_name=new IR(Charsetname,OP3("",middle,""),_id,_char_set_name);
  }
  else {
    POP(_id)
    char_set_name=new IR(Charsetname,OP3("","",""),_id,NULL);
  }
  COLLECT(char_set_name)
  PUSH(char_set_name)
}

void Listener::exitCreate_table(Parser::Create_tableContext *ctx){
  /*  prefix */
  string prefix="CREATE ";
  if(ctx->TEMPORARY()){
    if(ctx->GLOBAL()) prefix+="GLOBAL ";
    else prefix+="PRIVATE ";
    prefix+="TEMPORARY ";
  }
  else if(ctx->SHARDED()) prefix+="SHARDED ";
  else if(ctx->DUPLICATED()) prefix+="DUPLICATED ";
  else if(ctx->BLOCKCHAIN()){
    prefix+="IMMUTABLE ";
    prefix+="BLOCKCHAIN ";
    }
  else prefix+="IMMUTABLE ";
  prefix+="TABLE";
  /*  middle1*/
  string middle1=".";
  string suffix1="IF NOT EXISTS ";
  string middle2="SHARING=";
  if(ctx->METADATA()) middle2+="METADATA ";
  else if(ctx->DATA()) middle2+="DATA ";
  else middle2+="NONE";
  /*  suffix  */
  string suffix="USAGE QUEUE";
  /*  先全部pop */
  IR *_mem,*_obj_tb,*_id2,*_id1;
  IR *create_tb_ir1,*create_tb_ir2,*create_tb_ir3;
  POP(_mem)
  POP(_obj_tb)
  POP(_id2)
  POP(_id1)
  create_tb_ir1=new IR(Unknown,OP3(prefix,middle1,suffix1),_id1,_id2);
  create_tb_ir2=new IR(Unknown,OP3("",middle2,""),create_tb_ir1,_obj_tb);
  create_tb_ir3=new IR(Createtb,OP3("","",suffix),create_tb_ir2,_mem);
      /*  

                          ir3
                          / \
                       ir2   _mem  
                      / \
                    ir1  _obj_tb
                    / \
                _id1  _id2

      */
  /*  收集所有ir  */
  COLLECT(create_tb_ir1)
  COLLECT(create_tb_ir2)
  COLLECT(create_tb_ir3)
  PUSH(create_tb_ir3)
}

void Listener::exitObject_table(Parser::Object_tableContext *ctx){
  string prefix="OF";
  IR *_id1;
  IR *_id2;
  IR *_obj_tb_sub;
  IR *_obj_prop_list;
  IR *_tb_prop;
  IR *obj_tb_ir1,*obj_tb_ir2;
  IR *obj_tb_ir3,*obj_tb_ir4;
  POP(_tb_prop)
  POP(_obj_prop_list)
  POP(_obj_tb_sub)
  POP(_id2)
  POP(_id1)
  /* middle相同 */
  string middle="";
  if(ctx->ON()){
    middle+="ON COMMIT ";
    if(ctx->DELETE()) middle+="DELETE ";
    else middle+="PRESERVE ";
    middle+="ROWS";
  }
    string middle1=".";
    obj_tb_ir1=new IR(Unknown,OP3(prefix,middle1,""),_id1,_id2);
    if(_obj_tb_sub) obj_tb_ir2=new IR(Unknown,OP3("","",""),obj_tb_ir1,_obj_tb_sub);
    string middle2="(";
    string suffix2=")";
    obj_tb_ir3=new IR(Unknown,OP3("",middle2,suffix2),obj_tb_ir2,_obj_prop_list);
    obj_tb_ir4=new IR(Objecttb,OP3("",middle,""),obj_tb_ir3,_tb_prop);
    
  COLLECT(obj_tb_ir1)
  COLLECT(obj_tb_ir2)
  COLLECT(obj_tb_ir3)
  COLLECT(obj_tb_ir4)
  PUSH(obj_tb_ir4)
}


void Listener::exitObject_table_substitution(Parser::Object_table_substitutionContext *ctx){
  string prefix="";
  prefix+="NOT ";
  prefix+="SUBSTITUTABLE AT ALL LEVELS";
  IR *obj_tb_sub=new IR(Objecttbsub,OP3(prefix,"",""),NULL,NULL);
  COLLECT(obj_tb_sub)
  PUSH(obj_tb_sub)
}

void Listener::exitObject_properties_list(Parser::Object_properties_listContext *ctx){
  IR *obj_prop_list,*_obj_prop,*_list;
  if(ctx->OP_COMM()){
    POP(_list)
    POP(_obj_prop)
    string middle=",";
    obj_prop_list=new IR(Charsetname,OP3("",middle,""),_obj_prop,_list);
  }
  else {
    POP(_obj_prop)
    obj_prop_list=new IR(Charsetname,OP3("","",""),_obj_prop,NULL);
  }
  COLLECT(obj_prop_list)
  PUSH(obj_prop_list)
}

void Listener::exitObject_properties(Parser::Object_propertiesContext *ctx){
  IR *obj_prop;
  if(ctx->DEFAULT()){
    IR *_id;
    IR *_exp=NULL;
    string middle="";
    POP(_exp)
    middle+="DEFAULT";
    POP(_id)
    obj_prop=new IR(Objectprop,OP3("",middle,""),_id,_exp);
  }
  else {
    IR *_out;
    POP(_out)
    obj_prop=new IR(Objectprop,OP3("","",""),_out,NULL);
  }
  COLLECT(obj_prop)
  PUSH(obj_prop)
}

void Listener::exitTable_properties(Parser::Table_propertiesContext *ctx){
    IR *_ro,*_index,*_parallel,*_endis,*_select,*_id1,*_id2;
    IR *tb_prop_ir1,*tb_prop_ir2,*tb_prop_ir3,*tb_prop_ir4,*tb_prop_ir5=NULL;
    string suffix1;
    if(ctx->CACHE()) suffix1="CACHE";
    else suffix1="NOCACHE";
    string suffix2;
    if(ctx->ROWDEPENDENCIES()) suffix2="ROWDEPENDENCIES";
    else suffix2="NOROWDEPENDENCIES";
    string suffix3="ROW ARCHIVAL ";
    if(ctx->AS()){
        string middle="AS";
        POP(_select)
        POP(_endis)
        POP(_parallel)
        POP(_index)
        POP(_ro)
        tb_prop_ir1=new IR(Unknown,OP3("","",suffix1),_ro,_index);
        tb_prop_ir2=new IR(Unknown,OP3("","",suffix2),tb_prop_ir1,_parallel);
        tb_prop_ir3=new IR(Unknown,OP3("","",suffix3),tb_prop_ir2,_endis);
        tb_prop_ir4=new IR(Tbprop,OP3("",middle,""),tb_prop_ir3,_select);
    }
    else{
        string middle1="FOR EXCHANGE WITH TABLE";
        POP(_id2)
        POP(_id1)
        POP(_endis)
        POP(_parallel)
        POP(_index)
        POP(_ro)
        tb_prop_ir1=new IR(Unknown,OP3("","",suffix1),_ro,_index);
        tb_prop_ir2=new IR(Unknown,OP3("","",suffix2),tb_prop_ir1,_parallel);
        tb_prop_ir3=new IR(Unknown,OP3("","",suffix3),tb_prop_ir2,_endis);
        tb_prop_ir4=new IR(Unknown,OP3("",middle1,""),tb_prop_ir3,_id1);
        string middle2=".";
        tb_prop_ir5=new IR(Tbprop,OP3("",middle2,""),tb_prop_ir4,_id2);
    }
    COLLECT(tb_prop_ir1)
    COLLECT(tb_prop_ir2)
    COLLECT(tb_prop_ir3)
    COLLECT(tb_prop_ir4)
    if(tb_prop_ir5) COLLECT(tb_prop_ir5)
    if(tb_prop_ir5) PUSH(tb_prop_ir5)
    else PUSH(tb_prop_ir4)

}

void Listener::exitRead_only_clause(Parser::Read_only_clauseContext *ctx){
    string prefix="READ ";
    if(ctx->ONLY()) prefix+="ONLY";
    else prefix+="WRITE";
    IR *ro=new IR(Roclause,OP3(prefix,"",""),NULL,NULL);
    COLLECT(ro)
    PUSH(ro)
}

void Listener::exitIndexing_clause(Parser::Indexing_clauseContext *ctx){
    string prefix="INDEXING ";
    if(ctx->ON()) prefix+="ON";
    else prefix+="OFF";
    IR *index=new IR(Indexclause,OP3(prefix,"",""),NULL,NULL);
    COLLECT(index)
    PUSH(index)
}

void Listener::exitParallel_clause(Parser::Parallel_clauseContext *ctx){
    string prefix;
    IR *_dt=NULL;
    if(ctx->NOPARALLEL()) prefix="NOPARALLEL";
    else {
      prefix="PARALLEL";
      POP(_dt)
    }
    IR *parallel=new IR(Parallelclause,OP3(prefix,"",""),_dt,NULL);
    COLLECT(parallel)
    PUSH(parallel)
}

void Listener::exitDrop_stmt(Parser::Drop_stmtContext *ctx){
    IR *_drop_one;
    POP(_drop_one)
    IR *drop_stmt=new IR(Dropstmt,OP3("","",""),_drop_one,NULL);
    COLLECT(drop_stmt)
    PUSH(drop_stmt)
}

void Listener::exitDrop_database(Parser::Drop_databaseContext *ctx){
    string prefix="DROP DATABASE INCLUDING BACKUPS NOPROMPT";
    IR *drop_db=new IR(Dropdb,OP3(prefix,"",""),NULL,NULL);
    COLLECT(drop_db)
    PUSH(drop_db)
}

void Listener::exitDrop_table(Parser::Drop_tableContext *ctx){
    IR *_id;
    POP(_id)
    string prefix="DROP TABLE";
    string middle="IF EXISTS PURGE";
    IR *drop_tb=new IR(Droptb,OP3(prefix,middle,""),_id,NULL);
    COLLECT(drop_tb)
    PUSH(drop_tb)
}

void Listener::exitSelect_stmt(Parser::Select_stmtContext *ctx){
    string prefix="SELECT ";
    if(ctx->DISTINCT()) prefix+="DISTINCT";
    else if(ctx->UNIQUE()) prefix+="UNIQUE";
    else prefix+="ALL";
    IR *_list,*_from,*_where,*_group,*_order;
    POP(_order)
    POP(_group)
    POP(_where)
    POP(_from)
    POP(_list)
    IR *select_stmt_ir1=new IR(Unknown,OP3(prefix,"",""),_list,_from);
    IR *select_stmt_ir2=new IR(Unknown,OP3("","",""),select_stmt_ir1,_where);
    IR *select_stmt_ir3=new IR(Unknown,OP3("","",""),select_stmt_ir2,_group);
    IR *select_stmt_ir4=new IR(Unknown,OP3("","",""),select_stmt_ir3,_group);
    IR *select_stmt_ir5=new IR(Selectstmt,OP3("","",""),select_stmt_ir4,_order);
    COLLECT(select_stmt_ir1)
    COLLECT(select_stmt_ir2)
    COLLECT(select_stmt_ir3)
    COLLECT(select_stmt_ir4)
    COLLECT(select_stmt_ir5)
    PUSH(select_stmt_ir5)
}

void Listener::exitSelected_list(Parser::Selected_listContext *ctx){
    IR *selected_list;
    IR *_id,*_list;
    if(ctx->Asterisk()){
        string prefix="*";
        selected_list=new IR(Selectedl,OP3(prefix,"",""),NULL,NULL);
    }
    else if(ctx->OP_COMM()){
        string middle=",";
        POP(_list)
        POP(_id)
        selected_list=new IR(Selectedl,OP3("",middle,""),_id,_list);
    }
    else {
        POP(_id)
        selected_list=new IR(Selectedl,OP3("","",""),_id,_list);
    }
    COLLECT(selected_list)
    PUSH(selected_list)
}

void Listener::exitFrom_clause(Parser::From_clauseContext *ctx){
    string prefix="FROM";
    IR *_tb_ref;
    POP(_tb_ref)
    IR *from=new IR(Fromclause,OP3(prefix,"",""),_tb_ref,NULL);
    COLLECT(from)
    PUSH(from)
}

void Listener::exitTable_ref_list(Parser::Table_ref_listContext *ctx){
    IR *tb_ref_list,*_tb_ref,*_id;
  if(ctx->table_ref_list()){
    POP(_tb_ref)
    POP(_id)
    string middle=".";
    tb_ref_list=new IR(Charsetname,OP3("",middle,""),_id,_tb_ref);
  }
  else {
    POP(_id)
    tb_ref_list=new IR(Charsetname,OP3("","",""),_id,NULL);
  }
  COLLECT(tb_ref_list)
  PUSH(tb_ref_list)
}


void Listener::exitWhere_clause(Parser::Where_clauseContext *ctx){
    string prefix="WHERE";
    IR *_exp;
    POP(_exp)
    IR *where=new IR(Whereclause,OP3(prefix,"",""),_exp,NULL);
    COLLECT(where)
    PUSH(where)
}

void Listener::exitExpression(Parser::ExpressionContext *ctx){
    IR *_id1,*_id2,*_op;
    POP(_id2)
    POP(_op)
    POP(_id1)
    IR *exp_ir1=new IR(Unknown,OP3("","",""),_id1,_op);
    IR *exp_ir2=new IR(Exp,OP3("","",""),exp_ir1,_id2);
    COLLECT(exp_ir1)
    COLLECT(exp_ir2)
    PUSH(exp_ir2)
}

void Listener::exitOperator(Parser::OperatorContext *ctx){
    string prefix;
    prefix="=";
    //简单写
    IR *op=new IR(Op,OP3(prefix,"",""),NULL,NULL);
    COLLECT(op)
    PUSH(op)
}

void Listener::exitGroup_by_clause(Parser::Group_by_clauseContext *ctx){
    string prefix="GROUP BY";
    IR *_id,*_have;
    POP(_have)
    POP(_id)
    IR *group_by=new IR(Groupbyclause,OP3(prefix,"",""),_id,_have);
    COLLECT(group_by)
    PUSH(group_by)
}

void Listener::exitHaving_clause(Parser::Having_clauseContext *ctx){
    string prefix="HAVING";
    IR *_exp;
    POP(_exp)
    IR *having=new IR(Havingclause,OP3(prefix,"",""),_exp,NULL);
    COLLECT(having)
    PUSH(having)
}

void Listener::exitOrder_by_clause(Parser::Order_by_clauseContext *ctx){
    string prefix="ORDER SIBLINGS BY";
    IR *_order;
    POP(_order)
    IR *order_by=new IR(Orderbyclause,OP3(prefix,"",""),_order,NULL);
    COLLECT(order_by)
    PUSH(order_by)
}

void Listener::exitOrder_by_elements(Parser::Order_by_elementsContext *ctx){
    string middle;
    if(ctx->ASC()) middle="ASC";
    else middle="DESC";
    middle+="NULLS ";
    if(ctx->FIRST()) middle+="FIRST";
    else middle+="LAST";
    IR *_id;
    POP(_id)
    IR *order_by_ele=new IR(Orderbyele,OP3("",middle,""),_id,NULL);
    COLLECT(order_by_ele)
    PUSH(order_by_ele)
}

void Listener::exitUpdate_stmt(Parser::Update_stmtContext *ctx){
    string prefix="UPDATE";
    IR *_id,*_set,*_where;
    POP(_where)
    POP(_set)
    POP(_id)
    IR *update_stmt_ir1=new IR(Unknown,OP3(prefix,"",""),_id,_set);
    COLLECT(update_stmt_ir1)
    PUSH(update_stmt_ir1)
    IR *update_stmt_ir2=new IR(Updatestmt,OP3(prefix,"",""),update_stmt_ir1,_where);
    COLLECT(update_stmt_ir2)
    PUSH(update_stmt_ir2)
}

void Listener::exitUpdate_set_clause(Parser::Update_set_clauseContext *ctx){
    string prefix="SET";
    IR *_one_list,*_id1,*_id2,*_dt;
    IR *update_set;
    if(ctx->update_set_clause_one_list()){
        POP(_one_list)
        update_set=new IR(Updatesetclause,OP3(prefix,"",""),_one_list,NULL);
    }
    else{
        prefix+="VALUE (";
        string middle=") =";
        if(ctx->datatype()) {POP(_dt)}
        else {POP(_id2)}
        POP(_id1)
        if(_dt) update_set=new IR(Updatesetclause,OP3(prefix,middle,""),_id1,_dt);
        else update_set=new IR(Updatesetclause,OP3(prefix,middle,""),_id1,_id2);
    }
    COLLECT(update_set)
    PUSH(update_set)
}

void Listener::exitUpdate_set_clause_one_list(Parser::Update_set_clause_one_listContext *ctx){
    IR *update_set_c_one_list,*_one,*_one_list;
  if(ctx->update_set_clause_one_list()){
    POP(_one_list)
    POP(_one)
    string middle=",";
    update_set_c_one_list=new IR(Updatesetclauseonelist,OP3("",middle,""),_one,_one_list);
  }
  else {
    POP(_one)
    update_set_c_one_list=new IR(Updatesetclauseonelist,OP3("","",""),_one,NULL);
  }
  COLLECT(update_set_c_one_list)
  PUSH(update_set_c_one_list)
}

void Listener::exitUpdate_set_clause_one(Parser::Update_set_clause_oneContext *ctx){
    IR *_id1,*_select,*_id2,*_dt;
    IR *update_set_c_one;
    if(ctx->select_stmt()){
        POP(_select)
        POP(_id1)
        string middle="=(";
        string suffix=")";
        update_set_c_one=new IR(Updatesetclauseone,OP3("",middle,suffix),_id1,_select);
    }
    else {
        if(ctx->datatype()) {POP(_dt)}
        else {POP(_id2)}
        POP(_id1)
        string middle="=";
        if(_dt) update_set_c_one=new IR(Updatesetclauseone,OP3("",middle,""),_id1,_dt);
        else update_set_c_one=new IR(Updatesetclauseone,OP3("",middle,""),_id1,_id2);
    }
    COLLECT(update_set_c_one)
  PUSH(update_set_c_one)
}

void Listener::exitInsert_stmt(Parser::Insert_stmtContext *ctx){
    string prefix="INSERT";
    IR *_into,*_values,*_select;
    IR *insert_stmt;
    if(ctx->values_clause()) {POP(_values)}
    else {POP(_select)}
    POP(_into)
    if(_values) insert_stmt=new IR(Insertstmt,OP3(prefix,"",""),_into,_values);
    else insert_stmt=new IR(Insertstmt,OP3(prefix,"",""),_into,_select);
    COLLECT(insert_stmt)
  PUSH(insert_stmt)
}

void Listener::exitInsert_into_clause(Parser::Insert_into_clauseContext *ctx){
    string prefix="INTO";
    IR *_paren,*_id;
    IR *insert_into;
    POP(_paren)
    POP(_id)
    insert_into=new IR(Insertinto,OP3(prefix,"",""),_id,_paren);
    COLLECT(insert_into)
   PUSH(insert_into)
}

void Listener::exitParen_column_list(Parser::Paren_column_listContext *ctx){
    string prefix="(";
    string middle=")";
    IR *_column;
    POP(_column)
    IR *paren_column=new IR(Parencolumnl,OP3(prefix,middle,""),_column,NULL);
    COLLECT(paren_column)
   PUSH(paren_column)
}

void Listener::exitColumn_list(Parser::Column_listContext *ctx){
     IR *column_list,*_id,*_one_list;
  if(ctx->column_list()){
    POP(_one_list)
    POP(_id)
    string middle=",";
    column_list=new IR(Updatesetclauseonelist,OP3("",middle,""),_id,_one_list);
  }
  else {
    POP(_id)
    column_list=new IR(Updatesetclauseonelist,OP3("","",""),_id,NULL);
  }
  COLLECT(column_list)
  PUSH(column_list)
}

void Listener::exitValues_clause(Parser::Values_clauseContext *ctx){
    string prefix="VALUES ";
    IR *_exp;
    IR *values;
    if(ctx->REGULAR_ID()){
        prefix+="REGULAR_ID";
        values=new IR(Vclause,OP3(prefix,"",""),NULL,NULL);
    }
    else {
        prefix+="(";
        string middle=")";
        POP(_exp)
        values=new IR(Vclause,OP3(prefix,middle,""),_exp,NULL);
    
    }
    COLLECT(values)
  PUSH(values)
}

void Listener::exitAlter_stmt(Parser::Alter_stmtContext *ctx){
  IR *_alter_one;
  POP(_alter_one)
  IR *alter_stmt=new IR(Alterstmt,OP3("","",""),_alter_one,NULL);
  COLLECT(alter_stmt)
  PUSH(alter_stmt)
}

void Listener::exitAlter_database(Parser::Alter_databaseContext *ctx){
  string prefix="ALTER";
  IR *_database,*_one;
  POP(_one)
  POP(_database)
  IR *alter_db=new IR(Alterdb,OP3(prefix,"",""),_database,_one);
  COLLECT(alter_db)
  PUSH(alter_db)
}

void Listener::exitDatabase_clause(Parser::Database_clauseContext *ctx){
  string prefix="PLUGGABLE DATABASE";
  IR *_id;
  POP(_id)
  IR *db=new IR(Dbclause,OP3(prefix,"",""),_id,NULL);
  COLLECT(db)
  PUSH(db)
}

void Listener::exitStartup_clauses(Parser::Startup_clausesContext *ctx){
  string prefix;
  if(ctx->MOUNT()){
    prefix="MOUNT ";
    if(ctx->CLONE()) prefix+="CLONE";
    else prefix+="STANDBY";
    prefix+="DATABASE";
  }
  else if(ctx->ONLY()) prefix="OPEN READ ONLY";
  else prefix="OPEN READ WRITE";
  IR *startup=new IR(Startupclause,OP3(prefix,"",""),NULL,NULL);
  COLLECT(startup)
  PUSH(startup)
}

void Listener::exitDatabase_file_clauses(Parser::Database_file_clausesContext *ctx){
  string prefix="RENAME FILE";
  string middle="TO";
  IR *_id1,*_id2;
  POP(_id1)
  POP(_id2)
  IR *db_file=new IR(Dbfileclause,OP3(prefix,middle,""),_id1,_id2);
  COLLECT(db_file)
  PUSH(db_file)
}

void Listener::exitControlfile_clauses(Parser::Controlfile_clausesContext *ctx){
  string prefix;
  IR *_id,*_trace;
  IR *controlfile;
  if(ctx->CREATE()){
    prefix="CREATE ";
    if(ctx->LOGICAL()) prefix+="LOGICAL ";
    else prefix+="PHYSICAL ";
    prefix+="STANDBY CONTROLFILE AS";
    string suffix="REUSE";
    POP(_id)
    controlfile=new IR(Controlfileclause,OP3(prefix,"",suffix),_id,NULL);
  }
  else {
    prefix="BACKUP CONTROLFILE TO";
    string suffix="";
    if(ctx->REUSE()) suffix +="REUSE";
    POP(_id)
    controlfile=new IR(Controlfileclause,OP3(prefix,"",suffix),_id,NULL);
  }
  COLLECT(controlfile)
  PUSH(controlfile)
}

void Listener::exitTrace_file_clause(Parser::Trace_file_clauseContext *ctx){
  string prefix="TRACE AS";
  string suffix="REUSE ";
  if(ctx->RESETLOGS()) suffix+="RESETLOGS";
  else suffix+="NORESETLOGS";
  IR *_id;
  POP(_id)
  IR *tracefile=new IR(Tracefileclause,OP3(prefix,"",suffix),_id,NULL);
  COLLECT(tracefile)
  PUSH(tracefile)
}

void Listener::exitInstance_clauses(Parser::Instance_clausesContext *ctx){
  string suffix="INSTANCE CHAR_STRING";
  IR *_endis;
  POP(_endis)
  IR *instance=new IR(Instanceclause,OP3("","",suffix),_endis,NULL);
  COLLECT(instance)
  PUSH(instance)
}

void Listener::exitEnable_or_disable(Parser::Enable_or_disableContext *ctx){
  string prefix;
  if(ctx->ENABLE()) prefix="ENABLE";
  else prefix="DISABLE";
  IR *en_or_dis=new IR(Eode,OP3(prefix,"",""),NULL,NULL);
  COLLECT(en_or_dis)
  PUSH(en_or_dis)
}

void Listener::exitSecurity_clause(Parser::Security_clauseContext *ctx){
  string prefix="GUARD ";
  if(ctx->ALL()) prefix+="ALL";
  else if(ctx->NONE()) prefix+="NONE";
  else prefix+="STANDBY";
  IR *security=new IR(Secclause,OP3(prefix,"",""),NULL,NULL);
  COLLECT(security)
  PUSH(security)
}

void Listener::exitAlter_table(Parser::Alter_tableContext *ctx){
  string prefix="ALTER TABLE";
  IR *_id,*_mem,*_one,*_endis;
  POP(_endis);
  POP(_one)
  POP(_mem)
  POP(_id)
  IR *alter_tb_ir1=new IR(Unknown,OP3(prefix,"",""),_id,_mem);
  IR *alter_tb_ir2=new IR(Unknown,OP3("","",""),alter_tb_ir1,_one);
  string suffix;
  if(ctx->LOCK()) suffix="TABLE LOCK";
  else suffix="ALL TRIGGERS";
  IR *alter_tb_ir3=new IR(Altertb,OP3("","",suffix),alter_tb_ir2,_endis);
  COLLECT(alter_tb_ir1)
  COLLECT(alter_tb_ir2)
  COLLECT(alter_tb_ir3)
  PUSH(alter_tb_ir3)
}

void Listener::exitMemoptimize_read_write_clause(Parser::Memoptimize_read_write_clauseContext *ctx){
  string prefix="NO MEMOPTIMIZE FOR ";
  if(ctx->READ()) prefix+="READ";
  else prefix+="WRITE";
  IR *mem=new IR(Memoptrwclause,OP3(prefix,"",""),NULL,NULL);
  COLLECT(mem)
  PUSH(mem)
}

void Listener::exitAlter_table_properties(Parser::Alter_table_propertiesContext *ctx){
  string prefix="";
  IR *_one=NULL;
  IR *alter_tb_prop;
  if(ctx->RENAME()) {
    prefix+="RENAME TO";
    POP(_one)
  }
  else if(ctx->ONLY()) prefix+="READ ONLY";
  else if(ctx->WRITE()) prefix+="READ WRITE";
  else if(ctx->REKEY()) prefix+="REKEY CHAR_STRING";
  else {POP(_one)}
  alter_tb_prop=new IR(Altertbprop,OP3(prefix,"",""),_one,NULL);
  COLLECT(alter_tb_prop)
  PUSH(alter_tb_prop)
}

void Listener::exitShrink_clause(Parser::Shrink_clauseContext *ctx){
  string prefix="SHRINK SPACE_KEYWORD COMPACT CASCADE";
  IR *shrink=new IR(Shrinkclause,OP3(prefix,"",""),NULL,NULL);
  COLLECT(shrink)
  PUSH(shrink)
}

void Listener::exitConstraint_clauses(Parser::Constraint_clausesContext *ctx){
  string prefix;
  string suffix="";
  string middle="";
  IR *_one=NULL;
  IR *_two=NULL;
  IR *constraint;
  if(ctx->out_of_line_constraint_list()){
    prefix="ADD (";
    POP(_one)
    suffix+=")";
  }
  else if(ctx->out_of_line_constraint()){
    prefix="ADD";
    POP(_one)
  }
  else if(ctx->MODIFY()){
    prefix="MODIFY ";
    if(ctx->CONSTRAINT()) {
      prefix+="CONSTRAINT";
      POP(_two)
      POP(_one)
    }
    else if(ctx->UNIQUE()){
      prefix+="UNIQUE (";
      middle+=")";
      POP(_two)
      POP(_one)
    }
    else {
      prefix+="PRIMARY KEY";
      POP(_one)
    }
    suffix+="CASCADE";
  }
  else if(ctx->RENAME()){
    prefix="RENAME CONSTRAINT";
    POP(_two)
    POP(_one)
    middle+="TO";
  }
  else {POP(_one)}
  constraint=new IR(Constraintclause,OP3(prefix,middle,suffix),_one,_two);
  COLLECT(constraint)
  PUSH(constraint)
}

void Listener::exitOut_of_line_constraint_list(Parser::Out_of_line_constraint_listContext *ctx){
  IR *out_of_line_list,*_list,*_one;
  if(ctx->out_of_line_constraint_list()){
    POP(_list)
    POP(_one)
    string middle=",";
    out_of_line_list=new IR(Outoflinecon,OP3("",middle,""),_one,_list);
  }
  else {
    POP(_one)
    out_of_line_list=new IR(Outoflinecon,OP3("","",""),_one,NULL);
  }
  COLLECT(out_of_line_list)
  PUSH(out_of_line_list)
}

void Listener::exitOut_of_line_constraint(Parser::Out_of_line_constraintContext *ctx){
  string prefix="CONSTRAINT";
  string middle="";
  string suffix="";
  IR *_id,*_one,*_state;
  POP(_state)
  POP(_one)
  POP(_id)
  if(ctx->UNIQUE()) {
    middle+="UNIQUE (";
    suffix+=")";
  }
  else if(ctx->PRIMARY()){
    middle+="PRIMARY KEY (";
    suffix+=")";
  }
  else if(ctx->CHECK()){
    middle+="CHECK (";
    suffix+=")";
  }
  IR *out_of_line_ir1=new IR(Unknown,OP3(prefix,middle,suffix),_id,_one);
  IR *out_of_line_ir2=new IR(Outoflinecon,OP3("","",""),out_of_line_ir1,_state);
  COLLECT(out_of_line_ir1)
  COLLECT(out_of_line_ir2)
  PUSH(out_of_line_ir2)
}

void Listener::exitForeign_key_clause(Parser::Foreign_key_clauseContext *ctx){
  string prefix="FOREIGN KEY";
  IR *_paren,*_ref,*_on;
  POP(_on)
  POP(_ref)
  POP(_paren)
  IR *foreign_key_ir1=new IR(Unknown,OP3(prefix,"",""),_paren,_ref);
  IR *foreign_key_ir2=new IR(Foreignkeyclause,OP3("","",""),foreign_key_ir1,_on);
  COLLECT(foreign_key_ir1)
  COLLECT(foreign_key_ir2)
  PUSH(foreign_key_ir2)
}

void Listener::exitReferences_clause(Parser::References_clauseContext *ctx){
  string prefix="REFERENCES";
  IR *_id,*_paren,*_on;
  POP(_on)
  POP(_paren)
  POP(_id)
  IR *ref_ir1=new IR(Unknown,OP3(prefix,"",""),_id,_paren);
  IR *ref_ir2=new IR(Refclause,OP3("","",""),ref_ir1,_on);
  COLLECT(ref_ir1)
  COLLECT(ref_ir2)
  PUSH(ref_ir2)
}

void Listener::exitOn_delete_clause(Parser::On_delete_clauseContext *ctx){
  IR *on_delete;
  string prefix="ON DELETE ";
  if(ctx->CASCADE()) prefix+="CASCADE";
  else prefix+="SET NULL_";
  on_delete=new IR(Ondeleteclause,OP3("","",""),NULL,NULL);
  COLLECT(on_delete)
  PUSH(on_delete)
}

void Listener::exitConstraint_state(Parser::Constraint_stateContext *ctx){
  IR *state;
  string prefix="";
  if(ctx->DEFERRABLE()) prefix+="NOT DEFERRABLE";
  else if(ctx->RELY()) prefix+="RELY";
  else if(ctx->NORELY()) prefix+="NORELY";
  else if(ctx->ENABLE()) prefix+="ENABLE";
  else prefix+="DISABLE";
  state=new IR(Constraintstate,OP3(prefix,"",""),NULL,NULL);
  COLLECT(state)
  PUSH(state)
}

void Listener::exitDrop_constraint_clause(Parser::Drop_constraint_clauseContext *ctx){
  string prefix="DROP ";
  string middle="";
  IR *_one=NULL;
  if(ctx->PRIMARY()){
    prefix+="PRIMARY KEY";
    prefix+="CASCADE";
  }
  else if(ctx->UNIQUE()){
    prefix+="UNIQUE (";
    POP(_one)
    middle+=") CASCADE";
  }
  else{
    prefix+="CONSTRAINT";
    POP(_one)
    middle+="CASCADE";
  }
  IR *drop_cons=new IR(Dropconclause,OP3(prefix,middle,""),_one,NULL);
  COLLECT(drop_cons)
  PUSH(drop_cons)
}

void Listener::exitEnable_disable_clause(Parser::Enable_disable_clauseContext *ctx){
  string prefix;
  string middle="";
  IR *_one=NULL;
  if(ctx->ENABLE()) prefix="ENABLE ";
  else prefix="DISABLE ";
  if(ctx->CONSTRAINT()){
    prefix+="CONSTRAINT";
    POP(_one)
    middle+="CASCADE";
  }
  else if(ctx->UNIQUE()){
    prefix+="UNIQUE (";
    POP(_one)
    middle+=") CASCADE";
  }
  else prefix+="PRIMARY KEY CASCADE";
  IR *en_dis=new IR(Edeclause,OP3(prefix,middle,""),_one,NULL);
  COLLECT(en_dis)
  PUSH(en_dis)
}

void Listener::exitIdentifier_list(Parser::Identifier_listContext *ctx){
  IR *id_list,*_list,*_id;
  if(ctx->identifier_list()){
    POP(_list)
    POP(_id)
    string middle=",";
    id_list=new IR(Charsetname,OP3("",middle,""),_id,_list);
  }
  else {
    POP(_id)
    id_list=new IR(Charsetname,OP3("","",""),_id,NULL);
  }
  COLLECT(id_list)
  PUSH(id_list)
}

void Listener::exitIdentifier(Parser::IdentifierContext *ctx){
  antlr4::tree::TerminalNode *text;
  if(ctx->DELIMITED_ID()) text=ctx->DELIMITED_ID();
  else if(ctx->CHAR_STRING()) text=ctx->CHAR_STRING();
  else text=ctx->STR();
  string str=text->getText();
  IR *id=new IR(ID,str,kSTR_DATA);
  COLLECT(id)
  PUSH(id)
}

void Listener::exitDatatype(Parser::DatatypeContext *ctx){
  IR *data;
  antlr4::tree::TerminalNode *text;
  if(ctx->INTEGER_STR()) {
    text=ctx->INTEGER_STR();
    int ival=stof(text->getText());
    data=new IR(INT,ival,kINT_DATA);
  }
  else {
    text=ctx->FLOAT_STR();
    float fval=stoi(text->getText());
    data=new IR(FLOAT,fval,kINT_DATA);
  }
  COLLECT(data)
  PUSH(data)
}

void trim_string(string &res) {
  int count = 0;
  int idx = 0;
  bool expect_space = false;
  for (int i = 0; i < res.size(); i++) {
    if (res[i] == ';' && i != res.size() - 1) {
      res[i + 1] = '\n';
    }
    if (res[i] == ' ') {
      if (expect_space == false) {
        continue;
      } else {
        expect_space = false;
        res[idx++] = res[i];
        count++;
      }
    } else {
      expect_space = true;
      res[idx++] = res[i];
      count++;
    }
  }

  res.resize(count);
}

string IR::to_string() {
  auto res = to_string_core();
  trim_string(res);
  return res;
}

string IR::to_string_core() {
  switch (type_) {
    case INT:
      return std::to_string(ival);
    case FLOAT:
      return std::to_string(fval);
    case ID:
      return str;
    case STR:
      return str;
  }
  string res="";
  if (op_ != NULL) res += op_->prefix_ + " ";
  if (left_ != NULL) {
    if(op_ !=NULL && op_->middle_.compare(".") ==0 )
      res+=left_->to_string_core();
    else
      res += left_->to_string_core()+" ";
  }
  if (op_ != NULL) {
    if(op_->middle_.compare(".") ==0) 
      res += op_->middle_;
    else
      res += op_->middle_ + " ";
  }
  if (right_ != NULL) res += right_->to_string_core()+" ";
  if (op_ != NULL) res += op_->suffix_+" ";
  return res;
}