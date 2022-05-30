


#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "stdbool.h"
#include "threads/interrupt.h"
#include "threads/thread.h"

void syscall_init (void);
static void syscall_handler (struct intr_frame *f);
void get_size(struct intr_frame *frame);
void change_pos(struct intr_frame *frame);
int read(int fd , char* buffer , unsigned size);
void position_returned(struct intr_frame *f);
bool Valid(struct intr_frame *f);
int open(char *file_name);
int close(int fd);


int createFile(int size ,char * fileName);
int WriteToFile(unsigned initSize ,int fd ,char * buff);
tid_t waiting(tid_t tid);
void halt();
struct user_file * getCurrFile( int  fd);
void exit(int stat);
bool CheckValid(void * threadName);

//wrapper part
void WrapperWrite(struct intr_frame *f);
void WrapperRead(struct intr_frame *frame);

void WrapperOpen(struct intr_frame *f);
void WrapperClose(struct intr_frame *f);

void WrapperCreate(struct intr_frame *f);
void WrapperExit(struct intr_frame *f);

void WrapperWait(struct intr_frame *f);
void WrapperExec(struct intr_frame *f);

void WrapperRemove(struct intr_frame *f);


struct lock write_lock;
int File_Remover(char * file_name);




#endif /* userprog/syscall.h */

  

