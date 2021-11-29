#include "types.h"
#include "param.h"
#include "fs.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "defs.h"
#include "stat.h"
#include "proc.h"

int
atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}

uint64 build_proc_path(int value, char path[MAXPATH]) {
  char buff[36];
  char* tmp = "/proc/";
  for(int i = 0; i < 6; i++) {
    path[i] = tmp[i];
  }
  char *pid = itoa(value, buff, 10);
  int length = MAXPATH;
  for(int j = 0; j < 4; j++) {
    path[6 + j] = pid[j];
    if(pid[j] == '\0') {
      length = 6 + j;
      break;
    }
  }
  return length;
}

void build_pid_directory(struct proc *p) {
  char path[MAXPATH];
  int path_length = build_proc_path(p->pid, path);

  struct inode *in;

  // Create directory
  in = create(path, T_DIR, 0, 0);
  if(in) {
    iunlockput(in);
  }

  // Create status file
  memmove((void*)&path[path_length], "/status", 7); // Append /status to the path
  in = create(path, T_DEVICE, PROCFS_STATUS, p->pid);
  if(in) {
    iunlockput(in);
  }

  // Create status file
  memmove((void*)&path[path_length], "/name", 7); // Append /name to the path
  in = create(path, T_DEVICE, PROCFS_NAME, p->pid);
  if(in) {
    iunlockput(in);
  }
}

uint64 remove_pid_directory(struct proc *p) {
  char path[MAXPATH];
  
  // Delete process files
  int path_length = build_proc_path(p->pid, path);
  memmove((void*)&path[path_length], "/status", 7); // Append /status to the path
  uint64 result = unlink(path);
  if(result != 0) {
    return -1;
  }
  memmove((void*)&path[path_length], "/name", 7); // Append /status to the path
  result = unlink(path);
  if(result != 0) {
    return -1;
  }
  
  // Delete directory
  build_proc_path(p->pid, path);
  result = unlink(path);
  if(result != 0) {
    return -1;
  }
  return 0;
}

int procfs_file_status(struct file *f, uint64 dst, int n) {

  struct proc process = proc_by_id(f->ip->minor);
  char *state = "unknown";
  switch(process.state) {
    case UNUSED: { state = "unused"; break; }
    case USED: { state = "used"; break; }
    case SLEEPING: { state = "sleeping"; break; }
    case RUNNABLE: { state = "runnable"; break; }
    case RUNNING: { state = "running"; break; }
    case ZOMBIE: { state = "zombie"; break; }  
  };
  
  printf("pid: %d\n", process.pid);
  printf("state: %s\n", state);

  return 0;
}

int procfs_file_name(struct file *f, uint64 dst, int n) {

  struct proc process = proc_by_id(f->ip->minor);
  
  printf("pid: %d\n", process.pid);
  printf("name: %s\n", process.name);
  return 0;
}

void procfsinit() {
  devsw[PROCFS_STATUS].read = procfs_file_status;
  devsw[PROCFS_NAME].read = procfs_file_name;
}
