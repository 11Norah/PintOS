



#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
struct lock write_lock;
int File_Remover(char * file_name);

//wrapper part
void WrapperWrite(struct intr_frame *f);
void WrapperRead(struct intr_frame *f);

void WrapperOpen(struct intr_frame *f);
void WrapperClose(struct intr_frame *f);

void WrapperCreate(struct intr_frame *f);
void WrapperExit(struct intr_frame *f);

void WrapperWait(struct intr_frame *f);
void WrapperExec(struct intr_frame *f);

void WrapperRemove(struct intr_frame *f);
#endif /* userprog/syscall.h */

  

