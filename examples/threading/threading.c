#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    int error_code;

    if(NULL == thread_param){
	    return false;
    }
    
    struct thread_data *THREAD_PARAM  =  (struct thread_data *)thread_param;

    error_code = usleep(THREAD_PARAM->wait_to_obtain_ms*1000);
    if(-1 == error_code){
	    return thread_param;
    }


    error_code = pthread_mutex_lock(THREAD_PARAM->mutex);
    if(-1 == error_code){
            return thread_param;
    }


    error_code = usleep(THREAD_PARAM->wait_to_release_ms*1000);
    if(-1 == error_code){
            return thread_param;
    }

    error_code = pthread_mutex_unlock(THREAD_PARAM->mutex);
    if(-1 == error_code){
	    return thread_param;
    }

    if(0== error_code){
	    THREAD_PARAM->thread_complete_success=true;
    }
    else{
	    THREAD_PARAM->thread_complete_success=false;
    }

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */


    struct thread_data *THREAD_DATA = (struct thread_data*)malloc(sizeof(struct thread_data));

    int error_code = true;

    if(NULL == THREAD_DATA){
	    return false;
    }

    THREAD_DATA->wait_to_obtain_ms= wait_to_obtain_ms;
    THREAD_DATA->wait_to_release_ms=wait_to_release_ms;
    THREAD_DATA->mutex=mutex;

    error_code = pthread_create(thread,NULL,threadfunc, THREAD_DATA);

    if(error_code){
	    return false;
    }

    return true;
}

