#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "timespec_operations.h"
#include "eat.h"

static struct timespec initial_time;

// Structure containing the parameters of each periodic thread
struct periodic_data
{
  struct timespec period;  // period
  struct timespec wcet1;   // first part of worst-case execution time
  struct timespec wcet2;   // second part of worst-case execution time
  struct timespec wcetmut; // worst-case execution time with mutex locked
  struct timespec phase;   // initial phase to start the thread
  struct timespec wcrt;    // worst-case response time
  pthread_mutex_t *mutex;  // pointer to mutex
  int id;                  // thread identifier
};

typedef enum
{
  YES,
  NO
} protocol_usage;

// Show a message with the relative elapsed time, and response_time
void report(char *message, int id, struct timespec *response_time)
{
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  decr_timespec(&now, &initial_time);
  printf("%3.3f - %s - %d", (double)(now.tv_sec + now.tv_nsec / 1.0e9), message, id);
  if (response_time == NULL)
  {
    printf("\n");
  }
  else
  {
    printf(" - %3.3f\n", (double)(response_time->tv_sec +
                                  response_time->tv_nsec / 1.0e9));
  }
}

// Periodic thread using nanosleep
void *periodic(void *arg)
{
  struct periodic_data *my_data = (struct periodic_data *)arg;
  struct timespec next_time;
  struct timespec response_time;
  int err;

  my_data->wcrt.tv_sec = 0;
  my_data->wcrt.tv_nsec = 0;

  // set initial time and wait for the critical instant
  next_time = initial_time;
  incr_timespec(&next_time, &(my_data->phase));
  if ((err = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
                             &next_time, NULL)) != 0)
  {
    printf("Error in clock_nanosleep: %s\n", strerror(err));
    pthread_exit(NULL);
  }

  // infinite loop where periodic work is done
  while (1)
  {
    // periodic work
    // report ("Start thread ",my_data->id,NULL);
    eat(&(my_data->wcet1)); // this simulates useful work
    if (my_data->mutex != NULL)
    {
      pthread_mutex_lock(my_data->mutex);
      eat(&(my_data->wcetmut));
      pthread_mutex_unlock(my_data->mutex);
    }
    eat(&(my_data->wcet2)); // this simulates useful work
    clock_gettime(CLOCK_MONOTONIC, &response_time);
    decr_timespec(&response_time, &next_time);
    // report ("End   thread ",my_data->id,&response_time);

    // set wcrt
    if smaller_timespec (&(my_data->wcrt), &response_time)
    {
      my_data->wcrt = response_time;
      report("Worst-case response time ", my_data->id, &(my_data->wcrt));
    }

    // wait for next period
    incr_timespec(&next_time, &(my_data->period));
    if ((err = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
                               &next_time, NULL)) != 0)
    {
      printf("Error in clock_nanosleep: %s\n", strerror(err));
      pthread_exit(NULL);
    }
  }
}

// Main program that creates four periodic threads
int main()
{
  pthread_t t1, t2, t3, t4;
  struct sched_param sch_param;
  pthread_attr_t attr;
  pthread_mutex_t mutex;
  pthread_mutexattr_t mutexattr;
  struct periodic_data data1, data2, data3, data4;

  const protocol_usage PROTOCOL = NO;

  // set data for all threads

  // Thread τ₁: C=0.15s, C_R1=0.05s (33%), T=1.4s
  // Distribution: before=0.03s (20%), mutex=0.05s (33%), after=0.07s (47%)
  data1.period.tv_sec = 1;
  data1.period.tv_nsec = 400000000;
  data1.wcet1.tv_sec = 0;
  data1.wcet1.tv_nsec = 30000000; // 0.03s before mutex (20% of C)
  data1.wcet2.tv_sec = 0;
  data1.wcet2.tv_nsec = 70000000; // 0.07s after mutex (47% of C)
  data1.wcetmut.tv_sec = 0;
  data1.wcetmut.tv_nsec = 50000000; // 0.05s in mutex (33% of C)
  data1.phase.tv_sec = 0;
  data1.phase.tv_nsec = 100000000; // 0.1s phase to start after τ₄
  data1.mutex = &mutex;
  data1.id = 1;

  // Thread τ₂: C=0.6s, no mutex, T=2.9s
  // Distribution: all execution time split evenly
  data2.period.tv_sec = 2;
  data2.period.tv_nsec = 900000000;
  data2.wcet1.tv_sec = 0;
  data2.wcet1.tv_nsec = 300000000; // 0.3s before
  data2.wcet2.tv_sec = 0;
  data2.wcet2.tv_nsec = 300000000; // 0.3s after
  data2.wcetmut.tv_sec = 0;
  data2.wcetmut.tv_nsec = 0;
  data2.phase.tv_sec = 0;
  data2.phase.tv_nsec = 50000000; // 0.05s phase
  data2.mutex = NULL;
  data2.id = 2;

  // Thread τ₃: C=2.7s, no mutex, T=13.0s
  // Distribution: all execution time split evenly
  data3.period.tv_sec = 13;
  data3.period.tv_nsec = 0;
  data3.wcet1.tv_sec = 1;
  data3.wcet1.tv_nsec = 350000000; // 1.35s before
  data3.wcet2.tv_sec = 1;
  data3.wcet2.tv_nsec = 350000000; // 1.35s after
  data3.wcetmut.tv_sec = 0;
  data3.wcetmut.tv_nsec = 0;
  data3.phase.tv_sec = 0;
  data3.phase.tv_nsec = 60000000; // 0.06s phase
  data3.mutex = NULL;
  data3.id = 3;

  // Thread τ₄: C=5.3s, C_R1=1.0s (19%), T=50.0s
  // Distribution: before=1.06s (20%), mutex=1.0s (19%), after=3.24s (61%)
  data4.period.tv_sec = 50;
  data4.period.tv_nsec = 0;
  data4.wcet1.tv_sec = 1;
  data4.wcet1.tv_nsec = 60000000; // 1.06s before mutex (20% of C)
  data4.wcet2.tv_sec = 3;
  data4.wcet2.tv_nsec = 240000000; // 3.24s after mutex (61% of C)
  data4.wcetmut.tv_sec = 1;
  data4.wcetmut.tv_nsec = 0; // 1.0s in mutex (19% of C)
  data4.phase.tv_sec = 0;
  data4.phase.tv_nsec = 0; // starts immediately
  data4.mutex = &mutex;
  data4.id = 4;

  // Set the priority of the main program to max_prio-1
  sch_param.sched_priority =
      (sched_get_priority_max(SCHED_FIFO) - 1);
  if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &sch_param) != 0)
  {
    printf("Error while setting main thread's priority\n");
    exit(1);
  }

  // Create the mutex attributes object
  pthread_mutexattr_init(&mutexattr);

  // Set the mutex protocol and ceiling
  if (PROTOCOL == YES)
  {
    pthread_mutexattr_setprotocol(&mutexattr, PTHREAD_PRIO_PROTECT);
    pthread_mutexattr_setprioceiling(&mutexattr, sched_get_priority_min(SCHED_FIFO) + 5);
  }

  // Create the mutex
  if (pthread_mutex_init(&mutex, &mutexattr) != 0)
  {
    printf("mutex_init\n");
    exit(1);
  }

  // Create the thread attributes object
  if (pthread_attr_init(&attr) != 0)
  {
    printf("Error while initializing attributes\n");
    exit(1);
  }

  // Set thread attributes

  // Never forget the inheritsched attribute
  // Otherwise the scheduling attributes are not used
  if (pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) != 0)
  {
    printf("Error in inheritsched attribute\n");
    exit(1);
  }

  // Thread is created dettached
  if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0)
  {
    printf("Error in detachstate attribute\n");
    exit(1);
  }

  // The scheduling policy is fixed-priorities, with
  // FIFO ordering for threads of the same priority
  if (pthread_attr_setschedpolicy(&attr, SCHED_FIFO) != 0)
  {
    printf("Error in schedpolicy attribute\n");
    exit(1);
  }

  // Set the priority of thread 1 to min_prio+5
  sch_param.sched_priority =
      (sched_get_priority_min(SCHED_FIFO) + 5);
  if (pthread_attr_setschedparam(&attr, &sch_param) != 0)
  {
    printf("Error en atributo schedparam\n");
    exit(1);
  }

  // create thread 1 with the attributes specified in attr_used

  if (pthread_create(&t1, &attr, periodic, &data1) != 0)
  {
    printf("Error en creacion de thread 1\n");
  }

  // Set the priority of thread 2 to min_prio+4
  sch_param.sched_priority =
      (sched_get_priority_min(SCHED_FIFO) + 4);
  if (pthread_attr_setschedparam(&attr, &sch_param) != 0)
  {
    printf("Error en atributo schedparam\n");
    exit(1);
  }

  // create thread 2 with the attributes specified in attr_used

  if (pthread_create(&t2, &attr, periodic, &data2) != 0)
  {
    printf("Error en creacion de thread 2\n");
  }

  // Set the priority of thread 3 to min_prio+3
  sch_param.sched_priority =
      (sched_get_priority_min(SCHED_FIFO) + 3);
  if (pthread_attr_setschedparam(&attr, &sch_param) != 0)
  {
    printf("Error en atributo schedparam\n");
    exit(1);
  }

  // create thread 3 with the attributes specified in attr_used

  if (pthread_create(&t3, &attr, periodic, &data3) != 0)
  {
    printf("Error en creacion de thread 3\n");
  }

  // Set the priority of thread 4 to min_prio+2
  sch_param.sched_priority =
      (sched_get_priority_min(SCHED_FIFO) + 2);
  if (pthread_attr_setschedparam(&attr, &sch_param) != 0)
  {
    printf("Error en atributo schedparam\n");
    exit(1);
  }

  // create thread 4 with the attributes specified in attr_used

  if (pthread_create(&t4, &attr, periodic, &data4) != 0)
  {
    printf("Error en creacion de thread 4\n");
  }

  // Read the initial time for use by the report function
  // and also as the time reference to synchronize all threads
  clock_gettime(CLOCK_MONOTONIC, &initial_time);

  sleep(1000);
}
