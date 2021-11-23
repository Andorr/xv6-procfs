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

int pid_exists[NPROC] = { 0 };

int
atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}

void build_proc_path(int value, char path[MAXPATH]) {
  char buff[36];
  char* tmp = "/proc/";
  for(int i = 0; i < 6; i++) {
    path[i] = tmp[i];
  }
  char *pid = itoa(value, buff, 10);
  for(int j = 0; j < 4; j++) {
    path[6 + j] = pid[j];
  }
}


int procfs_proc_read(struct file *f, uint64 dst, int n) {
  char path[MAXPATH];
  char* files[1] = { "/status" }; 

  int i = 0;
  int pids[NPROC];
  struct inode *p;
  proc_ptable_pids(pids);
  begin_op();
  while(pids[i] != -1) {

    if(pid_exists[pids[i]] != 0) {
      i++;
      continue;
    }

    // Create directory
    build_proc_path(pids[i], path);
    p = create(path, T_DIR, PROCFS_PID, 0);
    if(p != 0) {
      iunlockput(p);
    }

    // Create status files
    memmove((void*)&path[7], files[0], 7); // Append /status to the path
    create(path, T_DEVICE, PROCFS_FILE, pids[i]);

    pid_exists[pids[i]] = 1;
    i++;
  }
  end_op();
  
  int r = 0;
  ilock(f->ip);
  if((r = readi(f->ip, 1, dst, f->off, n)) > 0)
    f->off += r;
  iunlock(f->ip);

  return r;
}

int procfs_pid_read(struct file *f, uint64 dst, int n) {
  int r = 0;

  ilock(f->ip);
  if((r = readi(f->ip, 1, dst, f->off, n)) > 0)
    f->off += r;
  iunlock(f->ip);

  return r;
}

int procfs_file_read(struct file *f, uint64 dst, int n) {
  
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
  
  printf("name: %s\n", process.name);
  // printf("pid: %d\n", process.pid);
  if(process.parent) {
    // printf("parent pid: %s\n", process.parent->pid);
  }
  printf("state: %s\n", state);

  return 0;
}

void procfsinit() {
  devsw[PROCFS_PROC].read = procfs_proc_read;
  // devsw[PROCFS_PID].read = procfs_pid_read;
  devsw[PROCFS_FILE].read = procfs_file_read;
}
