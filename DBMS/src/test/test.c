#include "/usr/local/hiredis-1.2.0/hiredis.h"

redisContext *Connect(){
	// cgo string -> char*
	redisContext *res = redisConnect("127.0.0.1",6379);
	if (res == NULL || res->err != 0 ){
		return NULL ;
	}
	return res;
}

// return tuple (name,type,parent_name)
void Collect_metadata(redisContext *con){
	char *libstr = "FUNCTION LIST";
    redisReply *res = redisCommand(con,libstr);
	size_t num = res->elements;
	printf("lib num:%d\n",num);
	printf("func num:%d\n",res->element[0]->element[5]->elements);
	printf("lib name:%s\n",res->element[0]->element[1]->str);
	printf("func name:%s\n",res->element[0]->element[5]->element[0]->element[1]->str);
}

int main(){
    Collect_metadata(Connect());
}

