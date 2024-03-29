#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "lib/user/syscall.h"

void syscall_init(void);

void syscall_halt(void);
void syscall_exit(int);
pid_t syscall_exec(const char *);
int syscall_wait(pid_t);

int syscall_fibonacci(int);
int syscall_sum_of_four_integers(int, int, int, int);

bool syscall_create(const char *, unsigned);
bool syscall_remove(const char *);
int syscall_open(const char *);
int syscall_filesize(int);
int syscall_read(int, void *, unsigned);
int syscall_write(int, const void *, unsigned);

void syscall_seek(int, unsigned);
unsigned syscall_tell(int);
void syscall_close(int);
void validate_user_vaddr(const void *vaddr);

#endif /* userprog/syscall.h */
