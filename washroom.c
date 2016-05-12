#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include "uthread.h"                  // uthread and uthread_mutex_cond files written by Dr. Mike Feeley
#include "uthread_mutex_cond.h"       // and not included here
#include <time.h>

#ifdef VERBOSE
#define VERBOSE_PRINT(S, ...) printf (S, ##__VA_ARGS__);
#else
#define VERBOSE_PRINT(S, ...) ;
#endif

#define MAX_OCCUPANCY      3
#define NUM_ITERATIONS     100
#define NUM_PEOPLE         20
#define FAIR_WAITING_COUNT 4

/**
 * You might find these declarations useful.
 */

enum Sex {MALE = 0, FEMALE = 1};
const static enum Sex otherSex [] = {FEMALE, MALE};

struct Washroom {
  // TODO
  uthread_mutex_t mutex;
  uthread_cond_t  maleAvailable;
  uthread_cond_t  femaleAvailable;
  int sex;
  int numPeople;
};

struct Washroom* createWashroom() {
  struct Washroom* washroom = malloc (sizeof (struct Washroom));
  // TODO
  washroom->mutex = uthread_mutex_create();
  washroom->maleAvailable = uthread_cond_create(washroom->mutex);
  washroom->femaleAvailable = uthread_cond_create(washroom->mutex);
  washroom->numPeople = 0;
  washroom->sex = 2;
  return washroom;
}

#define WAITING_HISTOGRAM_SIZE (NUM_ITERATIONS * NUM_PEOPLE)
int             entryTicker;  // incremented with each entry
int             waitingHistogram         [WAITING_HISTOGRAM_SIZE];
int             waitingHistogramOverflow;
uthread_mutex_t waitingHistogrammutex;
int             occupancyHistogram       [2] [MAX_OCCUPANCY + 1];

void recordWaitingTime (int waitingTime) {
  uthread_mutex_lock (waitingHistogrammutex);
  if (waitingTime < WAITING_HISTOGRAM_SIZE)
    waitingHistogram [waitingTime] ++;
  else
    waitingHistogramOverflow ++;
  uthread_mutex_unlock (waitingHistogrammutex);
}

void enterWashroom (struct Washroom* washroom, enum Sex Sex) {
  // TODO
  uthread_mutex_lock(washroom->mutex);
    int initial = entryTicker;
    while(washroom->numPeople == MAX_OCCUPANCY || washroom->sex == otherSex[Sex]){
      if(Sex == 0)
        uthread_cond_wait(washroom->maleAvailable);
      else
        uthread_cond_wait(washroom->femaleAvailable);
    }
    
    if(washroom->sex == MALE && washroom->numPeople != 0){
      assert(Sex == MALE);
    }
    else if(washroom->sex == FEMALE && washroom->numPeople != 0){
      assert(Sex == FEMALE);
    }

    washroom->numPeople++;
    occupancyHistogram[Sex][washroom->numPeople]++;
    recordWaitingTime(entryTicker-initial);
    entryTicker++;
    if(Sex != washroom->sex) {
      washroom->sex = Sex;
    }
  assert(washroom->numPeople < 4);  
  uthread_mutex_unlock(washroom->mutex);
  
}



void leaveWashroom (struct Washroom* washroom) {
  // TODO
  uthread_mutex_lock(washroom->mutex);

    washroom->numPeople--;

    if(washroom->numPeople == 0){
      washroom->sex = 2;
      uthread_cond_signal(washroom->maleAvailable);
      uthread_cond_signal(washroom->femaleAvailable);
    }
   
    if(washroom->sex == 0){
      occupancyHistogram[MALE][washroom->numPeople]++;
      uthread_cond_signal(washroom->maleAvailable);
    }
    else if(washroom->sex ==1) {
      occupancyHistogram[FEMALE][washroom->numPeople]++;
      uthread_cond_signal(washroom->femaleAvailable);
    }

  uthread_mutex_unlock(washroom->mutex);
}



//
// TODO
// You will probably need to create some additional produres etc.
//
void* male(void* wv){
  struct Washroom* w  = wv;
  enum Sex sex = 0;
  for(int i=0;i<NUM_ITERATIONS;i++){
    enterWashroom(w, sex);
    for(int i=0;i<NUM_PEOPLE;i++){
      uthread_yield();
    }

    leaveWashroom(w);

    for(int i=0;i<NUM_PEOPLE;i++){
      uthread_yield();
    }
  }
  
  return NULL;
}

void* female(void* wv){
  struct Washroom* w  = wv;
  enum Sex sex = 1;
  for(int i=0;i<NUM_ITERATIONS;i++){
    enterWashroom(w, sex);
    for(int i=0;i<NUM_PEOPLE;i++){
      uthread_yield();
    }

    leaveWashroom(w);

    for(int i=0;i<NUM_PEOPLE;i++){
      uthread_yield();
    }
  }
  
  return NULL;
}

int main (int argc, char** argv) {
  uthread_init (8);
  struct Washroom* washroom = createWashroom();
  uthread_t        pt [NUM_PEOPLE];
  waitingHistogrammutex = uthread_mutex_create ();
  srand(time(NULL));
  int numMales = rand() % (NUM_PEOPLE+1);
  int numFemales = NUM_PEOPLE - numMales;
  // TODO
  for(int i=0;i<numMales;i++){
    pt[i]= uthread_create(male, washroom);
  }
  for(int i=numMales;i<NUM_PEOPLE;i++){
    pt[i]= uthread_create(female, washroom);
  }
  for(int i=0; i<NUM_PEOPLE;i++){
    uthread_join(pt[i],0);
  }


  printf ("Times with 1 male    %d\n", occupancyHistogram [MALE]   [1]);
  printf ("Times with 2 males   %d\n", occupancyHistogram [MALE]   [2]);
  printf ("Times with 3 males   %d\n", occupancyHistogram [MALE]   [3]);
  printf ("Times with 1 female  %d\n", occupancyHistogram [FEMALE] [1]);
  printf ("Times with 2 females %d\n", occupancyHistogram [FEMALE] [2]);
  printf ("Times with 3 females %d\n", occupancyHistogram [FEMALE] [3]);
  printf ("Waiting Histogram\n");
  for (int i=0; i<WAITING_HISTOGRAM_SIZE; i++)
    if (waitingHistogram [i])
      printf ("  Number of times people waited for %d %s to enter: %d\n", i, i==1?"person":"people", waitingHistogram [i]);
  if (waitingHistogramOverflow)
    printf ("  Number of times people waited more than %d entries: %d\n", WAITING_HISTOGRAM_SIZE, waitingHistogramOverflow);
}