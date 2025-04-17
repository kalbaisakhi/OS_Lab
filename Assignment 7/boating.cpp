#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <unistd.h>
#include <ctime>
using namespace std;
typedef struct {
    int value;
    pthread_mutex_t mtx;
    pthread_cond_t cv;
} semaphore;
int rand_range(int low, int high) {
    return low + rand() % (high - low + 1);
}

void P(semaphore *s) {
    pthread_mutex_lock(&s->mtx);
    s->value--;
    if (s->value < 0) {
        pthread_cond_wait(&s->cv, &s->mtx);
    }
    pthread_mutex_unlock(&s->mtx);
}

void V(semaphore *s) {
    pthread_mutex_lock(&s->mtx);
    s->value++;
    if (s->value <= 0) {
        pthread_cond_signal(&s->cv);
    }
    pthread_mutex_unlock(&s->mtx);
}
int m, n;
semaphore boat_sem = {0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};
semaphore rider_sem = {0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};
pthread_mutex_t bmtx = PTHREAD_MUTEX_INITIALIZER;
int *BA;
int *BC;
int *BT;
pthread_barrier_t *BB;
int *ride_done;
pthread_cond_t *ride_done_cv;
pthread_mutex_t done_mtx = PTHREAD_MUTEX_INITIALIZER;
int visitors_remaining;
pthread_mutex_t visitors_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t print_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t eos;
int last_active_boat = 0;
pthread_mutex_t last_boat_mtx = PTHREAD_MUTEX_INITIALIZER;
int all_visitors_done = 0;
pthread_mutex_t done_flag_mtx = PTHREAD_MUTEX_INITIALIZER;

void *boat_thread(void *arg) {
    int boat_id = *(int *)arg;
    delete (int *)arg;
    int displayID = boat_id + 1;
    pthread_mutex_lock(&print_mtx);
    cout << "Boat " << displayID << " Ready" << endl;
    pthread_mutex_unlock(&print_mtx);
    pthread_mutex_lock(&bmtx);
    BA[boat_id] = 1;
    pthread_mutex_unlock(&bmtx);
    while (1) {
        pthread_mutex_lock(&done_flag_mtx);
        if (all_visitors_done) {
            pthread_mutex_unlock(&done_flag_mtx);
            pthread_mutex_lock(&last_boat_mtx);
            bool is_last_boat = (boat_id == last_active_boat);
            pthread_mutex_unlock(&last_boat_mtx);
            if (is_last_boat) {
                pthread_barrier_wait(&eos);
            }
            break;
        }
        pthread_mutex_unlock(&done_flag_mtx);
        V(&rider_sem);
        P(&boat_sem);
        pthread_mutex_lock(&done_flag_mtx);
        bool should_terminate = all_visitors_done;
        pthread_mutex_unlock(&done_flag_mtx);
        if (should_terminate) {
            pthread_mutex_lock(&last_boat_mtx);
            bool is_last_boat = (boat_id == last_active_boat);
            pthread_mutex_unlock(&last_boat_mtx);
            if (is_last_boat) {
                pthread_barrier_wait(&eos);
            }
            break;
        }
        pthread_mutex_lock(&visitors_mtx);
        bool no_visitors_left = (visitors_remaining == 0);
        pthread_mutex_unlock(&visitors_mtx);
        if (no_visitors_left) {
            pthread_mutex_lock(&last_boat_mtx);
            bool is_last_boat = (boat_id == last_active_boat);
            pthread_mutex_unlock(&last_boat_mtx);
            if (is_last_boat) {
                pthread_barrier_wait(&eos);
            }
            break;
        }
        pthread_barrier_wait(&BB[boat_id]);
        pthread_mutex_lock(&bmtx);
        BA[boat_id] = 0;
        int visitor_id = BC[boat_id];
        int ride_time = BT[boat_id];
        pthread_mutex_unlock(&bmtx);
        pthread_mutex_lock(&last_boat_mtx);
        last_active_boat = boat_id;
        pthread_mutex_unlock(&last_boat_mtx);
        pthread_mutex_lock(&print_mtx);
        cout << "Boat " << displayID << " Start of ride for visitor " << visitor_id + 1 << endl;
        pthread_mutex_unlock(&print_mtx);
        usleep(ride_time * 100000);
        pthread_mutex_lock(&print_mtx);
        cout << "Boat " << displayID << " End of ride for visitor " << visitor_id + 1 
             << " (ride time = " << ride_time << ")" << endl;
        pthread_mutex_unlock(&print_mtx);
        pthread_mutex_lock(&done_mtx);
        ride_done[visitor_id] = 1;
        pthread_cond_signal(&ride_done_cv[visitor_id]);
        pthread_mutex_unlock(&done_mtx);
        pthread_mutex_lock(&bmtx);
        BA[boat_id] = 1;
        BC[boat_id] = -1;
        pthread_mutex_unlock(&bmtx);
    }
    return NULL;
}

void *visitor_thread(void *arg) {
    int vid = *(int *)arg;
    delete (int *)arg;
    int displayVID = vid + 1;
    int vtime = rand_range(30, 120);
    int rtime = rand_range(15, 60);
    pthread_mutex_lock(&print_mtx);
    cout << "Visitor " << displayVID << " Starts sightseeing for " << vtime << " minutes" << endl;
    pthread_mutex_unlock(&print_mtx);
    usleep(vtime * 100000);
    pthread_mutex_lock(&print_mtx);
    cout << "Visitor " << displayVID << " Ready to ride a boat (ride time = " << rtime << ")" << endl;
    pthread_mutex_unlock(&print_mtx);
    V(&boat_sem);
    P(&rider_sem);
    int chosenBoat = -1;
    while (1) {
        pthread_mutex_lock(&bmtx);
        for (int i = 0; i < m; i++) {
            if (BA[i] == 1 && BC[i] == -1) {
                chosenBoat = i;
                BC[i] = vid;
                BT[i] = rtime;
                break;
            }
        }
        pthread_mutex_unlock(&bmtx);
        if (chosenBoat != -1) {
            break;
        }
        usleep(10000);
    }
    pthread_mutex_lock(&print_mtx);
    cout << "Visitor " << displayVID << " Finds boat " << chosenBoat + 1 << endl;
    pthread_mutex_unlock(&print_mtx);
    pthread_barrier_wait(&BB[chosenBoat]);
    pthread_mutex_lock(&done_mtx);
    while (ride_done[vid] == 0) {
        pthread_cond_wait(&ride_done_cv[vid], &done_mtx);
    }
    pthread_mutex_unlock(&done_mtx);
    pthread_mutex_lock(&print_mtx);
    cout << "Visitor " << displayVID << " Leaving" << endl;
    pthread_mutex_unlock(&print_mtx);
    pthread_mutex_lock(&visitors_mtx);
    visitors_remaining--;
    pthread_mutex_unlock(&visitors_mtx);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <boats 5-10> <visitors 20-100>" << endl;
        exit(1);
    }
    m = atoi(argv[1]);
    n = atoi(argv[2]);
    if (m < 5 || m > 10 || n < 20 || n > 100) {
        cerr << "Invalid inputs" << endl;
        exit(1);
    }
    visitors_remaining = n;
    srand(time(NULL));
    BA = new int[m];
    BC = new int[m];
    BT = new int[m];
    BB = new pthread_barrier_t[m];
    for (int i = 0; i < m; i++) {
        BA[i] = 1;
        BC[i] = -1;
        BT[i] = 0;
        pthread_barrier_init(&BB[i], NULL, 2);
    }
    ride_done = new int[n];
    ride_done_cv = new pthread_cond_t[n];
    for (int i = 0; i < n; i++) {
        ride_done[i] = 0;
        pthread_cond_init(&ride_done_cv[i], NULL);
    }
    pthread_barrier_init(&eos, NULL, 2);
    pthread_t *boatThreads = new pthread_t[m];
    pthread_t *visitorThreads = new pthread_t[n];
    for (int i = 0; i < m; i++) {
        int *bid = new int;
        *bid = i;
        pthread_create(&boatThreads[i], NULL, boat_thread, bid);
    }
    for (int i = 0; i < n; i++) {
        int *vid = new int;
        *vid = i;
        pthread_create(&visitorThreads[i], NULL, visitor_thread, vid);
    }
    for (int i = 0; i < n; i++) {
        pthread_join(visitorThreads[i], NULL);
    }
    pthread_mutex_lock(&done_flag_mtx);
    all_visitors_done = 1;
    pthread_mutex_unlock(&done_flag_mtx);
    for (int i = 0; i < m; i++) {
        V(&boat_sem);
    }
    pthread_barrier_wait(&eos);
    for (int i = 0; i < m; i++) {
        pthread_join(boatThreads[i], NULL);
    }
    for (int i = 0; i < m; i++) {
        pthread_barrier_destroy(&BB[i]);
    }
    delete[] BB;
    delete[] BA;
    delete[] BC;
    delete[] BT;
    for (int i = 0; i < n; i++) {
        pthread_cond_destroy(&ride_done_cv[i]);
    }
    delete[] ride_done_cv;
    delete[] ride_done;
    delete[] boatThreads;
    delete[] visitorThreads;
    pthread_mutex_destroy(&bmtx);
    pthread_mutex_destroy(&done_mtx);
    pthread_mutex_destroy(&visitors_mtx);
    pthread_mutex_destroy(&print_mtx);
    pthread_barrier_destroy(&eos);
    pthread_mutex_destroy(&last_boat_mtx);
    pthread_mutex_destroy(&done_flag_mtx);
    return 0;
}