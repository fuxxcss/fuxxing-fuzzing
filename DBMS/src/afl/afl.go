package afl
/*
#include"afl.h"
*/
import "C"

import (
	"unsafe"

	"DBMS/gramfree"
)

const (
	MaxSize = 0x100000
)

func End_testcase_to_afl(status gramfree.DBMStatus){
	// cgo iota -> size_t
	C.end_testcase_to_afl(C.size_t(status))
}

func Map_shm_check(){
	C.map_shm_check()
}

func Start_forkserver_without_maybe_log(){
	C.start_forkserver_without_maybe_log()
}

func Next_testcase_from_afl(testcase *byte) uint32 {
	// cgo []byte -> u8*
	ret := C.next_testcase_from_afl((*C.uint8_t)(unsafe.Pointer(testcase)),MaxSize)
	return uint32(ret)
}

