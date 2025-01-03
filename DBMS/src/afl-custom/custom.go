package main

/*
#include "afl/afl-fuzz.h"
*/
import "C"

import (
    //"fmt"
    "unsafe"
    "net/rpc"
    "os"
)

var (
    cli *rpc.Client
    testcase string
)

/**
 * Initialize this custom mutator
 *
 * @param[in] afl a pointer to the internal state object. Can be ignored for
 * now.
 * @param[in] seed A seed for this mutator - the same seed should always mutate
 * in the same way.
 * @return Pointer to the data object this custom mutator instance should use.
 *         There may be multiple instances of this mutator in one afl-fuzz run!
 *         Return NULL on error.
 */


//export afl_custom_init
func afl_custom_init(afl *C.afl_state_t,seed uint32) *int {
    cli = nil
    return nil
}

/**
 * Perform custom mutations on a given input
 *
 * (Optional for now. Required in the future)
 *
 * @param[in] data pointer returned in afl_custom_init for this fuzz case
 * @param[in] buf Pointer to input data to be mutated
 * @param[in] buf_size Size of input data
 * @param[out] out_buf the buffer we will work on. we can reuse *buf. NULL on
 * error.
 * @param[in] add_buf Buffer containing the additional test case
 * @param[in] add_buf_size Size of the additional test case
 * @param[in] max_size Maximum size of the mutated output. The mutation must not
 *     produce data larger than max_size.
 * @return Size of the mutated output.
 */


//export afl_custom_fuzz
func afl_custom_fuzz(not_use *int,buf *C.uint8_t,buf_size int,out_buf **C.uint8_t,
add_buf *uint8,add_buf_size int,max_size int) int {
    var err error
    // if first rpc
    if cli == nil {
        cli,err = rpc.DialHTTP("tcp","localhost:1234")
        if err != nil { panic("rpc error") }
    }
    var testcase_len int
    err = cli.Call("Mutator.Mutate",0,&testcase_len)
    if err != nil { panic("Mutate,rpc error") }
    err = cli.Call("Mutator.Ret_testcase",0,&testcase)
    if err != nil { panic("Ret_testcase,rpc error") }
    //test
    file,err := os.OpenFile("mutated.txt",os.O_CREATE|os.O_WRONLY|os.O_APPEND,0664)
	if err != nil { panic("open failed") }
    _,err = file.WriteString("\nmutated\n")
    if err != nil { panic("write failed") }
    _,err = file.WriteString(testcase)
	if err != nil { panic("write failed") }
    *out_buf = (*C.uint8_t)(unsafe.Pointer(C.CString(testcase)))
    return testcase_len
}

/**
   * Deinitialize the custom mutator.
   *
   * @param data pointer returned in afl_custom_init by this custom mutator
   */

   
//export afl_custom_deinit
func afl_custom_deinit(not_use *int) {}

// needed by c-shared
func main() { /* empty */ }