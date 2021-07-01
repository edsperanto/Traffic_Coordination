#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

// constants
const bool NORTH = 1;
const bool SOUTH = 0;
const int TOO_MANY_CARS = 10;
const int TIME_LEN = 10;
const int TIME_PASS = 2;
const int TIME_BETWEEN_BURSTS = 20;
const int TOTAL_PROB = 100;
const int PROB_NEXT_CAR = 80;
const int PROB_NO_CAR = TOTAL_PROB - PROB_NEXT_CAR;

// structs
struct car {
    bool direction; // north 1 south 0
    struct tm *arrival_time;
    struct tm *start_time;
    struct tm *end_time;
    struct car *next;
};

// global variables
int carID = 1;
bool curr_side = 1;  // north 1 south 0
bool end_of_day = false;
int total_cars;
struct car *north = NULL;
struct car *south = NULL;
FILE *flagFile;
FILE *carFile;

// mutexes and semaphores
pthread_mutex_t lane_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t car_queues_mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t car_queues_count;

// function declarations
int pthread_sleep(int seconds);
void *north_traffic(void *arg);
void *south_traffic(void *arg);
void *flag_person(void *arg);
void *passing_car(void *arg);

// main function
int main(int argc, char *argv[]) {
    
    // get command line argument
    if(argc != 2) {
        perror("Usage: ./coord_traffic <number_of_cars>\n");
        return -1;
    }
    total_cars = atoi(argv[1]);
    
    // initialization
    srand(time(NULL));
    sem_init(&car_queues_count, 0, 0);
    if((flagFile = fopen("flagperson.log", "w")) == NULL) {
        perror("Cannot open flagperson.log\n");
        return -1;
    };
    if((carFile = fopen("car.log", "w")) == NULL) {
        perror("Cannot open car.log\n");
        return -1;
    };
    fprintf(flagFile, "%s\n", "Time       State");
    fprintf(carFile, "%s\n", "carID direction arrival-time start-time   end-time");
    
    // begin work day (start threads)
    pthread_t north_tid, south_tid, flag_tid;
    if(pthread_create(&flag_tid, NULL, flag_person, NULL) == -1) {
        perror("Cannot create flag person thread.\n");
        return -1;
    }
    if(pthread_create(&north_tid, NULL, north_traffic, NULL) == -1) {
        perror("Cannot create north traffic thread.\n");
        return -1;
    }
    if(pthread_create(&south_tid, NULL, south_traffic, NULL) == -1) {
        perror("Cannot create south traffic thread.\n");
        return -1;
    }
    
    // end of work day (join threads and clean up)
    if(pthread_join(north_tid, NULL)) {
        perror("Cannot join north traffic thread.\n");
        return -1;
    }
    if(pthread_join(south_tid, NULL)) {
        perror("Cannot join south traffic thread.\n");
        return -1;
    }
    if(pthread_join(flag_tid, NULL)) {
        perror("Cannot join flag traffic thread.\n");
        return -1;
    }
    pthread_mutex_destroy(&lane_mutex);
    pthread_mutex_destroy(&car_queues_mutex);
    sem_destroy(&car_queues_count);
    fclose(flagFile);
    fclose(carFile);
    return 0;
    
}

// custom sleep function in seconds
int pthread_sleep(int seconds) {
    // variables
    pthread_mutex_t mutex;
    pthread_cond_t conditionvar;
    struct timespec timetoexpire;
    // sleep for x seconds
    if(pthread_mutex_init(&mutex, NULL)) return -1;
    if(pthread_cond_init(&conditionvar, NULL)) return -1;
    timetoexpire.tv_sec = (unsigned int)time(NULL) + seconds;
    timetoexpire.tv_nsec = 0;
    return pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
}

// north traffic (producer)
void *north_traffic(void *arg) {
    while(1) {
        pthread_mutex_lock(&car_queues_mutex);
        // if there are cars left
        if(total_cars > 0) {
            // create new car
            int num_cars = 1;
            time_t now = time(NULL);
            struct car *car_ptr = north;
            struct car *new_car_ptr = malloc(sizeof *new_car_ptr);
            new_car_ptr->direction = NORTH;
            new_car_ptr->arrival_time = localtime(&now);
            new_car_ptr->next = NULL;
            // add new car to end of queue
            if(car_ptr == NULL) {  // first car
                north = new_car_ptr;
            } else {  // subsequent cars
                while(car_ptr->next != NULL) {
                    car_ptr = car_ptr->next;
                    num_cars++;
                }
                car_ptr->next = new_car_ptr;
                if(++num_cars > TOO_MANY_CARS) curr_side = NORTH;
            }
            sem_post(&car_queues_count);
            total_cars--;
        }
        pthread_mutex_unlock(&car_queues_mutex);
        // if in between bursts
        if(rand() % TOTAL_PROB < PROB_NO_CAR) pthread_sleep(TIME_BETWEEN_BURSTS);
        // if there are no more cars left
        if(total_cars <= 0) return NULL;
    }
}
 
// south traffic (producer)
void *south_traffic(void *arg) {
    while(1) {
        pthread_mutex_lock(&car_queues_mutex);
        // if there are cars left
        if(total_cars > 0) {
            // create new car
            int num_cars = 1;
            time_t now = time(NULL);
            struct car *car_ptr = south;
            struct car *new_car_ptr = malloc(sizeof *new_car_ptr);
            new_car_ptr->direction = SOUTH;
            new_car_ptr->arrival_time = localtime(&now);
            new_car_ptr->next = NULL;
            // add new car to end of queue
            if(car_ptr == NULL) {  // first car
                south = new_car_ptr;
            } else {  // subsequent cars
                while(car_ptr->next != NULL) {
                    car_ptr = car_ptr->next;
                    num_cars++;
                }
                car_ptr->next = new_car_ptr;
                if(++num_cars > TOO_MANY_CARS) curr_side = SOUTH;
            }
            sem_post(&car_queues_count);
            total_cars--;
        }
        pthread_mutex_unlock(&car_queues_mutex);
        // if in between bursts
        if(rand() % TOTAL_PROB < PROB_NO_CAR) pthread_sleep(TIME_BETWEEN_BURSTS);
        // if there are no more cars left
        if(total_cars <= 0) return NULL;
    }

}

// flag person (consumer)
void *flag_person(void *arg) {
    bool asleep = true;
    time_t now;
    char t_str[TIME_LEN];
    // start work day
    while(total_cars > 0 || north != NULL || south != NULL) {
        sem_wait(&car_queues_count);
        // get next car
        pthread_mutex_lock(&car_queues_mutex);
        struct car *next_car = NULL;
        // if there are cars
        if(north != NULL || south != NULL) {
            if(asleep) {
                asleep = false;
                now = time(NULL);
                strftime(t_str, TIME_LEN, "%T", localtime(&now));
                fprintf(flagFile, "%-15s%-10s\n", t_str, "woken-up");
            }
            if((curr_side == NORTH && north != NULL) || south == NULL) {
                if(south == NULL) curr_side = NORTH;
                next_car = north;
                north = north->next;
            } else if((curr_side == SOUTH && south != NULL) || north == NULL) {
                if(north == NULL) curr_side = SOUTH;
                next_car = south;
                south = south->next;
            }
        }
        pthread_mutex_unlock(&car_queues_mutex);
        // if there are no more cars
        if(north == NULL && south == NULL && !asleep) {
            asleep = true;
            now = time(NULL);
            strftime(t_str, TIME_LEN, "%T", localtime(&now));
            fprintf(flagFile, "%-15s%-10s\n", t_str, "sleep");
        }
        // let car pass lane
        pthread_t car_tid;
        if(next_car != NULL && pthread_create(&car_tid, NULL, passing_car, (void *)next_car) == -1) {
            perror("Cannot create passing car thread.\n");
            return NULL;
        }
        // last car
        bool last_car = total_cars == 0 && north == NULL && south == NULL;
        if(last_car) {
            if(pthread_join(car_tid, NULL)) {                
                perror("Cannot join last car thread.\n");
                return NULL;
            }
        } else if(pthread_detach(car_tid)) {
            perror("Cannot detach car thread.\n");
            return NULL;
        }
    }
    // end work day
    return NULL;
}

// passing car
void *passing_car(void *arg) {
    // get car info
    time_t now;
    struct car *car_ptr = (struct car *)arg;
    char at_str[TIME_LEN], st_str[TIME_LEN], et_str[TIME_LEN];
    strftime(at_str, TIME_LEN, "%T", car_ptr->arrival_time);
    // start passing lane
    pthread_mutex_lock(&lane_mutex);
    now = time(NULL);
    car_ptr->start_time = localtime(&now);
    strftime(st_str, TIME_LEN, "%T", car_ptr->start_time);
    pthread_sleep(TIME_PASS);
    now = time(NULL);
    car_ptr->end_time = localtime(&now);
    strftime(et_str, TIME_LEN, "%T", car_ptr->end_time);
    pthread_mutex_unlock(&lane_mutex);
    // log car trip
    char dir = car_ptr->direction ? 'N' : 'S';
    fprintf(carFile, "%-6d%-10c%-13s%-13s%-13s\n", carID++, dir, at_str, st_str, et_str);
    free(car_ptr);
    return NULL;
}