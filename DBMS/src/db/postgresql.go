package db

/*
#cgo LDFLAGS: -L/usr/local/pgsql/lib -lpq
#include "/usr/include/postgresql/libpq-fe.h"
*/
import "C"

import (
	//"os"
	"os/exec"
	"fmt"

	"DBMS/gramfree"
)

const SelectMetadata = `
	select name,type,parent_name from 
	(
	select table_name as name,'TABLE' as type,null as parent_name from information_schema.tables where table_schema='public'
	union
	select column_name as name,data_type as type,table_name as parent_name from information_schema.columns where table_schema='public'
	union
	select trigger_name as name,'TRIGGER' as type,event_object_table as parent_name from information_schema.triggers where trigger_schema='public'
	union
	select table_name as name,'VIEW' as type,null as parent_name from information_schema.views where table_schema='public'
	union
	select indexname as name,'INDEX' as type,tablename as parent_name from pg_indexes where schemaname='public'
	);
`
	/*
	union
	select rulename as name,'RULE' as type,null as parent_name from pg_rules
	union
	select policyname as name,'POLICY' as type,null as parent_name from pg_policies
	*/

type PostgresqlClient struct {
	connstr string
	status gramfree.DBMStatus
	conn *C.PGconn
}

func (self *PostgresqlClient) Connect(user string,pwd string,dbn string) bool {
	connstr := "user=%s password=%s host=localhost port=5432 dbname=%s"
	connstr = fmt.Sprintf(connstr,user,pwd,dbn)
	self.connstr = connstr
	// cgo string -> char*
	conn := C.PQconnectdb(C.CString(connstr))
	if C.PQstatus(conn) == C.CONNECTION_BAD {
		self.status = gramfree.ConnectError
		return false
	}
	self.conn = conn
	return true
}

func (self *PostgresqlClient) Reconnect() bool {
	conn := C.PQconnectdb(C.CString(self.connstr))
	if C.PQstatus(conn) == C.CONNECTION_BAD {
		self.status = gramfree.ConnectError
		return false
	}
	self.conn = conn
	return true
}
 
func (self *PostgresqlClient) Check_alive() bool {
	res := C.PQping(C.CString(self.connstr))
	if res == C.PQPING_OK { return true }
	return false
}

func (self *PostgresqlClient) Clean_up() bool {
	cleanup_str := "DROP SCHEMA public CASCADE; CREATE SCHEMA public;"
	// cgo string -> char*
	cstr := C.CString(cleanup_str)
	res := C.PQexec(self.conn,cstr)
	status := C.PQresultStatus(res)
	if status != C.PGRES_COMMAND_OK && status != C.PGRES_TUPLES_OK { return false }
	return true
}

func (self *PostgresqlClient) Execute(sql string) gramfree.DBMStatus {
	cstr := C.CString(sql)
	res := C.PQexec(self.conn,cstr)
	conn_status := C.PQstatus(self.conn)
	// Crash
  	if conn_status != C.CONNECTION_OK {
    	C.PQclear(res)
    	self.status = gramfree.Crash
		return gramfree.Crash
  	}
	// ExecError
	exe_status := C.PQresultStatus(res)
  	if exe_status != C.PGRES_COMMAND_OK && exe_status != C.PGRES_TUPLES_OK {
    	C.PQclear(res);
    	self.status = gramfree.ExecError
		return gramfree.ExecError
  	}
	C.PQclear(res);
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
		/*
		case "RULE" :
			tp = "RULE"
		case "POLICY" :
			tp = "POLICY"
		*/
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
	//os.Setenv()
	program := "/usr/local/pgsql-inst/bin/postgres"
	arg1 := "-D" 
	arg2 := "/usr/local/pgsql-inst/data"
	arg3 := "&"
	cmd := exec.Command(program,arg1,arg2,arg3)
	err := cmd.Run()
	if err != nil {
		return false
	}
	return true
}

