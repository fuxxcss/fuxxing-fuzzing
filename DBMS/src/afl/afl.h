
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "afl/types.h"
#include "afl/afl-fuzz.h"
#ifndef _AFL_H_
#define _AFL_H_

// same as gramfree/client.go BMStaus
typedef enum {
	cCrash =0,
	// execute error，maybe syntax or semantic error
	cExecError,
	// connect error
	cConnectError,
	cNormal,
}cDBMStatus;

u8 *__afl_area_ptr;
#ifdef __ANDROID__
u32 __afl_map_size = MAP_SIZE;
#else
__thread u32 __afl_map_size = 65536;
#endif
// Error reporting to forkserver controller 
void send_forkserver_error(int error) {
	u32 status;
	if (!error || error > 0xffff) return;
	status = (FS_OPT_ERROR | FS_OPT_SET_ERROR(error));
	if (write(FORKSRV_FD + 1, (char *)&status, 4) != 4) return;
  }
// SHM setup. 

void map_shm_check(void) {
	char *id_str = getenv("SHM_ID");
	char *ptr;
  
	if ((ptr = getenv("AFL_MAP_SIZE")) != NULL) {
	  u32 val = atoi(ptr);
	  if (val > 0) __afl_map_size = val;
	}
  
	if (__afl_map_size > MAP_SIZE) {
	  if (__afl_map_size > FS_OPT_MAX_MAPSIZE) {
		fprintf(stderr,
				"[-] Error: AFL++ tools *require* to set AFL_MAP_SIZE to %u to "
				"be able to run this instrumented program!\n",
				__afl_map_size);
		if (id_str) {
		  send_forkserver_error(FS_ERROR_MAP_SIZE);
		  exit(-1);
		}
  
	  } else {
		fprintf(stderr,
				"[-] Warning: AFL++ tools will need to set AFL_MAP_SIZE to %u to "
				"be able to run this instrumented program!\n",
				__afl_map_size);
	  }
	}
  
	if (id_str) {
  #ifdef USEMMAP
	  const char *shm_file_path = id_str;
	  int shm_fd = -1;
	  unsigned char *shm_base = NULL;
  
	  shm_fd = shm_open(shm_file_path, O_RDWR, 0600);
	  if (shm_fd == -1) {
		fprintf(stderr, "shm_open() failed\n");
		send_forkserver_error(FS_ERROR_SHM_OPEN);
		exit(1);
	  }
  
	  shm_base =
		  mmap(0, __afl_map_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  
	  if (shm_base == MAP_FAILED) {
		close(shm_fd);
		shm_fd = -1;
  
		fprintf(stderr, "mmap() failed\n");
		send_forkserver_error(FS_ERROR_MMAP);
		exit(2);
	  }
  
	  __afl_area_ptr = shm_base;
  #else
	  u32 shm_id = atoi(id_str);
	  __afl_area_ptr = (u8 *)shmat(shm_id, 0, 0);
	  printf("[-] DBMS Server shm id=%d, shmat success\n",shm_id);
  #endif
  
	  if (__afl_area_ptr == (void *)-1) {
		send_forkserver_error(FS_ERROR_SHMAT);
		exit(1);
	  }
	  /* Write something into the bitmap so that the parent doesn't give up */
    __afl_area_ptr[0] = 1;
  
	}
}

// Fork server logic. 

void start_forkserver_without_maybe_log(void) {
	u8 tmp[4] = {0, 0, 0, 0};
	u32 status = 0;
  
	if (__afl_map_size <= FS_OPT_MAX_MAPSIZE)
	  status |= (FS_OPT_SET_MAPSIZE(__afl_map_size) | FS_OPT_MAPSIZE);
	if (status) status |= (FS_OPT_ENABLED);
	memcpy(tmp, &status, 4);
  
   /* Phone home and tell the parent that we're OK. */
	if (write(FORKSRV_FD + 1, tmp, 4) != 4) return;
}
  
u32 next_testcase_from_afl(u8 *buf, u32 max_len) {
	s32 status, res = 0xffffff;
  /* Wait for parent by reading from the pipe. Abort if read fails. */
	if (read(FORKSRV_FD, &status, 4) != 4) return 0;
  /* we have a testcase - read it */
	status = read(0, buf, max_len);
   /* report that we are starting the target */
	if (write(FORKSRV_FD + 1, &res, 4) != 4) return 0;
  
	return status;
}

void end_testcase_to_afl(size_t status) {
	size_t waitpid_status = 0xffffff;
	if (status == cCrash) {
	  waitpid_status = 0x6;  // raise.
	}
	if (write(FORKSRV_FD + 1, &waitpid_status, 4) != 4) exit(1);
}

#endif