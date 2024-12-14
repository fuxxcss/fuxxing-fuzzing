package db

/*
#cgo LDFLAGS: -L/usr/local/hiredis-1.2.0/ -lredis
#include "/usr/local/hiredis-1.2.0/hiredis.h"
*/
/*
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
	res := C.redisCommand(self.conn,"info")
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
func (self *PostgresqlClient) Select_metadata() [][3]string {
	cstr := C.CString(SelectMetadata)
	res := C.PQexec(self.conn,cstr)
	len := C.PQntuples(res) - 1
	ret := make([][3]string,0)
	var tuple [3]string
	var name string
	for ; len >= 0 ; len-- {
		// name
		name = C.GoString(C.PQgetvalue(res,len,0))
		tuple[0] = name
		// type
		var tp string
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

func (self *PostgresqlClient) Restart() bool {
	//os.Setenv() todo
	program := "/usr/local/redis/src/redis-server"
	arg1 := "&"
	cmd := exec.Command(program,arg1)
	err := cmd.Run()
	if err != nil {
		return false
	}
	return true
}
*/