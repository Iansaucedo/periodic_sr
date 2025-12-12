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
  struct timespec period;   // period
  struct timespec wcet1;    // first part of worst-case execution time (before mutexes)
  struct timespec wcet2;    // middle part of execution time (between mutexes)
  struct timespec wcet3;    // last part of execution time (after mutexes)
  struct timespec wcetmut1; // worst-case execution time with mutex1 (R1) locked
  struct timespec wcetmut2; // worst-case execution time with mutex2 (R2) locked
  struct timespec phase;    // initial phase to start the thread
  struct timespec wcrt;     // worst-case response time
  pthread_mutex_t *mutex1;  // pointer to first mutex (R1)
  pthread_mutex_t *mutex2;  // pointer to second mutex (R2)
  int mutex_order;          // order of mutex acquisition: 1=R1->R2, 2=R2->R1
  int id;                   // thread identifier
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
  struct periodic_data *d = (struct periodic_data *)arg;
  struct timespec next_time = initial_time;
  struct timespec response_time;
  int err;

  d->wcrt.tv_sec = d->wcrt.tv_nsec = 0;

  incr_timespec(&next_time, &d->phase);
  if ((err = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_time, NULL)) != 0)
  {
    printf("Error in clock_nanosleep: %s\n", strerror(err));
    pthread_exit(NULL);
  }

  while (1)
  {
    report("Start thread ", d->id, NULL);
    eat(&d->wcet1);

    if (d->mutex1 && d->mutex2)
    {
      if (d->mutex_order == 1)
      {
        report("Thread trying to lock R1", d->id, NULL);
        pthread_mutex_lock(d->mutex1);
        report("Thread acquired R1", d->id, NULL);
        eat(&d->wcetmut1);

        report("Thread trying to lock R2", d->id, NULL);
        pthread_mutex_lock(d->mutex2);
        report("Thread acquired R2", d->id, NULL);
        eat(&d->wcetmut2);

        pthread_mutex_unlock(d->mutex2);
        report("Thread released R2", d->id, NULL);
        pthread_mutex_unlock(d->mutex1);
        report("Thread released R1", d->id, NULL);
      }
      else
      {
        report("Thread trying to lock R2", d->id, NULL);
        pthread_mutex_lock(d->mutex2);
        report("Thread acquired R2", d->id, NULL);
        eat(&d->wcetmut2);

        report("Thread trying to lock R1", d->id, NULL);
        pthread_mutex_lock(d->mutex1);
        report("Thread acquired R1", d->id, NULL);
        eat(&d->wcetmut1);

        pthread_mutex_unlock(d->mutex1);
        report("Thread released R1", d->id, NULL);
        pthread_mutex_unlock(d->mutex2);
        report("Thread released R2", d->id, NULL);
      }
    }
    else if (d->mutex1)
    {
      pthread_mutex_lock(d->mutex1);
      eat(&d->wcetmut1);
      pthread_mutex_unlock(d->mutex1);
    }

    eat(&d->wcet2);
    eat(&d->wcet3);

    clock_gettime(CLOCK_MONOTONIC, &response_time);
    decr_timespec(&response_time, &next_time);
    report("End   thread ", d->id, &response_time);

    if smaller_timespec (&d->wcrt, &response_time)
    {
      d->wcrt = response_time;
      report("Worst-case response time ", d->id, &d->wcrt);
    }

    incr_timespec(&next_time, &d->period);
    if ((err = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_time, NULL)) != 0)
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
  pthread_mutex_t mutex1, mutex2; // R1 and R2
  pthread_mutexattr_t mutexattr1, mutexattr2;
  struct periodic_data data1, data2, data3, data4;

  const protocol_usage PROTOCOL = YES; // Change to YES to avoid deadlock

  // set data for all threads

  // Thread τ₁: C=0.15s, C_R1=0.025s, C_R2=0.025s (total 0.05s in mutexes = 33%), T=1.4s
  // Distribution: before=0.03s (20%), R1=0.025s, R2=0.025s, between=0.01s, after=0.06s
  // Order: R1 -> R2 (mutex_order=1)
  data1.period.tv_sec = 1;
  data1.period.tv_nsec = 400000000;
  data1.wcet1.tv_sec = 0;
  data1.wcet1.tv_nsec = 30000000; // 0.03s before mutexes (20% of C)
  data1.wcet2.tv_sec = 0;
  data1.wcet2.tv_nsec = 10000000; // 0.01s between mutexes
  data1.wcet3.tv_sec = 0;
  data1.wcet3.tv_nsec = 60000000; // 0.06s after mutexes
  data1.wcetmut1.tv_sec = 0;
  data1.wcetmut1.tv_nsec = 25000000; // 0.025s in R1
  data1.wcetmut2.tv_sec = 0;
  data1.wcetmut2.tv_nsec = 25000000; // 0.025s in R2
  data1.phase.tv_sec = 0;
  data1.phase.tv_nsec = 0; // Start immediately to create deadlock condition
  data1.mutex1 = &mutex1;  // R1
  data1.mutex2 = &mutex2;  // R2
  data1.mutex_order = 1;   // Order: R1 -> R2
  data1.id = 1;

  // Thread τ₂: C=0.6s, no mutex, T=2.9s
  // Distribution: split in three parts
  data2.period.tv_sec = 2;
  data2.period.tv_nsec = 900000000;
  data2.wcet1.tv_sec = 0;
  data2.wcet1.tv_nsec = 200000000; // 0.2s
  data2.wcet2.tv_sec = 0;
  data2.wcet2.tv_nsec = 200000000; // 0.2s
  data2.wcet3.tv_sec = 0;
  data2.wcet3.tv_nsec = 200000000; // 0.2s
  data2.wcetmut1.tv_sec = 0;
  data2.wcetmut1.tv_nsec = 0;
  data2.wcetmut2.tv_sec = 0;
  data2.wcetmut2.tv_nsec = 0;
  data2.phase.tv_sec = 0;
  data2.phase.tv_nsec = 50000000; // 0.05s phase
  data2.mutex1 = NULL;
  data2.mutex2 = NULL;
  data2.mutex_order = 0;
  data2.id = 2;

  // Thread τ₃: C=2.7s, no mutex, T=13.0s
  // Distribution: split in three parts
  data3.period.tv_sec = 13;
  data3.period.tv_nsec = 0;
  data3.wcet1.tv_sec = 0;
  data3.wcet1.tv_nsec = 900000000; // 0.9s
  data3.wcet2.tv_sec = 0;
  data3.wcet2.tv_nsec = 900000000; // 0.9s
  data3.wcet3.tv_sec = 0;
  data3.wcet3.tv_nsec = 900000000; // 0.9s
  data3.wcetmut1.tv_sec = 0;
  data3.wcetmut1.tv_nsec = 0;
  data3.wcetmut2.tv_sec = 0;
  data3.wcetmut2.tv_nsec = 0;
  data3.phase.tv_sec = 0;
  data3.phase.tv_nsec = 60000000; // 0.06s phase
  data3.mutex1 = NULL;
  data3.mutex2 = NULL;
  data3.mutex_order = 0;
  data3.id = 3;

  // Thread τ₄: C=5.3s, C_R1=0.5s, C_R2=0.5s (total 1.0s in mutexes = 19%), T=50.0s
  // Distribution: before=1.06s (20%), R2=0.5s, R1=0.5s, between=0.24s, after=3.0s
  // Order: R2 -> R1 (mutex_order=2) - OPPOSITE to τ₁ to create DEADLOCK!
  data4.period.tv_sec = 50;
  data4.period.tv_nsec = 0;
  data4.wcet1.tv_sec = 1;
  data4.wcet1.tv_nsec = 60000000; // 1.06s before mutexes (20% of C)
  data4.wcet2.tv_sec = 0;
  data4.wcet2.tv_nsec = 240000000; // 0.24s between mutexes
  data4.wcet3.tv_sec = 3;
  data4.wcet3.tv_nsec = 0; // 3.0s after mutexes
  data4.wcetmut1.tv_sec = 0;
  data4.wcetmut1.tv_nsec = 500000000; // 0.5s in R1
  data4.wcetmut2.tv_sec = 0;
  data4.wcetmut2.tv_nsec = 500000000; // 0.5s in R2
  data4.phase.tv_sec = 0;
  data4.phase.tv_nsec = 10000000; // Start at 0.01s, slightly after τ₁ to create deadlock
  data4.mutex1 = &mutex1;         // R1
  data4.mutex2 = &mutex2;         // R2
  data4.mutex_order = 2;          // Order: R2 -> R1 (OPPOSITE to thread 1!)
  data4.id = 4;

  // Set the priority of the main program to max_prio-1
  sch_param.sched_priority =
      (sched_get_priority_max(SCHED_FIFO) - 1);
  if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &sch_param) != 0)
  {
    printf("Error while setting main thread's priority\n");
    exit(1);
  }

  // Create the mutex attributes objects for R1 and R2
  pthread_mutexattr_init(&mutexattr1);
  pthread_mutexattr_init(&mutexattr2);

  // Set the mutex protocol and ceiling for both mutexes
  if (PROTOCOL == YES)
  {
    // With priority ceiling protocol, deadlock is avoided
    pthread_mutexattr_setprotocol(&mutexattr1, PTHREAD_PRIO_PROTECT);
    pthread_mutexattr_setprioceiling(&mutexattr1, sched_get_priority_min(SCHED_FIFO) + 5);

    pthread_mutexattr_setprotocol(&mutexattr2, PTHREAD_PRIO_PROTECT);
    pthread_mutexattr_setprioceiling(&mutexattr2, sched_get_priority_min(SCHED_FIFO) + 5);
  }

  // Create mutex R1
  if (pthread_mutex_init(&mutex1, &mutexattr1) != 0)
  {
    printf("mutex1_init (R1)\n");
    exit(1);
  }

  // Create mutex R2
  if (pthread_mutex_init(&mutex2, &mutexattr2) != 0)
  {
    printf("mutex2_init (R2)\n");
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

  // Capture initial time before creating the threads so timestamps start near 0
  clock_gettime(CLOCK_MONOTONIC, &initial_time);

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

  sleep(1000);
}
