package db

// add #define type Type before redisReply

/*
#cgo LDFLAGS: -L/usr/local/hiredis-1.2.0/ -lhiredis
#include "/usr/local/hiredis-1.2.0/hiredis.h"

redisReply *redisCommand_wrapper(redisContext * rc,const char *format,char *arg){
	redisReply *res;
	res = redisCommand(rc,format,arg);
	return res;
}

char *redisReply_element(redisReply *res,size_t index){
	return res->element[index]->str;
}

*/
import "C"

import (
	"os"
	"os/exec"
	//"fmt"
	"strings"
	"DBMS/gramfree"
)

// Commands
var (
	Format = C.CString("%s")
	Ping = C.CString("PING")
	Flushall = C.CString("FLUSHALL")
	TYPE = C.CString("TYPE %s")
	HKeys = C.CString("HKEYS %s")
)

type RedisClient struct {
	conn_ip string
	conn_port int
	status gramfree.DBMStatus
	conn *C.redisContext
}

func (self *RedisClient) Connect(ip string,port int) bool {
	// cgo string -> char*
	conn := C.redisConnect(C.CString(ip),C.int(port))
	if conn == nil || conn.err != 0 {
		self.status = gramfree.ConnectError
		return false
	}
	self.conn = conn
	return true
}

func (self *RedisClient) Reconnect() bool {
	ret := C.redisReconnect(self.conn)
	if ret == C.REDIS_ERR {
		self.status = gramfree.ConnectError
		return false
	}
	return true
}

func (self *RedisClient) Check_alive() bool {
	res := C.redisCommand_wrapper(self.conn,Format,Ping)
	if res == nil || res.Type == C.REDIS_REPLY_ERROR { return false }
	return true
}

func (self *RedisClient) Clean_up() bool {
	res := C.redisCommand_wrapper(self.conn,Format,Flushall)
	if res == nil || res.Type == C.REDIS_REPLY_ERROR { return false }
	return true
}

func (self *RedisClient) Execute(command string) gramfree.DBMStatus {
	cstr := C.CString(command)
	res := C.redisCommand_wrapper(self.conn,Format,cstr)
	if res.Type == C.REDIS_REPLY_ERROR {
		// Crash
		if strings.Contains(C.GoString(res.str),"Server is down") {
			self.status = gramfree.Crash
			return gramfree.Crash
		// ExecError
		}else {
			self.status = gramfree.ExecError
			return gramfree.ExecError
		}
	}
	self.status = gramfree.Normal
	return gramfree.Normal
}

// return tuple (name,type,parent_name)
func (self *RedisClient) Collect_metadata() [][3]string {
	keystr := C.CString("KEYS *")
	res := C.redisCommand_wrapper(self.conn,Format,keystr)
	ret := make([][3]string,0)
	var tuple [3]string
	num := uint(res.elements)
	for ; num >= 0 ; num -- {
		name := C.redisReply_element(res,C.size_t(num))
		// type
		var tp string
		types := C.redisCommand_wrapper(self.conn,TYPE,name).str
		switch C.GoString(types) {
		case "string" :
			tp = "STR"
		case "hash" :
			tp = "HASH"
			fields := C.redisCommand_wrapper(self.conn,HKeys,name)
			fnum := uint(fields.elements)
			for ; fnum >= 0 ; fnum -- {
				field_name := C.redisReply_element(fields,C.size_t(fnum))
				var ftuple [3]string
				ftuple[0] = C.GoString(field_name)
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