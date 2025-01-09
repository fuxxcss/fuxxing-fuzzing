package db

/*
#cgo LDFLAGS: -L/usr/local/hiredis-1.2.0/ -lhiredis
#include "/usr/local/hiredis-1.2.0/hiredis.h"
#include <stdio.h>

redisReply *redisCommand_wrapper(redisContext * rc,const char *format,char *arg){
	redisReply *res;
	res = redisCommand(rc,format,arg);
	if(res->type == REDIS_REPLY_ERROR) printf("%s %s: ERROR\n",format,arg);
	return res;
}

redisReply *redisCommand_one(redisContext * rc,const char *format){
	redisReply *res;
	res = redisCommand(rc,format);
	if(res->type == REDIS_REPLY_ERROR) printf("%s: ERROR\n",format);
	return res;
}

void redisReply_free(redisReply *res){
	freeReplyObject(res);
}

int redisReply_type(redisReply *res){
	return res->type;
}

redisReply *redisReply_element(redisReply *res,size_t index){
	return res->element[index];
}

*/
import "C"

import (
	"os"
	"os/exec"
	//"fmt"
	"DBMS/gramfree"
)

// Commands
var (
	Ping = C.CString("PING")
	Flushall = C.CString("FLUSHALL")
	KEYS = C.CString("KEYS *")
	TYPE = C.CString("TYPE %s")
	HKeys = C.CString("HKEYS %s")
	LIST = C.CString("FUNCTION LIST")
)

type RedisClient struct {
	conn_ip string
	conn_port int
	conn *C.redisContext
}

func (self *RedisClient) Connect(ip string,port int) bool {
	// cgo string -> char*
	conn := C.redisConnect(C.CString(ip),C.int(port))
	if conn == nil || conn.err != 0 {
		return false
	}
	self.conn = conn
	return true
}

func (self *RedisClient) Reconnect() bool {
	ret := C.redisReconnect(self.conn)
	if ret == C.REDIS_ERR {
		return false
	}
	return true
}

func (self *RedisClient) Check_alive() bool {
	res := C.redisCommand_one(self.conn,Ping)
	if res == nil || C.redisReply_type(res) == C.REDIS_REPLY_ERROR { return false }
	return true
}

func (self *RedisClient) Clean_up() bool {
	res := C.redisCommand_one(self.conn,Flushall)
	if res == nil || C.redisReply_type(res) == C.REDIS_REPLY_ERROR { return false }
	return true
}

func (self *RedisClient) Execute(command string) gramfree.DBMStatus {
	//fmt.Println(command)
	cstr := C.CString(command)
	res := C.redisCommand_one(self.conn,cstr)
	//fmt.Println(C.GoString(res.str))
	if C.redisReply_type(res) == C.REDIS_REPLY_ERROR {
		// Crash
		if !self.Check_alive() {
			return gramfree.Crash
		// ExecError
		}else {
			return gramfree.ExecError
		}
	}
	return gramfree.Normal
}

// return tuple (name,type,parent_name)
func (self *RedisClient) Collect_metadata() [][3]string {
	res := C.redisCommand_one(self.conn,KEYS)
	ret := make([][3]string,0)
	var tuple,ftuple [3]string
	index := int(res.elements) -1
	// collect all types 
	for ; index >= 0 ; index -- {
		name := C.redisReply_element(res,C.size_t(index)).str
		// type
		var tp string
		types := C.redisCommand_wrapper(self.conn,TYPE,name).str
		switch C.GoString(types) {
		case "string" :
			tp = "STR"
		case "hash" :
			tp = "HASH"
			fields := C.redisCommand_wrapper(self.conn,HKeys,name)
			findex := int(fields.elements) -1
			for ; findex >= 0 ; findex -- {
				fname := C.redisReply_element(fields,C.size_t(findex)).str
				ftuple[0] = C.GoString(fname)
				ftuple[1] = "FIELD"
				ftuple[2] = C.GoString(name)
				ret = append(ret,ftuple)
			}
		case "list" :
			tp = "LIST"
		case "set" :
			tp = "SET"
		case "ZSET" :
			tp = "zset"
		case "stream" :
			tp = "STREAM"
		case "geo" :
			tp = "GEO"
		default :
			tp = ""
		}
		tuple[0] = C.GoString(name)
		tuple[1] = tp
		// parent_name
		tuple[2] = ""
		ret = append(ret,tuple)
	}
	res = C.redisCommand_one(self.conn,LIST)
	index = int(res.elements) -1
	// collect libs and funcs
	for ; index >= 0 ; index -- {
		lib := C.redisReply_element(res,C.size_t(index))
		name := C.redisReply_element(lib,C.size_t(1)).str
		funcs := C.redisReply_element(lib,C.size_t(5))
		findex := funcs.elements -1
		for ; findex >= 0 ; findex -- {
			one_func := C.redisReply_element(funcs,C.size_t(findex))
			fname := C.redisReply_element(one_func,C.size_t(1)).str
			ftuple[0] = C.GoString(fname)
			ftuple[1] = "FUNC"
			ftuple[2] = C.GoString(name)
			ret = append(ret,ftuple)
		}
		tuple[0] = C.GoString(name)
		tuple[1] = "LIB"
		// parent_name
		tuple[2] = ""
		ret = append(ret,tuple)
	}
	return ret
}

func (self *RedisClient) Restart() bool {
	os.Setenv("AFL_MAP_SIZE",gramfree.AFL_MAP_SIZE)
	os.Setenv("__AFL_SHM_ID",gramfree.AFL_SHM_ID)
	program := "/usr/local/redis/src/redis-server"
	arg1 := "&"
	cmd := exec.Command(program,arg1)
	err := cmd.Run()
	if err != nil {
		return false
	}
	return true
}