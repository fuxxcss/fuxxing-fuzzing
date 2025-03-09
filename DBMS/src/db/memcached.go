package db
/*
#cgo LDFLAGS: -lmemcached
#include <libmemcached/memcached.h>
#include <stdio.h>
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
	Flushall = C.CString("flush_all")
	ITEMS = C.CString("stats items")
	KEYS = C.CString("stats cachedump %s 0")
)

type MemcachedClient struct {
	conn_ip string
	conn_port int
	conn *C.memcached_st
	rc C.memcached_return_t
}

func (self *MemcachedClient) Connect(ip string,port int) bool {
	// cgo string -> char*
	conn := C.memcached_create(nil)
	if conn == nil {
		return false
	}
	self.conn = conn
	rc := C.memcached_server_add(conn,C.CString(ip),C.int(port))
	if rc != C.MEMCACHED_SUCCESS {
		C.memcached_free(conn)
		self.conn = nil
		return false
	}
	self.rc = rc
	self.conn_ip = ip
	self.conn_port = port
	return true
}

func (self *MemcachedClient) Reconnect() bool {

	ret := self.Connect(self.conn_ip,self.conn_port)
	if !ret return false
	return true
}

func (self *MemcachedClient) Check_alive() bool {
	res := C.memcached_version(self.conn)
	if res != C.MEMCACHED_SUCCESS return false 
	return true
}

func (self *MemcachedClient) Clean_up() bool {
	res := C.memcached_flush(self.conn,0)
	res := C.redisCommand_one(self.conn,Flushall)
	if res != C.MEMCACHED_SUCCESS return false 
	return true
}

func (self *MemcachedClient) Execute(command string) gramfree.DBMStatus {
	//fmt.Println(command)
	command_slice := strings.Split(command," ")
	comm := command_slice[0]
	/*	todo	*/
	switch comm {
	case "GET","get":
		key := command_slice[1]
		C.memcached_get(self.conn,key,len(key),nil,0,&self.rc)
	case "SET","set":
		key := command_slice[1]
		flags := command_slice[2]
		exptime := command_slice[3]
		bytes := strings.Split(command_slice[4],"\n")[0]
		value := strings.Split(command_slice[4],"\n")[1]
		self.rc = C.memcached_set(self.conn,key,len(key),value,len(value),exptime,flags)
	/* todo */
	gets
	add
	replace
	append
	prepend
	cas
	delete
	incr
	decr
	}
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
func (self *MemcachedClient) Collect_metadata() [][3]string {
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

func (self *MemcachedClient) Restart() bool {
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