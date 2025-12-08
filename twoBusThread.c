#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h> // sleep, usleep

#define NUM_ZONES 5

typedef struct {
    int bus_id;
    int bus_capacity;
    int passenger_num;
    int* zone_people_ptr; // 指向 zone_people 陣列（保留）
} BusTask;

pthread_mutex_t zone_lock[NUM_ZONES];
pthread_mutex_t print_lock;               // 用來保護所有 printf，避免輸出被交錯
int zone_people[NUM_ZONES] = {500,50,100,250,100};

int Min(int a, int b) { return (a < b) ? a : b; }

// printing functions 都會在內部拿 print_lock，確保一個 block 的輸出不會被打斷
void print_start_pick_up_msg(BusTask* t) {
    pthread_mutex_lock(&print_lock);
    printf("Escort Team %d ENTERS Zone 0\n", t->bus_id);
    printf("    Zone 0 has %d shoppers to pick up\n", zone_people[0]);
    printf("    Picking up shoppers...\n");
    pthread_mutex_unlock(&print_lock);
}

void print_end_pick_up_msg(BusTask* t) {
    pthread_mutex_lock(&print_lock);
    printf("    Escort Team %d is at Zone 0 and has %d/%d shoppers\n",
           t->bus_id, t->passenger_num, t->bus_capacity);
    printf("    Zone 0 has %d shoppers to pick up\n", zone_people[0]);
    pthread_mutex_unlock(&print_lock);
}

void print_start_drop_off_msg(BusTask* t, int zone_id) {
    pthread_mutex_lock(&print_lock);
    printf("Escort Team %d ENTERS Zone %d\n", t->bus_id, zone_id);
    printf("    Zone %d has %d shoppers to drop off\n", zone_id, zone_people[zone_id]);
    printf("    Dropping shoppers off...\n");
    pthread_mutex_unlock(&print_lock);
}

void print_end_drop_off_msg(BusTask* t, int zone_id) {
    pthread_mutex_lock(&print_lock);
    printf("    Escort Team %d is at Zone %d and has %d/%d shoppers\n",
           t->bus_id, zone_id, t->passenger_num, t->bus_capacity);
    printf("    Zone %d has %d shoppers to drop off\n", zone_id, zone_people[zone_id]);
    pthread_mutex_unlock(&print_lock);
}

void* bus_routine(void* arg) {
    BusTask* t = (BusTask*)arg;

    while (1) {
        // ========== PICK UP ==========
        pthread_mutex_lock(&zone_lock[0]);

        if (zone_people[0] == 0) {
            pthread_mutex_unlock(&zone_lock[0]);
            pthread_exit(NULL);
        }

        print_start_pick_up_msg(t);

        int can_take = t->bus_capacity - t->passenger_num;
        int take = Min(zone_people[0], can_take);
        zone_people[0] -= take;
        t->passenger_num += take;

        print_end_pick_up_msg(t);

        pthread_mutex_unlock(&zone_lock[0]);

        // 為了讓另一個 thread 有機會執行（觀察交錯），加短暫延遲（可調或移除）
        usleep(100000); // 0.1s，非必要但有助於看到多執行緒行為

        // ========== DROP OFF ==========
        for (int i = 1; i < NUM_ZONES; i++) {
            pthread_mutex_lock(&zone_lock[i]);

            if (zone_people[i] > 0 && t->passenger_num > 0) {
                print_start_drop_off_msg(t, i);

                int drop = Min(t->passenger_num, zone_people[i]);
                zone_people[i] -= drop;
                t->passenger_num -= drop;

                print_end_drop_off_msg(t, i);

                pthread_mutex_unlock(&zone_lock[i]);

                // 放完就回到下一輪 pick up
                break;
            }

            pthread_mutex_unlock(&zone_lock[i]);
        }

        // 小睡一下讓其他 thread 有機會搶 zone0（可選）
        usleep(50000);
    }

    return NULL;
}

int main(void) {
    pthread_t bus0, bus1;

    BusTask task1 = { .bus_id = 0, .bus_capacity = 100, .passenger_num = 0, .zone_people_ptr = zone_people };
    BusTask task2 = { .bus_id = 1, .bus_capacity = 50,  .passenger_num = 0, .zone_people_ptr = zone_people };

    // init locks
    for (int i = 0; i < NUM_ZONES; i++) pthread_mutex_init(&zone_lock[i], NULL);
    pthread_mutex_init(&print_lock, NULL);

    // create threads
    pthread_create(&bus0, NULL, bus_routine, &task1);
    pthread_create(&bus1, NULL, bus_routine, &task2);

    // wait
    pthread_join(bus0, NULL);
    pthread_join(bus1, NULL);

    // destroy locks
    for (int i = 0; i < NUM_ZONES; i++) pthread_mutex_destroy(&zone_lock[i]);
    pthread_mutex_destroy(&print_lock);

    printf("All shoppers have been escorted. Program exiting.\n");
    return 0;
}
