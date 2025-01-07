#include "/usr/local/hiredis-1.2.0/hiredis.h"

redisContext *Connect(){
	// cgo string -> char*
	redisContext *res = redisConnect("127.0.0.1",6379);
	if (res == NULL || res->err != 0 ){
		return NULL ;
	}
	return res;
}

redisReply *redisCommand_wrapper(redisContext * rc,const char *format,char *arg){
	redisReply *res;
	res = redisCommand(rc,format,arg);
	if(res->type == REDIS_REPLY_ERROR) printf("EXEC EERROR");
	return res;
}

// return tuple (name,type,parent_name)
void Collect_metadata(redisContext *con){
	redisCommand_wrapper(con,"%s","MULTI\n");
}

int main(){
    Collect_metadata(Connect());
}

