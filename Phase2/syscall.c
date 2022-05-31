

#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/init.h"
#include "devices/shutdown.h" // for SYS_HALT
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "lib/kernel/list.h"




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


// check the pointer to file , if it is valid , then call remove function
void WrapperRemove(struct intr_frame *f){
    char *  file_name = (char *) (*((int *) f->esp + 1));
    if(!CheckValid(file_name)){
        exit(-1);
    }
    f->eax = File_Remover(file_name);
}

// to remove file
int File_Remover(char * file_name){
    int res = -1;
    lock_acquire(&write_lock);
    res = filesys_remove(file_name);
    lock_release(&write_lock);
    return res;
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
void WrapperRead(struct intr_frame *f){
    int fd = (int ) (*((int *) f->esp + 1));
    char * buffer = (char * ) (*((int *) f->esp + 2));
    if(fd==1 || !CheckValid(buffer)){
        // fd is 1 means (stdout ) so it is not allowed
        exit(-1);
    }
    unsigned size = *((unsigned *) f->esp + 3);
    f->eax = read(fd,buffer,size);
}
// take fd for target file and buffer to save the data in and size of to be read and reaturn the actual size to be read
int read(int fd,char* buffer,unsigned size){
    int res = size;
    if(fd ==0){
        // stdin .. so get the data with input_getc
        while (size--)
        {
            lock_acquire(&write_lock);
            char c = input_getc();
            lock_release(&write_lock);
            buffer+=c;
        }
        return res;
    }
    if(fd ==1){
        // negative area
    }

    struct user_file * user_file =getCurrFile(fd);

    if(user_file==NULL){
        return -1;
    }else{
        struct file * file = user_file->file;
        lock_acquire(&write_lock);
        size = file_read(file,buffer,size);
        lock_release(&write_lock);
        return size;
    }
}
// check paramater (fd) and if it is valid call close fuction otherwise exit
void WrapperClose(struct intr_frame *f){
    int fd = (int) (*((int *) f->esp + 1));
    if(fd<2){
        // if the target is stdin or stdout
        exit(-1);
    }
    f->eax = close(fd);
}
// take the fd for target file and close it if it exist to current process otherwise return -1
int close(int fd){
    struct user_file  *file = getCurrFile(fd);
    if(file!=NULL){
        lock_acquire(&write_lock);
        file_close(file->file);
        lock_release(&write_lock);
        list_remove(&file->elem);
        return 1;
    }
    return -1;
}
// check paramater (pointer to file name ) and if it is valid call open function otherwise exit

void WrapperOpen(struct intr_frame *f){
    char * file_name = (char *) (*((int *) f->esp + 1));
    if(!CheckValid(file_name)){
        exit(-1);
    }
    f->eax = open(file_name);
}

// open file with name ( file name ) and return it's fd
int open(char * file_name){
    static unsigned long curent_fd = 2;
    lock_acquire(&write_lock);
    struct file * opened_file  = filesys_open(file_name);
    lock_release(&write_lock);

    if(opened_file==NULL){
        return -1;
    }else{
        // wrapper contain the file and fd
        struct user_file* user_file = (struct user_file*) malloc(sizeof(struct user_file));
        int file_fd = curent_fd;
        user_file->fd = curent_fd;
        user_file->file = opened_file;
        lock_acquire(&write_lock);
        curent_fd++;
        lock_release(&write_lock);
        struct list_elem *elem = &user_file->elem;
        list_push_back(&thread_current()->files, elem);
        return file_fd;
    }
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

///--------------------write file------------

// check paramater (fd and poitner to buffer )and if they are valid call write function otherwise exit
void WrapperWrite(struct intr_frame *f){
    int fd = *((int *) f->esp + 1);
    char *buffer = (char *) (*((int *) f->esp + 2));
    if(!CheckValid(buffer) || fd ==0){
        // fd is 0 when target is stdin so it is not allowed
        exit(-1);
    }
    unsigned size = (unsigned)(*((int*) f->esp + 3));
    f->eax = WriteToFile(fd, buffer, size);
}

int WriteToFile(int fd,char * buffer,unsigned size){
    if(fd ==0){
        // negative area
    }
    else if (fd == 1) {
        lock_acquire(&write_lock);
        putbuf(buffer, size);
        lock_release(&write_lock);
        return size;
    }

    struct user_file *file = getCurrFile(fd);
    if(file ==NULL){
        return -1;
    }else{
        int res = 0;
        lock_acquire(&write_lock);
        res = file_write(file->file,buffer,size);
        lock_release(&write_lock);
        return res;
    }
}
//--------------------------------------------------------------------
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
        tid_t tid = *((int*)f->esp + 1);
        f->eax = waiting(tid);
    }
}

/////////call from process.c
void WrapperExec(struct intr_frame *f){
    char  *fileName = (char *) (*((int *) f->esp + 1));
    f->eax = process_execute(fileName);
}

// exit process
void exit(int status){
    char * name = thread_current()->name;
    char * save_ptr;
    char * executable = strtok_r (name, " ", &save_ptr);
    thread_current()->exit = status;
    printf("%s: exit(%d)\n",executable,status);
    thread_exit();
}

void WrapperExit(struct intr_frame *f){
    int stat = *((int*)f->esp + 1);
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
    return CheckValid((int*)f->esp) || ((*(int*)f->esp) < 0) || (*(int*)f->esp) > 12;
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
