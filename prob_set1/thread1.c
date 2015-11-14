#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define NUM_THREADS 5
#define NUM_READS 5
#define NUM_WRITES 5

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

    pthread_t reader_th[5], writer_th[5]; /* 5 reader & writer threads */
    int i, iret, rArgs[NUM_THREADS], wArgs[NUM_THREADS];

    for (i = 0; i < NUM_THREADS; i++)
    {
        rArgs[i] = i + 1;
        iret = pthread_create (&reader_th[i], NULL, reader, &rArgs[i]);
        if (iret)
        {
            fprintf (stderr, "Error - pthread_create() return code: %d\n", iret);
            exit (EXIT_FAILURE);
        }
    }

    /* create writer threads */
    for (i = 0; i < NUM_THREADS; i++)
    {
        wArgs[i] = i + 1;
        iret = pthread_create (&writer_th[i], NULL, writer, &wArgs[i]);
        if (iret)
        {
            fprintf (stderr, "Error - pthread_create() return code: %d\n", iret);
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
    int id, numReader=0, i;
    id = * ( (int*) arg);

    for (i=0; i<NUM_READS; i++)
    {
	usleep ( (rand() % 10000 + 50) * 200);	/* max 100 ms */
	
	int ret = pthread_mutex_lock (&mutex_R_W); /* lock */
	
	if (ret)
	    printf ("mutex lock error\n");
	
	waitingReaders++;
	while (resourceCnt == -1)
	    pthread_cond_wait (&cond_R, &mutex_R_W); /* release lock & wait */

	/* mutex_R_W is locked now */

	waitingReaders--;

	numReader=++resourceCnt;		    /* positive value means reader access */
	pthread_mutex_unlock (&mutex_R_W); /* release lock */
	
	//usleep((rand()%10000)*300);	/* max 100 ms */
	
	/* critical section */
	printf (" R%d\tcount[R]: %d\t[val]: %c\n", id, 
		numReader, sVar); /* using numReader instead of
				     resourceCnt as resourceCnt can 
				     change after mutex is unlocked */

	fflush (stdout);
	/* end CS */
	
	pthread_mutex_lock (&mutex_R_W);
	resourceCnt--;

	if(resourceCnt==0)
	    pthread_cond_signal (&cond_W); /* signal waiting writer */
	pthread_mutex_unlock (&mutex_R_W);
	
    }
    
    return NULL;
}

void *writer (void *arg)
{
    int id, i;
    char old;
    id = * ( (int*) arg);

    for(i=0; i<NUM_WRITES; i++)
    {
	usleep ( (rand() % 10000 + 50) * 200);	/* max 500 ms */
	
	pthread_mutex_lock (&mutex_R_W); /* lock */
	
	while (resourceCnt != 0 || waitingReaders>0) /* 2nd condition gives priority
							to waiting readers. is it redundant
							? as waiting writers are signalled
							only if waitingReaders==0 */
	    pthread_cond_wait (&cond_W, &mutex_R_W);
	/* mutex_R_W is locked */
	
	resourceCnt = -1;
	
	pthread_mutex_unlock (&mutex_R_W); /* release lock */
	
	/* critical section */
	old = sVar;
	sVar = 'A' + i;
	
	printf ("+W%d\t[old]: %c\t[new]: %c\n", id, old, sVar);
	fflush (stdout);
	/* end CS */
	
	pthread_mutex_lock (&mutex_R_W); /* acquire lock before accessing resourceCnt */
	resourceCnt = 0;

	/* signal waiting reader & writer threads */
	if(waitingReaders>0)
	    pthread_cond_broadcast (&cond_R);
	else
	    pthread_cond_signal (&cond_W);

	pthread_mutex_unlock (&mutex_R_W); /* release lock */
    }
    return NULL;
}

