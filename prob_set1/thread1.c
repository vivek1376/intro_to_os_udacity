#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define NUM_THREADS 5

int resourceCnt = 0;
int waitingReaders = 0;

void *reader (void *);		/* reader function */
void *writer (void *);		/* writer function */

pthread_mutex_t mutex_R_W = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_R = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_W = PTHREAD_COND_INITIALIZER;

char sVar = '*';

int main()
{
    srand (time (NULL));
    //printf("qq\n");//d

    pthread_t reader_th[5], writer_th[5];
    int i, iret1, iret2, rArgs[NUM_THREADS], wArgs[NUM_THREADS];

    for (i = 0; i < NUM_THREADS; i++)
    {
        rArgs[i] = i + 1;
        iret1 = pthread_create (&reader_th[i], NULL, reader, &rArgs[i]);
        if (iret1)
        {
            fprintf (stderr, "Error - pthread_create() return code: %d\n", iret1);
            exit (EXIT_FAILURE);
        }
    }

    /* create writer threads */
    for (i = 0; i < NUM_THREADS; i++)
    {
        wArgs[i] = i + 1;
        iret2 = pthread_create (&writer_th[i], NULL, writer, &wArgs[i]);
        if (iret2)
        {
            fprintf (stderr, "Error - pthread_create() return code: %d\n", iret2);
            exit (EXIT_FAILURE);
        }
    }

    /* join reader threads */
    for (i = 0; i < NUM_THREADS; i++)
        pthread_join (reader_th[i], NULL);

    /* join writer threads */
    for (i = 0; i < NUM_THREADS; i++)
        pthread_join (writer_th[i], NULL);

    //  return 0;
    exit (EXIT_SUCCESS);
}

void *reader (void *arg)
{
//    printf("rrr\n");//d
    usleep ( (rand() % 10000) * 200);	/* max 100 ms */

    int ret;
    ret = pthread_mutex_lock (&mutex_R_W); /* lock */

//    printf("ret: %d\n",ret);//d
    if (ret)
        printf ("mutex lock error\n");

    while (resourceCnt == -1)
    {
        waitingReaders++;
        pthread_cond_wait (&cond_R, &mutex_R_W); /* release lock & wait */
        waitingReaders--;
    }
    /* mutex_R_W is locked now */
    //  printf("rrrr\n");//d
    resourceCnt++;		    /* positive value means reader access */
    pthread_mutex_unlock (&mutex_R_W); /* release lock */

    //usleep((rand()%10000)*300);	/* max 100 ms */

    /* critical section */
    int i;
    i = * ( (int*) arg);

    printf (" R%d\tcount[R]: %d\t[val]: %c\n", i, resourceCnt, sVar);
    //printf("[val]: %c\n",sVar);
    fflush (stdout);
    /* end CS */

    pthread_mutex_lock (&mutex_R_W);
    resourceCnt--;
    pthread_mutex_unlock (&mutex_R_W);

    pthread_cond_signal (&cond_W); /* signal waiting writer */

    return NULL;
}

void *writer (void *arg)
{
    usleep ( (rand() % 10000) * 200);	/* max 500 ms */

    pthread_mutex_lock (&mutex_R_W); /* lock */

    while (resourceCnt != 0 || waitingReaders>0) /* 2nd condition gives priority
						    to waiting readers */
        pthread_cond_wait (&cond_W, &mutex_R_W);
    /* mutex_R_W is locked */

    resourceCnt--;

    pthread_mutex_unlock (&mutex_R_W); /* release lock */

    /* critical section */
    int i;
    char old;

    i = * ( (int*) arg);
    old = sVar;
    sVar = 'A' + i;

    //printf ("+W%d\t[waitingR]:%d\t\t[old]: %c\t[new]: %c\n", i, waitingReaders,
    //old, sVar);
    printf ("+W%d\t[old]: %c\t[new]: %c\n", i, old, sVar);
    fflush (stdout);
    /* end CS */

    pthread_mutex_lock (&mutex_R_W); /* acquire lock before accessing resourceCnt */
    resourceCnt = 0;
    pthread_mutex_unlock (&mutex_R_W); /* release lock */

    /* signal waiting reader & writer threads */
    pthread_cond_broadcast (&cond_R);

    //  if(waitingReaders==0)
    pthread_cond_signal (&cond_W);

    return NULL;
}

