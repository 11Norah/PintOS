

#include "threads/init.h"
#include "devices/shutdown.h" // for SYS_HALT
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "lib/kernel/list.h"

#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&write_lock);
}

static void
syscall_handler (struct intr_frame *f ) 
{
   if(!Valid(f)) {
        exit(-1);
    }

    switch(*(int*)f->esp) {
        case SYS_HALT: {
            halt();
            break;
        }
        case SYS_EXIT: {
            WrapperExit(f);

            break;
        }
        case SYS_WAIT:{
            WrapperWait(f);
            break;
        }
       case SYS_CREATE:{
            WrapperCreate(f);
            break;
        }
        case SYS_EXEC:{
            WrapperExec(f);
            break;
        }
         
         case SYS_WRITE: {
            WrapperWrite(f);
            break;
        }
        case SYS_OPEN :{
            WrapperOpen(f);
            break;
        }case SYS_CLOSE:{
            WrapperClose(f);
            break;
        }
        
        case SYS_FILESIZE:{

            get_size(f);
            break;
        }
        case SYS_READ :{
            WrapperRead(f);
            break;
        }
        case SYS_SEEK:{
            change_pos(f);
            break;
        }case SYS_TELL:{
            position_returned(f);
            break;
        }case SYS_REMOVE:{
            WrapperRemove(f);
            break;
        }
        default:{
            // NEGATIVE 
        }
    }

}


//--------------------------- to remove file
int File_Remover(char * file_name){
    int result = -1;
    lock_acquire(&write_lock);
    result = filesys_remove(file_name);
    lock_release(&write_lock);
    return result;
}


//if the the file pointer is valid then it calls remove function
void WrapperRemove(struct intr_frame *f){
    char *  file_name = (char *) (*((int *) f->esp + 1));
    if(!CheckValid(file_name)){
        exit(-1);
    }
    f->eax = File_Remover(file_name);
}

//------------------------------------------------------------------ 
// take fd and return postion to next bite to be read or written
void position_returned(struct intr_frame *f){
    int fd = (int ) (*((int *) f->esp + 1));
    struct user_file * file = getCurrFile(fd);
    if(file ==  NULL){
        f->eax =- 1;
    }else{
        lock_acquire(&write_lock);
        f->eax = file_tell(file->file);
        lock_release(&write_lock);
    }
}
//returns the size of the file
void get_size(struct intr_frame *frame){
    int fd = (int ) (*((int *) frame->esp + 1));
    struct user_file * file = getCurrFile(fd);
    if(file ==  NULL){
        frame->eax =- 1;
    }else{
        lock_acquire(&write_lock);
        frame->eax = file_length(file->file);
        lock_release(&write_lock);
    }
}

//func to change position to be read/written in this file to this position
void change_pos(struct intr_frame *frame){
    int fd = (int ) (*((int *) frame->esp + 1));
    unsigned position = (unsigned) (*((int *) frame ->esp + 2));
    struct user_file *file = getCurrFile(fd);
    if(file ==  NULL){
        frame->eax =- 1;
    }else{
        lock_acquire(&write_lock);
        file_seek(file->file,position);
        frame->eax = position;
        lock_release(&write_lock);
    }
    

}
///--------------------------Reading file----------------------
int read(int fd , char* buffer , unsigned size){
    int res = size;
    if(fd ==0){
        for (int i=size ; i>0 ; i--)
        {
            lock_acquire(&write_lock);
            char c = input_getc();
            lock_release(&write_lock);
            buffer+=c;
        }
        return res;
    }

    struct user_file *user_file =getCurrFile(fd);

    if(user_file==NULL){
        return -1;
    }else{
        struct file *file = user_file->file;
        lock_acquire(&write_lock);
        size = file_read(file,buffer,size);
        lock_release(&write_lock);
        return size;
    }
}
//if fd && buffer are valid , call read function otherwise exit
void WrapperRead(struct intr_frame *frame){
    int fd = (int ) (*((int *) frame->esp + 1));
    char * buffer = (char * ) (*((int *) frame->esp + 2));
    if( fd==1 || !CheckValid(buffer) ) {
        exit(-1);
    }
    unsigned size = *((unsigned *) frame->esp + 3);
    frame->eax = read( fd , buffer , size );
}




//---------------------------closing file----------------
int close(int fd){ 
    struct user_file *file = getCurrFile(fd);
    if(file!=NULL){
        lock_acquire(&write_lock);
        file_close(file->file);
        lock_release(&write_lock);
        list_remove(&file->elem);
        return 1;
    }
    return -1;
}

//if the pointer of the file is valid ,it will be closed
void WrapperClose(struct intr_frame *f){
    int fd = (int) (*((int *) f->esp + 1));
    if(fd<2){
        exit(-1);
    }
    f->eax = close(fd);
}




//----------------------opening file---------

int open(char *file_name){
    static unsigned long curr_fd = 2;
    lock_acquire(&write_lock);
    struct file * opened_file  = filesys_open(file_name);
    lock_release(&write_lock);

    if(opened_file==NULL){
        return -1;
    }else{
        // wrapper contain the file and fd
        struct user_file* user_file = (struct user_file*) malloc(sizeof(struct user_file));
        int F_fd = curr_fd;
        user_file->fd = curr_fd;
        user_file->file = opened_file;
        lock_acquire(&write_lock);
        curr_fd++;
        lock_release(&write_lock);
        struct list_elem *elem = &user_file->elem;
        list_push_back(&thread_current()->files, elem);
        return F_fd;
    }
}

void WrapperOpen(struct intr_frame *f){
    char f_name = (char *) (((int *) f->esp + 1));
    if(!CheckValid(f_name)){
        exit(-1);
    }
    f->eax = open(f_name);
}
//-----------------------------------------------

int createFile(int size ,char* fileName){
    int i = 0;
    lock_acquire(&write_lock);
    i = filesys_create(fileName,size);
    lock_release(&write_lock);
    return i;
}

void WrapperCreate(struct intr_frame *f){
    char* fileName = (char * )*((int  *)f->esp + 1 );
    if(!CheckValid(fileName)) {
            exit(-1);
    }
    else{
        int size = (unsigned) *((int *) f->esp + 2 );
        f->eax = createFile(size ,fileName);
    }
}

int WriteToFile(unsigned initSize ,int fd ,char * buff){
    if(fd ==0){
        // negative area
    }
    else if (fd == 1) {
        lock_acquire(&write_lock);
        putbuf(buff, initSize);
        lock_release(&write_lock);
        return initSize;
    }
    struct user_file *f = getCurrFile(fd);
    if(f == NULL){
        return -1;
    }
    else{
        int i = 0;
        lock_acquire(&write_lock);
        i = file_write(f->file,buff,initSize);
        lock_release(&write_lock);
        return i;
    }
}

void WrapperWrite(struct intr_frame *f){
    int fd = *((int *) f->esp + 1);
    char buff = (char *) (((int *) f->esp + 2));
    if(!CheckValid(buff) || fd ==0){
        exit(-1);
    }
    else{
        unsigned initSize = (unsigned)(((int) f->esp + 3));
        f->eax = WriteToFile(initSize ,fd ,buff);
    }
}
/////////call from process.c
tid_t waiting(tid_t tid){
    return process_wait(tid);
}
////////////////////////////
void WrapperWait(struct intr_frame *f){
    if(!CheckValid((int*)f->esp + 1)){
        exit(-1);
    }
    else{
        tid_t tid = ((int)f->esp + 1);
        f->eax = waiting(tid);
    }
}

/////////call from process.c
void WrapperExec(struct intr_frame *f){
    char fileName = (char *) (((int *) f->esp + 1));
    f->eax = process_execute(fileName);
}

void exit(int stat){
    char * threadName = thread_current()->name;
    char * pointer;
    char * exec = strtok_r (threadName, " ", &pointer);
    thread_current()->exit = stat;
    printf("%s: exit(%d)\n",exec,stat);
    thread_exit();
}

void WrapperExit(struct intr_frame *f){
    int stat = ((int)f->esp + 1);
    if(!is_user_vaddr(stat)) {
        f->eax = -1;
        exit(-1);
    }
    f->eax = stat;
    exit(stat);
}

bool CheckValid ( void * threadName){
    return threadName!=NULL && is_user_vaddr(threadName) && pagedir_get_page(thread_current()->pagedir, threadName) != NULL;
}
bool Valid(struct intr_frame *f ){
    return CheckValid((int*)f->esp) || (((int)f->esp) < 0) || ((int)f->esp) > 12;
}
void halt(){
    printf("(halt) begin\n");
    shutdown_power_off();
}

struct user_file *  getCurrFile(int  fd){
    struct list* lst  = &(thread_current()->files);
    for(struct list_elem* ele = list_begin(lst); ele!=list_end(lst) ; ele =list_next(ele)){
        struct user_file*  f = list_entry(ele, struct user_file , elem);
        if((f->fd) ==fd){
            return f;
        }
    }
    return NULL;
}
