#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
//Project 4
//@author Carl Yarwood
//last edited 5/5/2018
pthread_t *control;
pthread_t *workers;
pthread_barrier_t *firstBarrier;
pthread_barrier_t *secondBarrier;
pthread_mutex_t* randomLock;
int * numSuccessBuffer;
int * numIterations;
int * keepGoing;
struct inControl{
  uint numWorkers;
  double delta;
};
void* controlFunc(void *);
void* workerFunc(void *);
int main(int argc, char ** argv){
  //checking input into the program
  if( argc < 4){
    printf("Error, retry with correct parameters\nnumber of Workers, number of itterations, and the delta value\n");
    return -1;
  }
  //declaration and init
  int *workerID;
  struct inControl *inC = (struct inControl *)malloc(sizeof(struct inControl));
  numIterations = (int *)malloc(sizeof(int));
  keepGoing = (int *)malloc(sizeof(int));
  *keepGoing = 1;
  sscanf(argv[1] ,"%u", &(inC->numWorkers));
  sscanf(argv[2] ,"%d", numIterations);
  sscanf(argv[3] ,"%lf", &(inC->delta));
  //making sure we have a valid number of workers
  if(inC->numWorkers < 1){
    printf("must have more than zero threads\n");
    return -2;
  }
  randomLock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
  control = (pthread_t *)malloc(sizeof(pthread_t));
  workers = (pthread_t *)malloc(sizeof(pthread_t) * inC->numWorkers);
  workerID = (int *)malloc(sizeof(int) * inC->numWorkers);
  numSuccessBuffer = (int *)malloc(sizeof(int) * inC->numWorkers);
  firstBarrier = (pthread_barrier_t *)malloc(sizeof(pthread_barrier_t));
  secondBarrier = (pthread_barrier_t *)malloc(sizeof(pthread_barrier_t));
  pthread_mutex_init(randomLock, NULL);
  pthread_barrier_init(firstBarrier,NULL, inC->numWorkers + 1);
  pthread_barrier_init(secondBarrier, NULL, inC->numWorkers + 1);
  //create threads
  pthread_create(control, NULL, controlFunc, (void*) inC);
  for(uint i = 0; i < inC->numWorkers; i++){
    workerID[i] = i;
    pthread_create(&workers[i], NULL, workerFunc, (void*) &workerID[i]);
  }
  pthread_join(*control, NULL);
  for(uint i = 0; i < inC->numWorkers; i++){
    pthread_join(workers[i], NULL);
  }
  //free statements
  free(numIterations);
  free(inC);
  free(control);
  free(workers);
  free(firstBarrier);
  free(secondBarrier);
  return 0;
}
void* controlFunc(void* inarg){
  //grab input and extract necissary info
  struct inControl *in = (struct inControl*) inarg;
  double delta = in->delta;
  uint numWorkers = in->numWorkers;
  double numEst = 0.0;
  double last = 0.0;
  double current = 0.0;
  //start work
  while(*keepGoing){
     //wait for workers to finish
    pthread_barrier_wait(firstBarrier);
    //grab all the differnet info form workers and process
    for(uint i = 0; i< numWorkers; i++){
      current = current + (4.0*((double)numSuccessBuffer[i]/((double) *numIterations)));
    }
    //divide info by num workers for average
    current = current / (double) numWorkers;
    numEst = numEst + 1.0;
    //appropriatly weigh the most recent estimation with past
    //estimations to ensure propper convergence
    current = (current/numEst) + (last * ((numEst - 1.0)/numEst));
    //print current estimate
    printf("The Current estimate is %lf\n", current);
    //check if futher interation is nedded
    if(fabs(current - last) <= delta){
      *keepGoing = 0;
    }
    else{
      //if not reset and updated nedded values
      last = current;
      current = 0.0;
    }
    pthread_barrier_wait(secondBarrier);
  }
  return NULL;
}
void* workerFunc(void* inarg){
  int ID = *(int*) inarg;
  time_t t;
  //seed random number genterator
  srand((unsigned) time(&t));
  while(*keepGoing){
    //reset buffer for new information
    numSuccessBuffer[ID] = 0;
    //preform experiments and put number of successe into buffer
    for(int i = 0; i< *numIterations; i++){
      //random is not thread safe to we lock it
      pthread_mutex_lock(randomLock);
      double rx = (double) rand() / (double) RAND_MAX;
      double ry = (double) rand() / (double) RAND_MAX;
      pthread_mutex_unlock(randomLock);
      double distance = sqrt(pow((rx-0.5),2.0)+pow((ry-0.5),2.0));
      if(distance <= 0.5){
	numSuccessBuffer[ID] = numSuccessBuffer[ID] + 1;
      }
    }
    pthread_barrier_wait(firstBarrier);
    //wait for control to findish working
    pthread_barrier_wait(secondBarrier);
  }
  return NULL;
}
