#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

#define NUM_ZONES 5

typedef struct {
    int bus_id;
    int bus_capacity;
    int passenger_num;
    int* zone_people_ptr; //指向zone_people
} BusTask;

pthread_mutex_t zone_lock[NUM_ZONES];
int zone_people[NUM_ZONES] = {500, 50, 100, 250, 100}; //1~4區分別有：50、100、250、100筆資料要到

int Min(int a, int b) {
    return (a<b)? a:b;
}

void print_start_pick_up_msg(BusTask* t) {
    printf("Escort Team %d ENTERS Zone 0\n", t->bus_id);
    printf("    Zone 0 has %d shoppers to pick up\n", zone_people[0]);
    printf("    Picking up shoppers...\n");
}

void print_end_pick_up_msg(BusTask* t) {
    printf("    Escort Team %d is at Zone 0 and has %d/%d shoppers\n", t->bus_id, t->passenger_num, t->bus_capacity);
    printf("    Zone 0 has %d shoppers to pick up\n", zone_people[0]);
}

void print_start_drop_off_msg(BusTask* t, int zone_id) {
    printf("Escort Team %d ENTERS Zone %d\n", t->bus_id, zone_id);
    printf("    Zone %d has %d shoppers to drop off\n", zone_id, t->passenger_num);
    printf("    Dropping shoppers off...\n");
}

void print_end_drop_off_msg(BusTask* t, int zone_id) {
    printf("    Escort Team %d is at Zone %d and has %d/%d shoppers\n", t->bus_id, zone_id, t->passenger_num, t->bus_capacity);
    printf("    Zone %d has %d shoppers to drop off\n", zone_id, zone_people[zone_id]);
}

void* bus_routine(void* arg){
    BusTask* t = (BusTask*)arg;
    while(1){
        // =========== 去 zone 0 pick up ================
        pthread_mutex_lock(&zone_lock[0]);
        
        if (zone_people[0] == 0) { // zone0 都載完了
            pthread_mutex_unlock(&zone_lock[0]);
            pthread_exit(NULL);
        }

        print_start_pick_up_msg(t); // 開始載人

        int pick_up = Min(zone_people[0], t->bus_capacity); // 盡量載滿
        zone_people[0] -= pick_up;
        t->passenger_num += pick_up; // 更新車上人數

        print_end_pick_up_msg(t); // 結束載人
        
        pthread_mutex_unlock(&zone_lock[0]);
        // =========== 去 zone 0 pick up =================


        // =========== 去 zone 1~4 選一個空的 drop off =====
        for (int i = 1; i < NUM_ZONES; i++) {
            pthread_mutex_lock(&zone_lock[i]);
            if (zone_people[i] != 0 && t->passenger_num > 0) {
                print_start_drop_off_msg(t, i); // 開始放乘客

                int drop_off = Min(t->passenger_num, zone_people[i]);
                zone_people[i] -= drop_off; // 放乘客
                t->passenger_num -= drop_off;// 更新車上乘客人數

                print_end_drop_off_msg(t, i); // 放完乘客

                pthread_mutex_unlock(&zone_lock[i]);

                break;
            }
            pthread_mutex_unlock(&zone_lock[i]);
        }
    }
}

int main(const int argc, const char * argv[]){
    pthread_t bus0;
    pthread_t bus1;

    BusTask task1 = { .bus_id = 0, .bus_capacity = 100, .zone_people_ptr = zone_people};
    BusTask task2 = { .bus_id = 1, .bus_capacity = 50 , .zone_people_ptr = zone_people};

    // 初始化鎖
    for(int i = 0; i < NUM_ZONES; i++){
        pthread_mutex_init(&zone_lock[i], NULL);
    }

    // 建立 thread (每台巴士一個thread)
    pthread_create(&bus0, NULL, bus_routine, &task1); // create 需要函數指標(要做的工作)、需要的參數(想傳給函數的資訊)
    pthread_create(&bus1, NULL, bus_routine, &task2); // 20251202 先寫死task，繼續寫routine 的程式

    pthread_join(bus0, NULL); //等待 thread 完成工作，然後回收資源
    pthread_join(bus1, NULL);

    // 釋放不用的mutex lock
    for(int i = 0; i < NUM_ZONES; i++){
        pthread_mutex_destroy(&zone_lock[i]);
    }
    
    return 0;
}