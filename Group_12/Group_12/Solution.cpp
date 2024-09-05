#include<iostream>
#include<stdlib.h>
#include<pthread.h>
#include<time.h>
#include<unistd.h>
#include<bits/stdc++.h>
#include<dispatch/dispatch.h> // GCD for semaphore
using namespace std;

int s, e, MAX, T, c;

dispatch_semaphore_t max_query_semaphore; // GCD semaphore
pthread_mutex_t shared_table_mutex;
set<vector<int>> shared_table;
vector<int> events;

bool check_for_read_availability(int event_number, int query_type, int thread_id){
    pthread_mutex_lock(&shared_table_mutex);    

    if(shared_table.size() == MAX){
        printf("Thread:%d active queries limit exceeded for read.\n", thread_id);
        return false;
    }
    
    bool can_make_query = true;

    for(auto q: shared_table){
        if(q[0] == event_number && q[1] != 0){
            can_make_query = false;
            printf("Thread:%d conflicting entry already present in shared table for read for event %d.\n", thread_id, event_number);
            break;
        }
    }
    if(can_make_query){
        shared_table.insert({event_number, query_type, thread_id});
    }

    pthread_mutex_unlock(&shared_table_mutex);
    return can_make_query;
}

bool check_for_write_availability(int event_number, int query_type, int thread_id){
    pthread_mutex_lock(&shared_table_mutex);

    if(shared_table.size() == MAX) {
        printf("Thread:%d active queries limit exceeded for write.\n", thread_id);
        return false;
    }

    bool can_make_query = true;

    for(auto q: shared_table){
        if(q[0] == event_number){
            can_make_query = false;
            printf("Thread:%d conflicting entry already present in shared table for write for event %d.\n", thread_id, event_number);
            break;
        }
    }
    if(can_make_query){
        shared_table.insert({event_number, query_type, thread_id});
    }

    pthread_mutex_unlock(&shared_table_mutex);
    return can_make_query;
}

void remove_entry_in_shared_table(int event_number, int query_type, int thread_id){
    pthread_mutex_lock(&shared_table_mutex);
    
    shared_table.erase({event_number, query_type, thread_id});
    
    pthread_mutex_unlock(&shared_table_mutex);
}

void book_ticket(int event_no, int thread_id, vector<vector<int>> &booked_seats_details){
    int k = rand()%6 + 5;
    int available_seats = events[event_no];
    if(k > available_seats){
        printf("Thread:%d can't Book Ticket: Only %d seats available for event %d (requested %d) \n", thread_id, available_seats, event_no, k);
        return;
    }
    events[event_no] -= k;
    booked_seats_details.push_back({thread_id, event_no, k});
    printf("Thread:%d Successfully booked %d seats for event %d\n", thread_id, k, event_no);
}

void cancel_ticket(int thread_id, vector<vector<int>> &booked_seats_details){
    if(booked_seats_details.size() == 0){
        printf("Thread:%d No existing booking available!!!\n", thread_id);
        return;
    }
    int index = rand() % booked_seats_details.size();
    events[booked_seats_details[index][1]] += booked_seats_details[index][2];
    printf("Thread:%d Successfully cancelled %d seats for event %d\n", booked_seats_details[index][0], booked_seats_details[index][2], booked_seats_details[index][1]);
    booked_seats_details.erase(booked_seats_details.begin() + index);
}

void main_functionality(int event_number, int query_type, int thread_id, vector<vector<int>> &booked_seats_details){
    if(query_type == 0){
        printf("Thread:%d available seats %d for event %d\n", thread_id, events[event_number], event_number);
    } else if (query_type == 1){
        book_ticket(event_number, thread_id, booked_seats_details);
    } else {
        cancel_ticket(thread_id, booked_seats_details);
    }
}

void make_query(int thread_id, vector<vector<int>> &booked_seats_details){
    int event_number, query_type;
    event_number = rand() % e;
    query_type = rand() % 3;

    printf("Thread:%d made query %d for event %d\n", thread_id, query_type, event_number);
    
    bool can_make_query = false;
    if(query_type == 0){
        can_make_query = check_for_read_availability(event_number, query_type, thread_id);
    } else {
        can_make_query = check_for_write_availability(event_number, query_type, thread_id);
    }
    sleep((rand() % 5) + 1);
    if(can_make_query){
        main_functionality(event_number, query_type, thread_id, booked_seats_details);
        remove_entry_in_shared_table(event_number, query_type, thread_id);
    }
}

void *thread_function(void *t_id) {
    time_t start=time(NULL), secs=T;
    int *thread_id = (int *)t_id;

    srand((unsigned)time(NULL) + *thread_id + 1);

    vector<vector<int>> booked_seats_details;

    while(time(NULL)-start < secs){
        printf("Thread:%d Waiting in query queue\n", *thread_id);
        dispatch_semaphore_wait(max_query_semaphore, DISPATCH_TIME_FOREVER);
        
        make_query(*thread_id, booked_seats_details);

        dispatch_semaphore_signal(max_query_semaphore);
        sleep((rand() % 4) + 1);
    }

    free(t_id);
    pthread_exit(NULL);
}

int main() {

    printf("Enter the number of events: ");
    scanf("%d", &e);
    printf("Enter the capacity of auditorium: ");
    scanf("%d", &c);
    printf("Enter the number of worker threads: ");
    scanf("%d", &s);
    printf("Enter the maximum number of concurrent active queries: ");
    scanf("%d", &MAX);
    printf("Enter the total running time in seconds: ");
    scanf("%d", &T);

    events.resize(e, c);

    pthread_t threads[s];
    srand(time(NULL));

    max_query_semaphore = dispatch_semaphore_create(MAX);
    pthread_mutex_init(&shared_table_mutex, NULL);
    for(int i=0; i<s; i++) {
        int *thread_id = (int*)malloc(sizeof(int));
        *thread_id = i;
        if(pthread_create(&threads[i], NULL, thread_function, thread_id)) {
            printf("Error while creating threads.\n");
            exit(-1);
        }
    }

    for (int i = 0; i < s; i++) {
        pthread_join(threads[i], NULL);
    }
    printf("All threads have finished.\n");

    printf("Booked seats for Events\n");
    for(int i=0; i<events.size(); i++){
        printf("Event %d, booked seats %d\n", i, c-events[i]);
    }

    // No need to destroy GCD semaphore
    pthread_mutex_destroy(&shared_table_mutex);

    return 0;
}
