#include "dining.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct dining {
  // this is the student_mutex used for students
  pthread_mutex_t student_mutex;
  // this is the student_mutex used for cleaners
  pthread_mutex_t cleaner_mutex;
  // conditional variable for when cleaning is finished
  pthread_cond_t cleaning_done;
  // conditional vairbale for when all students have left
  pthread_cond_t student_done;
  // semaphore used to keep track of students
  sem_t students;
  // semaphore used to keep track of cleaners
  sem_t cleaners;
  // flag to see if cleaning is going on
  bool cleaning;
  // dining hall capacity
  int capacity;
  // keeps track of students in dining hall
  int num_students;
  // keeps track of cleaners in dining hall
  int num_cleaners;
} dining_t;

dining_t *dining_init(int capacity) {
  dining_t *dining = malloc(sizeof(dining_t));
  // initialize student student_mutex
  pthread_mutex_init(&dining->student_mutex, NULL);
  // initialize cleaner student_mutex
  pthread_mutex_init(&dining->cleaner_mutex, NULL);
  // initialize cleaning condition variable
  pthread_cond_init(&dining->cleaning_done, NULL);
  // initialize student condition variable
  pthread_cond_init(&dining->student_done, NULL);
  // set the student semaphore to be the capacity of the dining hall
  sem_init(&dining->students, 0, capacity);
  // set the cleaning semaphore to be 1
  sem_init(&dining->cleaners, 0, 1);
  // set cleaning bool variable to false
  dining->cleaning = false;
  // initialize capacity to dining hall capacity
  dining->capacity = capacity;
  // number of students is set to 0
  dining->num_students = 0;
  // number of cleaners is set to 0
  dining->num_cleaners = 0;
  return dining;
}

void dining_destroy(dining_t **dining_ptr) {
  dining_t *dining = *dining_ptr;
  // destroy the student_mutex for students
  pthread_mutex_destroy(&dining->student_mutex);
  // destroy the student_mutex for cleaners
  pthread_mutex_destroy(&dining->cleaner_mutex);
  // destroy the conditional variable for cleaners
  pthread_cond_destroy(&dining->cleaning_done);
  // destroy the conditional variable for students
  pthread_cond_destroy(&dining->student_done);
  // destroy the sempahore for students
  sem_destroy(&dining->students);
  // destroy the semaphore for cleaners
  sem_destroy(&dining->cleaners);
  free(dining);
  *dining_ptr = NULL;
}

void dining_student_enter(dining_t *dining) {
  // waiting for the student semaphore
  sem_wait(&dining->students);
  // here we lock the student student_mutex
  pthread_mutex_lock(&dining->student_mutex);
  // block function if dining hall is full or there are cleaners
  while (dining->num_cleaners > 0 || dining->num_students == dining->capacity) {
    pthread_cond_wait(&dining->student_done, &dining->student_mutex);
  }
  // increment student counter
  dining->num_students++;
  // unlock the student_mutex for students
  pthread_mutex_unlock(&dining->student_mutex);
}

void dining_student_leave(dining_t *dining) {
  // lock student_mutex for students
  pthread_mutex_lock(&dining->student_mutex);
  // decrement student counter
  dining->num_students--;
  // here we signal the student semaphore
  sem_post(&dining->students);
  // check to see if no students are there and cleaning is going on
  if (dining->num_students == 0 && dining->cleaning) {
    // broadcast that students have left and cleaning is done
    pthread_cond_broadcast(&dining->student_done);
    pthread_cond_broadcast(&dining->cleaning_done);
  }
  // unlock the student student_mutex
  pthread_mutex_unlock(&dining->student_mutex);
}

void dining_cleaning_enter(dining_t *dining) {
  // waiting for cleaner semaphore
  sem_wait(&dining->cleaners);
  // lock the cleaner student_mutex
  pthread_mutex_lock(&dining->cleaner_mutex);
  // increment the cleaning counter
  dining->num_cleaners++;
  // set cleaning flag to true
  dining->cleaning = true;
  // if the dining hall has students, then block the function
  while (dining->num_students > 0) {
    pthread_cond_wait(&dining->cleaning_done, &dining->cleaner_mutex);
  }
}

void dining_cleaning_leave(dining_t *dining) {
  // decrement the number of cleaners
  dining->num_cleaners--;
  // if there are 0 cleaners and cleaning is happening
  if (dining->num_cleaners == 0 &&
      dining->cleaning) {  // check if all cleaners have left
    // change the cleaning flag to false
    dining->cleaning = false;
    // broadcast that cleaning is finished and students can enter
    pthread_cond_broadcast(&dining->cleaning_done);
    pthread_cond_broadcast(&dining->student_done);
  }
  // unlock the cleaner student_mutex
  pthread_mutex_unlock(&dining->cleaner_mutex);
  // here we signal the cleaners semaphore
  sem_post(&dining->cleaners);
}
