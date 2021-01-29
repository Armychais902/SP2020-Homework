#include "threadutils.h"

void BinarySearch(int thread_id, int init, int maxiter)
{
    ThreadInit(thread_id, init, maxiter);
    /*
    Some initilization if needed.
    */
    Current->x=0;
    Current->y=100;
    int m=0;
    for (Current->i = 0; Current->i < Current->N; ++Current->i)
    {
        sleep(1);
        /*
        Do the computation, then output result.
        Call ThreadExit() if the work is done.
        */
        m=(Current->x+Current->y)/2;
        printf("BinarySearch: %d\n",m);
        if(m<Current->z){
            Current->x=m+1;
        }
        else if(m>Current->z){
            Current->y=m-1;
        }
        else    //find init
            break;
        ThreadYield();
    }
    ThreadExit(); //maxiter or stop cond.
    //between range(0,100) to find init of this function
}

int cmpMax(const void *a,const void *b){
    return ( *(int*)b-*(int*)a );
}
int cmpmin(const void *a,const void *b){
    return ( *(int*)a-*(int*)b );
}

void BlackholeNumber(int thread_id, int init, int maxiter)
{
    ThreadInit(thread_id, init, maxiter);
    /*
    Some initilization if needed.
    */
    Current->x=Current->z;  //assign init to x, x store the result of every iteration
    for (Current->i = 0; Current->i < Current->N; ++Current->i)
    {
        sleep(1);
        /*
        Do the computation, then output result.
        Call ThreadExit() if the work is done.
        */
        int digit[3]={0};
        digit[0]=Current->x/100;
        digit[1]=(Current->x/10)%10;
        digit[2]=Current->x%10;

        qsort(digit,3,sizeof(int),cmpMax);
        int max=100*digit[0]+10*digit[1]+digit[2];
        qsort(digit,3,sizeof(int),cmpmin);
        int min=100*digit[0]+10*digit[1]+digit[2];

        Current->x=max-min;
        printf("BlackholeNumber: %d\n",Current->x);
        if(Current->x==495)
            break;
        ThreadYield();
    }
    ThreadExit();
    //The function stops when the output is 495
}

void FibonacciSequence(int thread_id, int init, int maxiter)
{
    ThreadInit(thread_id, init, maxiter);
    /*
    Some initilization if needed.
    */
    Current->x=0;
    Current->y=1;
    int tmp=0;
    for (Current->i = 0; Current->i < Current->N; ++Current->i)
    {
        sleep(1);
        /*
        Do the computation, then output result.
        */
        tmp=Current->y;
        Current->y+=Current->x;
        Current->x=tmp;
        printf("FibonacciSequence: %d\n",Current->y);
        ThreadYield();
    }
    ThreadExit(); 
    //no stopping condition for this function
}
