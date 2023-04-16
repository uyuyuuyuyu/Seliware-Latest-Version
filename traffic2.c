#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

#include "uthread.h"
#include "uthread_mutex_cond.h"

#ifdef VERBOSE
#define VERBOSE_PRINT(S, ...) printf (S, ##__VA_ARGS__)
#else
#define VERBOSE_PRINT(S, ...) ((void) 0) // do nothing
#endif

#define MAX_OCCUPANCY  3
#define NUM_ITERATIONS 100
#define NUM_CARS       20

// These times determine the number of times yield is called when in
// the street, or when waiting before crossing again.
#define CROSSING_TIME             NUM_CARS
#define WAIT_TIME_BETWEEN_CROSSES NUM_CARS

/**
 * You might find these declarations useful.
 */
enum Direction {EAST = 0, WEST = 1};
const static enum Direction oppositeEnd [] = {WEST, EAST};

struct Street {
  // TODO

  uthread_mutex_t mutex;
  uthread_cond_t  canEnter[2];
  int inStreetNum;
  enum Direction currentDirection;
} Street;

void initializeStreet(void) {
  // TODO

  Street.mutex = uthread_mutex_create();
  Street.canEnter[EAST] = uthread_cond_create(Street.mutex);
  Street.canEnter[WEST] = uthread_cond_create(Street.mutex);
  Street.inStreetNum = 0;
  Street.currentDirection = EAST;
}

#define WAITING_HISTOGRAM_SIZE (NUM_ITERATIONS * NUM_CARS)
int             entryTicker;                                          // incremented with each entry
int             waitingHistogram [WAITING_HISTOGRAM_SIZE];
int             waitingHistogramOverflow;
uthread_mutex_t waitingHistogramLock;
int             occupancyHistogram [2] [MAX_OCCUPANCY + 1];

void recordWaitingTime (int waitingTime) {
  uthread_mutex_lock (waitingHistogramLock);
  if (waitingTime < WAITING_HISTOGRAM_SIZE)
    waitingHistogram [waitingTime] ++;
  else
    waitingHistogramOverflow ++;
  uthread_mutex_unlock (waitingHistogramLock);
}

void enterStreet (enum Direction g) {
  // TODO
  uthread_mutex_lock(Street.mutex);
  int entryTicket = entryTicker++;
  while (Street.inStreetNum == MAX_OCCUPANCY ||(Street.inStreetNum > 0&&Street.currentDirection != g)) {
    uthread_cond_wait(Street.canEnter[g]);
  }
  Street.inStreetNum++;
  occupancyHistogram[g][Street.inStreetNum]++;
  recordWaitingTime(entryTicker - entryTicket - 1);
  //uthread_cond_signal(Street.canEnter[g]);
  uthread_mutex_unlock(Street.mutex);
  
}

void leaveStreet(void) {
  // TODO

  uthread_mutex_lock(Street.mutex);
  Street.inStreetNum--;
  if (Street.inStreetNum == 0) {
    // uthread_cond_broadcast(Street.canEnter[Street.currentDirection]);
    uthread_cond_broadcast(Street.canEnter[oppositeEnd[Street.currentDirection]]);
    Street.currentDirection = oppositeEnd[Street.currentDirection];
  } else {
    uthread_cond_signal(Street.canEnter[Street.currentDirection]);
  }

  uthread_mutex_unlock(Street.mutex);
}



void* car(void* arg) {
  enum Direction dir = (enum Direction)arg;
  for (int i = 0; i < NUM_ITERATIONS; i++) {
    enterStreet(dir);
    uthread_yield();
    for (int j = 0; j < CROSSING_TIME; j++) {
      uthread_yield();
    }
    leaveStreet();
    for (int j = 0; j < WAIT_TIME_BETWEEN_CROSSES; j++) {
      uthread_yield();
    }
  }
  return NULL;
}

//
// TODO
// You will probably need to create some additional procedures etc.
//

int main (int argc, char** argv) {

  uthread_init(8);

  waitingHistogramLock = uthread_mutex_create();

  
  initializeStreet();
  uthread_t pt [NUM_CARS];

  // TODO

   for (int i = 0; i < NUM_CARS; i++) {
    enum Direction dir = i % 2 == 0 ? EAST : WEST;
    pt[i] = uthread_create(car, (void*)dir);
  }

    for (int i = 0; i < NUM_CARS; i++) {
    uthread_join(pt[i], NULL);
  }
  
  printf ("Times with 1 car  going east: %d\n", occupancyHistogram [EAST] [1]);
  printf ("Times with 2 cars going east: %d\n", occupancyHistogram [EAST] [2]);
  printf ("Times with 3 cars going east: %d\n", occupancyHistogram [EAST] [3]);
  printf ("Times with 1 car  going west: %d\n", occupancyHistogram [WEST] [1]);
  printf ("Times with 2 cars going west: %d\n", occupancyHistogram [WEST] [2]);
  printf ("Times with 3 cars going west: %d\n", occupancyHistogram [WEST] [3]);
  
  printf ("Waiting Histogram\n");
  for (int i=0; i < WAITING_HISTOGRAM_SIZE; i++)
    if (waitingHistogram [i])
      printf ("  Cars waited for           %4d car%s to enter: %4d time(s)\n",
	      i, i==1 ? " " : "s", waitingHistogram [i]);
  if (waitingHistogramOverflow)
    printf ("  Cars waited for more than %4d cars to enter: %4d time(s)\n",
	    WAITING_HISTOGRAM_SIZE, waitingHistogramOverflow);
}
