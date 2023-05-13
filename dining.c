#include "dining.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct dining {
    pthread_mutex_t mutex;
    pthread_cond_t cond_cleaning_done;
    sem_t sem_students;
    bool cleaning;
    int capacity;
    int num_students;
    int num_cleaners;
} dining_t;

dining_t *dining_init(int capacity) {
    dining_t *dining = malloc(sizeof(dining_t));
    pthread_mutex_init(&dining->mutex, NULL);
    pthread_cond_init(&dining->cond_cleaning_done, NULL);
    sem_init(&dining->sem_students, 0, capacity);
    dining->cleaning = false;
    dining->capacity = capacity;
    dining->num_students = 0;
    dining->num_cleaners = 0; // initialize the number of cleaners to zero
    return dining;
}

void dining_destroy(dining_t **dining_ptr) {
    dining_t *dining = *dining_ptr;

    pthread_mutex_lock(&dining->mutex);
    while (dining->cleaning || dining->num_students > 0 || dining->num_cleaners > 0) {
        pthread_cond_wait(&dining->cond_cleaning_done, &dining->mutex);
    }
    pthread_mutex_unlock(&dining->mutex);

    pthread_mutex_destroy(&dining->mutex);
    pthread_cond_destroy(&dining->cond_cleaning_done);
    sem_destroy(&dining->sem_students);
    free(dining);
    *dining_ptr = NULL;
}

void dining_student_enter(dining_t *dining) {
    sem_wait(&dining->sem_students);
    pthread_mutex_lock(&dining->mutex);
    while(dining->num_cleaners > 0 || dining->num_students == dining->capacity)
    {
        pthread_cond_wait(&dining->cond_cleaning_done, &dining->mutex);
    }
    dining->num_students++;
    pthread_mutex_unlock(&dining->mutex);
}

void dining_student_leave(dining_t *dining) {
    pthread_mutex_lock(&dining->mutex);
    dining->num_students--;
    sem_post(&dining->sem_students);
    if (dining->num_students == 0 && dining->cleaning) {
        pthread_cond_broadcast(&dining->cond_cleaning_done);
    }
    pthread_mutex_unlock(&dining->mutex);
}

void dining_cleaning_enter(dining_t *dining) {
    pthread_mutex_lock(&dining->mutex);
    dining->num_cleaners++; // increment the number of cleaners
    dining->cleaning = true;
    while (dining->num_students > 0) {
        pthread_cond_wait(&dining->cond_cleaning_done, &dining->mutex);
    }
}

void dining_cleaning_leave(dining_t *dining) {
    dining->num_cleaners--; // decrement the number of cleaners
    if (dining->num_cleaners == 0 && dining->cleaning) { // check if all cleaners have left
        dining->cleaning = false;
        pthread_cond_broadcast(&dining->cond_cleaning_done);
    }
    pthread_mutex_unlock(&dining->mutex);
}

