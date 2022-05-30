


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

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&write_lock);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
   if(!valid_esp(f)) {
        exit(-1);
    }

    switch(*(int*)f->esp) {
        case SYS_HALT: {
            halt();
            break;
        }
       
        case SYS_EXEC:{
            WrapperExec(f);
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
            tell(f);
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


// to remove file
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
    if(!valid(file_name)){
        exit(-1);
    }
    f->eax = File_Remover(file_name);
}

 

