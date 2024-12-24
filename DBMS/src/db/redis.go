package db

/*
#cgo LDFLAGS: -L/usr/local/hiredis-1.2.0/ -lredis
#include "/usr/local/hiredis-1.2.0/hiredis.h"
*/

import "C"

import (
	//"os"
	"os/exec"
	"fmt"
	"DBMS/gramfree"
)

type RedisClient struct {
	conn_ip string
	conn_port int
	status gramfree.DBMStatus
	conn *C.redisContext
}

func (self *RedisClient) Connect(ip string,port int) bool {
	// cgo string -> char*
	conn := C.redisConnect(C.CString(ip),port)
	if conn == nil || conn.err {
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
	res := C.redisCommand(self.conn,"ping")
	if res == nil || res.err { return false }
	return true
}

func (self *RedisClient) Clean_up() bool {
	res := C.redisCommand(self.conn,"flushall")
	if res == nil || res.err { return false }
	return true
}

func (self *RedisClient) Execute(command string) gramfree.DBMStatus {
	cstr := C.CString(command)
	res := C.redisCommand(self.conn,cstr)
	if res.type == REDIS_REPLY_ERROR {
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
	reply := C.redisCommand(self.conn,keystr)
	ret := make([][3]string,0)
	var tuple [3]string
	var name string
	for num =0 ; num < reply.elements ; num ++ {
		name = reply.element[num].str
		// type
		var tp string
		tpstr := C.CString("TYPE %s",name)
		types = C.redisCommand(self.conn,tpstr).str
		switch types {
		case "string"
			tp = "STR"
		case "hash" :
			tp = "HASH"
			hashstr := C.CString("HKEYS %s",name)
			...
		case "list" :
			tp = "LIST"
		case "set" :
			tp = "SET"
		case "ZSET" :
			tp = "zset"
		case "stream" :
			tp = "STREAM"
			...
		case "geo" :
			tp = "GEO"
		default :
			tp = ""
		}
		tuple[1] = tp
		// parent_name
		pname := C.GoString(C.PQgetvalue(res,len,2))
		tuple[2] = pname
		//fmt.Println("name:",name,",type:",tp,",pname:",pname)
		ret = append(ret,tuple)
	}
	for ; len >= 0 ; len-- {
		// name
		name = C.GoString(C.PQgetvalue(res,len,0))
		tuple[0] = name
		
		types := C.GoString(C.PQgetvalue(res,len,1))
		switch types {
		case "integer","smallint","serial","smallserial","bigint","bigserial" :
			tp = "RDB_INT"
		case "character","character varying","text","bit","bit varying" :
			tp = "RDB_STR"
		case "TABLE" :
			tp = "RDB_TABLE"
		case "TRIGGER" :
			tp = "RDB_TRIGGER"
		case "VIEW" :
			tp = "RDB_VIEW"
		case "INDEX" :
			tp = "RDB_INDEX"
		default :
			tp = ""
		}
		tuple[1] = tp
		// parent_name
		pname := C.GoString(C.PQgetvalue(res,len,2))
		tuple[2] = pname
		//fmt.Println("name:",name,",type:",tp,",pname:",pname)
		ret = append(ret,tuple)
	}
	return ret
}

func (self *RedisClient) Restart() bool {
	os.Setenv("AFL_MAP_SIZE",gramfree.AFL_MAP_SIZE)
	os.Setenv("__AFL_SHM_ID",gramfree.__AFL_SHM_ID)
	program := "/usr/local/redis/src/redis-server"
	arg1 := "&"
	cmd := exec.Command(program,arg1)
	err := cmd.Run()
	if err != nil {
		return false
	}
	return true
}