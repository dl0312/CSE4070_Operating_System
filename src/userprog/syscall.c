#include "userprog/syscall.h"
#include "userprog/process.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "devices/shutdown.h"
#include "devices/input.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/off_t.h"

#ifdef USERPROG

struct lock filesys_lock;

void syscall_handler(struct intr_frame *);

void syscall_halt(void)
{
  shutdown_power_off();
}

void syscall_exit(int status)
{
  int i;
  printf("%s: exit(%d)\n", thread_name(), status);
  thread_current()->exit_status = status;
  for (i = 3; i < 128; i++)
  {
    if (thread_current()->file_descriptor[i] != NULL)
    {
      syscall_close(i);
    }
  }
  thread_exit();
}

pid_t syscall_exec(const char *cmd_line)
{
  return process_execute(cmd_line);
}

int syscall_wait(pid_t pid)
{
  return process_wait(pid);
}

bool syscall_create(const char *file, unsigned initial_size)
{
  if (file == NULL)
    syscall_exit(-1);
  validate_user_vaddr(file);
  return filesys_create(file, initial_size);
}

bool syscall_remove(const char *file)
{
  if (file == NULL)
    syscall_exit(-1);
  validate_user_vaddr(file);
  return filesys_remove(file);
}

int syscall_filesize(int file_descriptor)
{
  if (thread_current()->file_descriptor[file_descriptor] == NULL)
    syscall_exit(-1);
  return file_length(thread_current()->file_descriptor[file_descriptor]);
}

int syscall_read(int file_descriptor, void *buffer, unsigned size)
{
  int i, result;
  validate_user_vaddr(buffer);
  lock_acquire(&filesys_lock);
  if (file_descriptor == 0)
  {
    for (i = 0; i < size; i++)
    {
      if (((char *)buffer)[i] == '\0')
      {
        break;
      }
    }
    result = i;
  }
  else if (file_descriptor > 2)
  {
    if (thread_current()->file_descriptor[file_descriptor] == NULL)
      syscall_exit(-1);
    result = file_read(thread_current()->file_descriptor[file_descriptor], buffer, size);
  }
  lock_release(&filesys_lock);
  return result;
}

int syscall_write(int file_descriptor, const void *buffer, unsigned size)
{
  int result = -1;
  validate_user_vaddr(buffer);
  lock_acquire(&filesys_lock);
  if (file_descriptor == 1)
  {
    putbuf(buffer, size);
    result = size;
  }
  else if (file_descriptor > 2)
  {
    if (thread_current()->file_descriptor[file_descriptor] == NULL)
    {
      lock_release(&filesys_lock);
      syscall_exit(-1);
    }
    if (thread_current()->file_descriptor[file_descriptor]->deny_write)
    {
      file_deny_write(thread_current()->file_descriptor[file_descriptor]);
    }
    result = file_write(thread_current()->file_descriptor[file_descriptor], buffer, size);
  }
  lock_release(&filesys_lock);
  return result;
}

void syscall_seek(int file_descriptor, unsigned position)
{
  if (thread_current()->file_descriptor[file_descriptor] == NULL)
    syscall_exit(-1);
  file_seek(thread_current()->file_descriptor[file_descriptor], position);
}

unsigned syscall_tell(int file_descriptor)
{
  if (thread_current()->file_descriptor[file_descriptor] == NULL)
    syscall_exit(-1);
  return file_tell(thread_current()->file_descriptor[file_descriptor]);
}

int syscall_open(const char *file)
{
  int i;
  int result = -1;
  struct file *file_pointer;
  if (file == NULL)
    syscall_exit(-1);
  validate_user_vaddr(file);
  lock_acquire(&filesys_lock);
  file_pointer = filesys_open(file);
  if (file_pointer == NULL)
  {
    result = -1;
  }
  else
  {
    for (i = 3; i < 128; i++)
    {
      if (thread_current()->file_descriptor[i] == NULL)
      {
        if (strcmp(thread_current()->name, file) == 0)
        {
          file_deny_write(file_pointer);
        }
        thread_current()->file_descriptor[i] = file_pointer;
        result = i;
        break;
      }
    }
  }
  lock_release(&filesys_lock);
  return result;
}

void syscall_close(int file_descriptor)
{
  struct file *file_pointer;
  if (thread_current()->file_descriptor[file_descriptor] == NULL)
    syscall_exit(-1);
  file_pointer = thread_current()->file_descriptor[file_descriptor];
  thread_current()->file_descriptor[file_descriptor] = NULL;
  return file_close(file_pointer);
}

int syscall_fibonacci(int num)
{
  return num < 3 ? 1 : syscall_fibonacci(num - 1) + syscall_fibonacci(num - 2);
}

int syscall_sum_of_four_integers(int a, int b, int c, int d)
{
  return a + b + c + d;
}

void validate_user_vaddr(const void *vaddr)
{
  if (!is_user_vaddr(vaddr))
  {
    syscall_exit(-1);
  }
}

void syscall_init(void)
{
  lock_init(&filesys_lock);
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

void syscall_handler(struct intr_frame *f UNUSED)
{
  int i;
  uint32_t syscall_num;
  syscall_num = *(uint32_t *)f->esp;
  switch (syscall_num)
  {
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
    f->eax = syscall_wait((pid_t) * (uint32_t *)(f->esp + 4));
    break;
  case SYS_CREATE:
    validate_user_vaddr(f->esp + 4);
    validate_user_vaddr(f->esp + 8);
    f->eax = syscall_create((const char *)*(uint32_t *)(f->esp + 4), (unsigned)*(uint32_t *)(f->esp + 8));
    break;
  case SYS_REMOVE:
    validate_user_vaddr(f->esp + 4);
    f->eax = syscall_remove((const char *)*(uint32_t *)(f->esp + 4));
    break;
  case SYS_OPEN:
    validate_user_vaddr(f->esp + 4);
    f->eax = syscall_open((const char *)*(uint32_t *)(f->esp + 4));
    break;
  case SYS_FILESIZE:
    validate_user_vaddr(f->esp + 4);
    f->eax = syscall_filesize((int)*(uint32_t *)(f->esp + 4));
    break;
  case SYS_READ:
    for (i = 0; i < 3; i++)
    {
      validate_user_vaddr(f->esp + 4 * i);
    }
    syscall_read((int)*(uint32_t *)(f->esp + 4), (void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12)));
    break;
  case SYS_WRITE:
    f->eax = syscall_write((int)*(uint32_t *)(f->esp + 4), (void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12)));
    break;
  case SYS_SEEK:
    validate_user_vaddr(f->esp + 4);
    validate_user_vaddr(f->esp + 8);
    syscall_seek((int)*(uint32_t *)(f->esp + 4), (unsigned)*(uint32_t *)(f->esp + 8));
    break;
  case SYS_TELL:
    validate_user_vaddr(f->esp + 4);
    f->eax = syscall_tell((int)*(uint32_t *)(f->esp + 4));
    break;
  case SYS_CLOSE:
    validate_user_vaddr(f->esp + 4);
    syscall_close((int)*(uint32_t *)(f->esp + 4));
    break;
  case SYS_FIBONACCI:
    f->eax = syscall_fibonacci((int)*(uint32_t *)(f->esp + 4));
    break;
  case SYS_SUM_OF_FOUR_INTEGERS:
    f->eax = syscall_sum_of_four_integers((int)*(uint32_t *)(f->esp + 4), (int)*(uint32_t *)(f->esp + 8), (int)*(uint32_t *)(f->esp + 12), (int)*(uint32_t *)(f->esp + 16));
    break;
  }
}
#endif