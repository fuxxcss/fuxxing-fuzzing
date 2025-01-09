package main

import (
	"os"
	"fmt"
	"bytes"
	"strings"
	"net/http"
	"net/rpc"

	"DBMS/gramfree"
	"DBMS/afl"
	"DBMS/db"
)

var (
	dbms string
	client gramfree.Client // interface
	mutator *gramfree.Mutator
)

func testcase_loop(testcase string) gramfree.DBMStatus {
	var status gramfree.DBMStatus
	// execute new testcase
	graph := new(gramfree.Graph)
	metamap := new(gramfree.Metamap)
	graph.Map = metamap
	line_slice := strings.SplitAfter(testcase,"\n")
	for _,line := range line_slice {
		// sequential execute one line, trim '\n'
		line_len := len(line)
		if line_len == 0 { continue }
		if line[line_len-1] == '\n' {
			status = client.Execute(line[:line_len-1])
		}else{
			status = client.Execute(line)
		}
		// deal with Crash
		if status == gramfree.Crash {
			fmt.Println("[*] Crash found ")
			file,err := os.OpenFile("crash.txt",os.O_CREATE|os.O_WRONLY|os.O_TRUNC,0664)
			if err != nil { panic("open failed") }
			_,err = file.WriteString(testcase)
			if err != nil { panic("write failed") } 
			ret := client.Restart()
			if !ret { panic("restart failed") }
			ret = client.Reconnect()
			if !ret { panic("reconnect failed") }
			return gramfree.Crash
		}
		// deal with Execute error
		if status == gramfree.ExecError {
			graph.Build_graph(line)
			continue
		}
		// select metadata to metamap
		metamap.Update()
		for _,tuple := range client.Collect_metadata() {
			// tuple (name,type,parent_name)
			graph.Build_metamap(tuple[0],tuple[1],tuple[2])
		}
		graph.Build_graph(line)
	}
	// add this testcase to Corpus
	last := mutator.Corpus_last()
	testcase_len := mutator.Calculate_line(testcase,dbms)
	if (last == "" || strings.Compare(last,testcase) != 0) && testcase_len != 0 {
		mutator.Corpus_add(testcase,testcase_len)
	}else { return status }
	// add graph
	mutator.Corpus_graph = append(mutator.Corpus_graph,graph)

	return status
}

func main() {
	var ret bool
	/*	get envs	*/
	dbms = os.Getenv("DBMS")
	gramfree.AFL_SHM_ID = os.Getenv("SHM_ID")
	gramfree.AFL_MAP_SIZE = os.Getenv("AFL_MAP_SIZE")
	switch dbms {
	case gramfree.Redis :
		client = new(db.RedisClient)
		ret = client.Connect(gramfree.Redis_IP,gramfree.Redis_PORT)
	case gramfree.KeyDB :
		client = new(db.RedisClient)
		ret = client.Connect(gramfree.Redis_IP,gramfree.KeyDB_PORT)
	default :
		panic("please export DBMS")
	}
	// connect DBMS server
	if !ret { panic("connect failed") }
	// shm DBMS server check
	afl.Map_shm_check()
	// forkserver init not need afl_maybe_log , answer afl-fuzz
	afl.Start_forkserver_without_maybe_log()
	// RPC Mutator
	mutator = new(gramfree.Mutator)
	rpc.Register(mutator)
	rpc.HandleHTTP()
	go func() {
		fmt.Println("[-] RPC Listening on 1234...")
		err := http.ListenAndServe(":1234",nil)
		if err != nil { panic("rpc error") }
	}()
	// range testcas_loop()
	mutator.Init()
	buffer := make([]byte,afl.MaxSize) // avoid: out of memory
	
	var testcase string
	/*
	file,err := os.OpenFile("testcase.txt",os.O_CREATE|os.O_WRONLY|os.O_TRUNC,0664)
	if err != nil { panic("open failed") }
	*/
	for {
		// clean up database
		ret = client.Clean_up()
		if !ret { 
			//fmt.Println(tmp)
			panic("clean up failed") 
		}
		// get one testcase
		length := afl.Next_testcase_from_afl(&buffer[0])
		if length <= 0 { panic("next testcase failed") }
		trim := bytes.TrimRight(buffer,"\x00")
		testcase = string(trim)
		/* test
		_,err = file.WriteString("\ntestcase\n")
		_,err = file.WriteString(testcase)
		if err != nil { panic("write failed") }
		*/
		//tmp = testcase
		status := testcase_loop(testcase)
		// answer to afl-fuzz ，let afl-fuzz decide interesting depends on shmmap coverage
		afl.End_testcase_to_afl(status)
	}
}

