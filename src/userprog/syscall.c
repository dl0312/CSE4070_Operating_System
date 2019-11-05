#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

 void syscall_handler (struct intr_frame *);

 void syscall_halt (void) {
  shutdown_power_off();
}

 void syscall_exit (int status) {
    printf("%s: exit(%d)\n", thread_name(), status);
    thread_current() -> exit_status = status;
    thread_exit ();
}

 pid_t syscall_exec (const char *cmd_line) {
  return process_execute(cmd_line);
}

 int syscall_wait (pid_t pid) {
  return process_wait(pid);
}

 int syscall_read (int fd, void* buffer, unsigned size) {
  int i;
  if (fd == 0) {
    for (i=0; i < size; i++) {
      if (((char *)buffer)[i] == '\0') {
        break;
      }
    }
  }
  return i;
}

 int syscall_write (int fd, const void *buffer, unsigned size) {
  if (fd == 1) {
    putbuf(buffer, size);
    return size;
  }
  return -1;
}


int syscall_fibonacci (int num)
{
	return n < 3 ? 1 : syscall_fibonacci(n - 1) + syscall_fibonacci( n - 2);
}

int syscall_sum_of_four_integers (int a, int b, int c, int d)
{
  return a+b+c+d;
}


void validate_user_vaddr(const void *vaddr){
  if(!is_user_vaddr(vaddr)){
    syscall_exit(-1);
  }
}


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

 void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int i;
  uint32_t syscall_num;
  syscall_num = * (uint32_t*) f->esp;
  switch (syscall_num) {
    case SYS_HALT:
      syscall_halt();
      break;
    case SYS_EXIT:
      validate_user_vaddr(f->esp + 4);
      syscall_exit(*(uint32_t *)(f->esp + 4));
      break;
    case SYS_EXEC:
      validate_user_vaddr(f->esp + 4);
      f->eax = syscall_exec((const char *)*(uint32_t *)(f->esp + 4));
      break; 
    case SYS_WAIT:
      validate_user_vaddr(f->esp + 4);
      f->eax = syscall_wait((pid_t)*(uint32_t *)(f->esp + 4));
      break; 
    case SYS_CREATE:
      break;
    case SYS_REMOVE:
      break;
    case SYS_OPEN:
      break;
    case SYS_FILESIZE:
      break;
    case SYS_READ:
      for(i=0;i<3;i++){
        validate_user_vaddr(f->esp + 20 + 4*i);
      }
      syscall_read((int)*(uint32_t *)(f->esp+20), (void *)*(uint32_t *)(f->esp + 24), (unsigned)*((uint32_t *)(f->esp + 28)));
      break;
    case SYS_WRITE:
      f->eax = syscall_write((int)*(uint32_t *)(f->esp+4), (void *)*(uint32_t *)(f->esp+8), (unsigned)*((uint32_t *)(f->esp+12)));
      break;
    case SYS_SEEK:
      break;
    case SYS_TELL:
      break;
    case SYS_CLOSE:
      break;
    case SYS_FIBONACCI:
			f->eax = syscall_fibonacci(arg[1]);
      break;
    case SYS_SUM_OF_FOUR_INTEGERS:
			f -> eax = syscall_sum_of_four_integers(arg[1],arg[2],arg[3],arg[4]);
      break;
  }
}
