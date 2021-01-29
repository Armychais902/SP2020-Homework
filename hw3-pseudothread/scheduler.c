#include "threadutils.h"


/*
0. You should state the signal you received by:
   printf('TSTP signal!\n') or printf('ALRM signal!\n')
1. If you receive SIGALRM, you should reset alarm() by timeslice argument passed in ./main
2. You should longjmp(SCHEDULER,1) once you're done
*/
void sighandler(int signo)
{
      //TSTP or ALRM would be blocked since sigaction would block itself
      if(signo==SIGTSTP){
         printf("TSTP signal!\n");
      }
      else if(signo==SIGALRM){
         printf("ALRM signal!\n");
         alarm(timeslice);
      }
      //restore mask
      sigprocmask(SIG_SETMASK,&base_mask,NULL);
      longjmp(SCHEDULER,1);
}

/*
0. You are stronly adviced to make setjmp(SCHEDULER) = 1 for ThreadYield() case
                                   setjmp(SCHEDULER) = 2 for ThreadExit() case
1. Please point the Current TCB_ptr to correct TCB_NODE
2. Please maintain the circular linked-list here
*/
void scheduler()
{
   int ret=setjmp(SCHEDULER);
   if(ret==1){
      Current=Current->Next;
      longjmp(Current->Environment,1);
      //setjmp to yield
      //restore mask
   }
   else if(ret==2){
      //delete TCB
      if(Current->Next!=Current){
          Work=Current->Next;
          Current->Prev->Next=Current->Next;
          Current->Next->Prev=Current->Prev;
          free(Current);
          Current=Work;
          longjmp(Current->Environment,1);
      }
      else{
          free(Current);
          longjmp(MAIN,1);
      }
   }
   else if(ret==0){
      //first time, jump to init
      Current=Head;
      longjmp(Current->Environment,1);
   }
}
