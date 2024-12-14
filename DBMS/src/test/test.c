#include "afl/afl-fuzz.h"

u8 *__afl_area_ptr;
u32 __afl_map_size;

void start_forkserver_without_maybe_log(void) {
	u8 tmp[4] = {0, 0, 0, 0};
	u32 status = 0;
	__afl_map_size = 0x10000;
	if (__afl_map_size <= FS_OPT_MAX_MAPSIZE)
	  status |= (FS_OPT_SET_MAPSIZE(__afl_map_size) | FS_OPT_MAPSIZE);
	if (status) status |= (FS_OPT_ENABLED);
	memcpy(tmp, &status, 4);
	if (write(FORKSRV_FD + 1, tmp, 4) != 4) return;
	else printf("write to forkserver %d\n",*tmp);
}

static void map_shm_for_afl(void) {
	char *id_str = getenv(SHM_ENV_VAR);
  char *ptr;

  /* NOTE TODO BUG FIXME: if you want to supply a variable sized map then
     uncomment the following: */

  if ((ptr = getenv("AFL_MAP_SIZE")) != NULL) {
    u32 val = atoi(ptr);
    if (val > 0) __afl_map_size = val;
  }

  if (__afl_map_size > MAP_SIZE) {
    if (__afl_map_size > FS_OPT_MAX_MAPSIZE) {
      fprintf(stderr,
              "Error: AFL++ tools *require* to set AFL_MAP_SIZE to %u to "
              "be able to run this instrumented program!\n",
              __afl_map_size);
      if (id_str) {
        exit(-1);
      }

    } else {
      fprintf(stderr,
              "Warning: AFL++ tools will need to set AFL_MAP_SIZE to %u to "
              "be able to run this instrumented program!\n",
              __afl_map_size);
    }
  }

  if (id_str) {
#ifdef USEMMAP
    const char *shm_file_path = id_str;
    int shm_fd = -1;
    unsigned char *shm_base = NULL;

    /* create the shared memory segment as if it was a file */
    shm_fd = shm_open(shm_file_path, O_RDWR, 0600);
    if (shm_fd == -1) {
      fprintf(stderr, "shm_open() failed\n");
      send_forkserver_error(FS_ERROR_SHM_OPEN);
      exit(1);
    }

    /* map the shared memory segment to the address space of the process */
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

#endif

    if (__afl_area_ptr == (void *)-1) {
      exit(1);
    }

    /* Write something into the bitmap so that the parent doesn't give up */
    __afl_area_ptr[0] = 1;
  }
}


u32 next_testcase_from_afl(u8 *buf, u32 max_len) {
	s32 status, res = 0xffffff;
  
	if (read(FORKSRV_FD, &status, 4) != 4) return 0;
  
	status = read(0, buf, max_len);
  
	if (write(FORKSRV_FD + 1, &res, 4) != 4) return 0;
  // 置1, 否则报错 FSRV_RUN_NOINST
	__afl_area_ptr[0] = 1;
  size_t waitpid_status = 0xffffff;
  if (write(FORKSRV_FD + 1, &waitpid_status, 4) != 4) exit(1);
  
	return status;
}
void target(int input){
	if(input++ >100) exit(-1);
}

int main(){
    // 测试testcase接收和确认, 
	uint64_t max = 0x100000;
	u8 *buf = (u8*)malloc(max);
	map_shm_for_afl();
	start_forkserver_without_maybe_log();
	int index = 0;
  int len =0;
	while(len =next_testcase_from_afl(buf,max) > 0){
    
    if(index >50) break;
		target(*buf);
		printf("look at me, testcase %d: %s,len is %d\n",index,buf,len);
		
		index ++;
	}
	return 0;
}


