#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <string>
#include <cstring>
#include <sstream>
#include <pthread.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <algorithm>

using namespace std;

int m, n;
int *available;
int **allocation;
int **maximum;
int **need;
bool *thread_exit;
int num_exited = 0;

int avail_id, req_id, req_thread_id, sync_id, to_exit_id;
int *req;
int *req_thread;
bool *thread_sync;
bool *to_exit;

pthread_barrier_t BOS;
pthread_barrier_t REQB;
pthread_barrier_t *ACKB;
pthread_mutex_t rmtx;
pthread_mutex_t pmtx;
pthread_mutex_t *cmtx;
pthread_cond_t *cv;

queue<pair<int, vector<int>>> Q;

void *process_function(void *arg);
void *master_function(void *arg);
bool check_safety(int thread_id, vector<int> &request);
void init_resources();
void clean_resources();
void process_pending_requests();
void print_waiting_threads();

int main() {
    init_resources();
    pthread_t threads[n + 1];
    
    if (pthread_create(&threads[0], NULL, master_function, NULL) != 0) {
        cerr << "Error creating master thread" << endl;
        exit(1);
    }
    
    for (int i = 0; i < n; i++) {
        int *thread_id = new int(i);
        if (pthread_create(&threads[i + 1], NULL, process_function, (void *)thread_id) != 0) {
            cerr << "Error creating thread " << i << endl;
            exit(1);
        }
    }
    
    for (int i = 1; i <= n; i++) {
        pthread_join(threads[i], NULL);
    }
    
    pthread_join(threads[0], NULL);
    clean_resources();
    return 0;
}

void init_resources() {
    ifstream system_file("input/system.txt");
    if (!system_file) {
        cerr << "Error opening system.txt" << endl;
        exit(1);
    }
    system_file >> m >> n;
    avail_id = shmget(IPC_PRIVATE, m * sizeof(int), IPC_CREAT | 0666);
    req_id = shmget(IPC_PRIVATE, m * sizeof(int), IPC_CREAT | 0666);
    req_thread_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    sync_id = shmget(IPC_PRIVATE, n * sizeof(bool), IPC_CREAT | 0666);
    to_exit_id = shmget(IPC_PRIVATE, sizeof(bool), IPC_CREAT | 0666);
    available = (int *)shmat(avail_id, NULL, 0);
    req = (int *)shmat(req_id, NULL, 0);
    req_thread = (int *)shmat(req_thread_id, NULL, 0);
    thread_sync = (bool *)shmat(sync_id, NULL, 0);
    to_exit = (bool *)shmat(to_exit_id, NULL, 0);
    *to_exit = false;
    for (int i = 0; i < n; i++) {
        thread_sync[i] = true;
    }
    for (int i = 0; i < m; i++) {
        system_file >> available[i];
    }
    allocation = new int*[n];
    maximum = new int*[n];
    need = new int*[n];
    thread_exit = new bool[n];
    for (int i = 0; i < n; i++) {
        allocation[i] = new int[m];
        maximum[i] = new int[m];
        need[i] = new int[m];
        thread_exit[i] = false;
        for (int j = 0; j < m; j++) {
            allocation[i][j] = 0;
        }
    }
    for (int i = 0; i < n; i++) {
        string filename = "input/thread";
        filename += (i < 10) ? "0" : "";
        filename += to_string(i) + ".txt";
        ifstream thread_file(filename);
        if (!thread_file) {
            cerr << "Error opening " << filename << endl;
            exit(1);
        }
        for (int j = 0; j < m; j++) {
			thread_file >> maximum[i][j];
			need[i][j] = maximum[i][j];
        }
        thread_file.close();
    }
    pthread_barrier_init(&BOS, NULL, n + 1);
    pthread_barrier_init(&REQB, NULL, 2);
    ACKB = new pthread_barrier_t[n];
    for (int i = 0; i < n; i++) {
        pthread_barrier_init(&ACKB[i], NULL, 2);
    }
    pthread_mutex_init(&rmtx, NULL);
    pthread_mutex_init(&pmtx, NULL);
    cmtx = new pthread_mutex_t[n];
    cv = new pthread_cond_t[n];
    for (int i = 0; i < n; i++) {
        pthread_mutex_init(&cmtx[i], NULL);
        pthread_cond_init(&cv[i], NULL);
    }
}

void clean_resources() {
	shmdt(available);
	shmdt(req);
	shmdt(req_thread);
	shmdt(thread_sync);
	shmdt(to_exit);
	shmctl(avail_id, IPC_RMID, NULL);
	shmctl(req_id, IPC_RMID, NULL);
	shmctl(req_thread_id, IPC_RMID, NULL);
	shmctl(sync_id, IPC_RMID, NULL);
	shmctl(to_exit_id, IPC_RMID, NULL);
	pthread_barrier_destroy(&BOS);
	pthread_barrier_destroy(&REQB);
	for (int i = 0; i < n; i++) {
		pthread_barrier_destroy(&ACKB[i]);
		pthread_mutex_destroy(&cmtx[i]);
		pthread_cond_destroy(&cv[i]);
	}
	pthread_mutex_destroy(&rmtx);
	pthread_mutex_destroy(&pmtx);
	for (int i = 0; i < n; i++) {
		delete[] allocation[i];
		delete[] maximum[i];
		delete[] need[i];
	}
	delete[] allocation;
	delete[] maximum;
	delete[] need;
	delete[] thread_exit;
	delete[] ACKB;
	delete[] cmtx;
	delete[] cv;
}

void *process_function(void *arg) {
    int *tid = (int *)arg;
    int thread_id = *tid;
    delete tid;
    pthread_mutex_lock(&pmtx);
    cout << "\tThread " << thread_id << " born" << endl;
    pthread_mutex_unlock(&pmtx);
    pthread_barrier_wait(&BOS);
    string filename = "input/thread";
    filename += (thread_id < 10) ? "0" : "";
    filename += to_string(thread_id) + ".txt";
    ifstream thread_file(filename);
    if (!thread_file) {
        cerr << "Error opening " << filename << endl;
        exit(1);
    }
    string line;
    getline(thread_file, line);
    while (getline(thread_file, line)) {
        istringstream iss(line);
        int delay;
        char request_type;
        iss >> delay;
        iss >> request_type;
        usleep(delay * 10000);
        if (request_type == 'Q') {
            vector<int> release_request(m, 0);
            for (int j = 0; j < m; j++) {
                release_request[j] = -allocation[thread_id][j];
            }
            pthread_mutex_lock(&rmtx);
            *req_thread = thread_id;
            for (int j = 0; j < m; j++) {
                req[j] = release_request[j];
            }
            pthread_barrier_wait(&REQB);
            pthread_barrier_wait(&ACKB[thread_id]);
            pthread_mutex_unlock(&rmtx);
            pthread_mutex_lock(&pmtx);
            cout << "\tThread " << thread_id << " going to quit" << endl;
            pthread_mutex_unlock(&pmtx);
            break;
        } else if (request_type == 'R') {
            vector<int> request(m);
            bool is_release_only = true;
            for (int j = 0; j < m; j++) {
                iss >> request[j];
                if (request[j] > 0) {
                    is_release_only = false;
                }
            }
            pthread_mutex_lock(&pmtx);
            cout << "\tThread " << thread_id << " sends resource request: type = " 
                 << (is_release_only ? "RELEASE" : "ADDITIONAL") << endl;
            pthread_mutex_unlock(&pmtx);
            pthread_mutex_lock(&rmtx);
            *req_thread = thread_id;
            for (int j = 0; j < m; j++) {
                req[j] = request[j];
            }
            pthread_barrier_wait(&REQB);
            pthread_barrier_wait(&ACKB[thread_id]);
            pthread_mutex_unlock(&rmtx);
            if (!is_release_only) {
                pthread_mutex_lock(&cmtx[thread_id]);
                if (thread_sync[thread_id]) {
                    pthread_cond_wait(&cv[thread_id], &cmtx[thread_id]);
                }
                thread_sync[thread_id] = true;
                pthread_mutex_unlock(&cmtx[thread_id]);
                pthread_mutex_lock(&pmtx);
                cout << "\tThread " << thread_id << " is granted its last resource request" << endl;
                pthread_mutex_unlock(&pmtx);
            } else {
                pthread_mutex_lock(&pmtx);
                cout << "\tThread " << thread_id << " is done with its resource release request" << endl;
                pthread_mutex_unlock(&pmtx);
            }
        }
    }
    thread_file.close();
    return NULL;
}

void *master_function(void *arg) {
    pthread_barrier_wait(&BOS);
    while (true) {
        pthread_barrier_wait(&REQB);
        int thread_id = *req_thread;
        vector<int> request(m);
        for (int i = 0; i < m; i++) {
            request[i] = req[i];
        }
        pthread_barrier_wait(&ACKB[thread_id]);
        bool is_quit = true;
        for (int i = 0; i < m; i++) {
            if (request[i] != -allocation[thread_id][i]) {
                is_quit = false;
                break;
            }
        }
        if (is_quit) {
            pthread_mutex_lock(&pmtx);
            for (int i = 0; i < m; i++) {
                available[i] += allocation[thread_id][i];
                allocation[thread_id][i] = 0;
            }
            thread_exit[thread_id] = true;
            num_exited++;
            cout << "\t\tWaiting threads: ";
            print_waiting_threads();
            cout << endl;
            vector<int> active_threads;
            for (int i = 0; i < n; i++) {
                if (!thread_exit[i]) active_threads.push_back(i);
            }
            if (1) {
                cout << active_threads.size() << " threads left: ";
                for (size_t i = 0; i < active_threads.size(); i++) {
                    cout << active_threads[i];
                    if (i < active_threads.size() - 1) cout << " ";
                }
                cout << endl;
                cout << "Available resources: ";
                for (int i = 0; i < m; i++) {
                    cout << available[i];
                    if (i < m - 1) cout << " ";
                }
                cout << endl;
            }
            pthread_mutex_unlock(&pmtx);
            if (num_exited == n) {
                *to_exit = true;
                break;
            }
            process_pending_requests();
        } else {
            bool is_release_only = true;
            for (int i = 0; i < m; i++) {
                if (request[i] > 0) {
                    is_release_only = false;
                    break;
                }
            }
            pthread_mutex_lock(&pmtx);
            if (is_release_only) {
                cout << "Master thread tries to grant pending requests" << endl;
            } else {
                cout << "Master thread stores resource request of thread " << thread_id << endl;
            }
            pthread_mutex_unlock(&pmtx);
            for (int i = 0; i < m; i++) {
                if (request[i] < 0) {
                    available[i] += -request[i];
                    allocation[thread_id][i] += request[i];
                    need[thread_id][i] = maximum[thread_id][i] - allocation[thread_id][i];
                    if (!is_release_only) {
                        request[i] = 0;
                    }
                }
            }
            if (!is_release_only) {
                pthread_mutex_lock(&pmtx);
                cout << "\t\tWaiting threads: ";
                print_waiting_threads();
                if (!Q.empty() || thread_id >= 0) {
                    if (!Q.empty()) cout << " ";
                    cout << thread_id;
                }
                cout << endl;
                pthread_mutex_unlock(&pmtx);
                Q.push(make_pair(thread_id, request));
            }
            process_pending_requests();
        }
    }
    return NULL;
}

bool check_safety(int thread_id, vector<int> &request) {
#ifdef _DLAVOID
    vector<int> work(m);
    vector<bool> finish(n, false);
    for (int i = 0; i < m; i++) {
        work[i] = available[i];
    }
    vector<int> temp_alloc(m), temp_need(m);
    for (int i = 0; i < m; i++) {
        work[i] -= request[i];
        temp_alloc[i] = allocation[thread_id][i] + request[i];
        temp_need[i] = need[thread_id][i] - request[i];
        if (work[i] < 0 || temp_need[i] < 0) {
            return false;
        }
    }
    for (int i = 0; i < n; i++) {
        finish[i] = thread_exit[i];
    }
    int count = 0;
    bool found = true;
    while (found && count < n) {
        found = false;
        for (int i = 0; i < n; i++) {
            if (!finish[i]) {
                bool can_finish = true;
                for (int j = 0; j < m; j++) {
                    int need_val = (i == thread_id) ? temp_need[j] : need[i][j];
                    if (need_val > work[j]) {
                        can_finish = false;
                        break;
                    }
                }
                if (can_finish) {
                    for (int j = 0; j < m; j++) {
                        int alloc_val = (i == thread_id) ? temp_alloc[j] : allocation[i][j];
                        work[j] += alloc_val;
                    }
                    finish[i] = true;
                    found = true;
                    count++;
                }
            }
        }
    }
    for (int i = 0; i < n; i++) {
        if (!thread_exit[i] && !finish[i]) {
            return false;
        }
    }
    return true;
#else
    for (int i = 0; i < m; i++) {
        if (request[i] > available[i] || request[i] > need[thread_id][i]) {
            return false;
        }
    }
    return true;
#endif
}

void process_pending_requests() {
    int queue_size = Q.size();
    bool processed_at_least_one = false;
    for (int i = 0; i < queue_size; i++) {
        pair<int, vector<int>> front = Q.front();
        Q.pop();
        int thread_id = front.first;
        vector<int> request = front.second;
        bool insufficient_resources = false;
        for (int j = 0; j < m; j++) {
            if (request[j] > available[j]) {
                insufficient_resources = true;
                break;
            }
        }
        bool unsafe_state = false;
        if (!insufficient_resources) {
#ifdef _DLAVOID
            if (!check_safety(thread_id, request)) {
                unsafe_state = true;
            }
#endif
        }
        bool can_grant = !insufficient_resources && !unsafe_state;
        pthread_mutex_lock(&pmtx);
        if (!can_grant) {
            if (insufficient_resources) {
                cout << "    +++ Insufficient resources to grant request of thread " << thread_id << endl;
            } else if (unsafe_state) {
                cout << "    +++ Unsafe to grant request of thread " << thread_id << endl;
            }
            Q.push(front);
        } else {
            cout << "Master thread grants resource request for thread " << thread_id << endl;
            for (int j = 0; j < m; j++) {
                available[j] -= request[j];
                allocation[thread_id][j] += request[j];
                need[thread_id][j] -= request[j];
            }
            pthread_mutex_lock(&cmtx[thread_id]);
            thread_sync[thread_id] = false;
            pthread_cond_signal(&cv[thread_id]);
            pthread_mutex_unlock(&cmtx[thread_id]);
            processed_at_least_one = true;
        }
        pthread_mutex_unlock(&pmtx);
    }
    pthread_mutex_lock(&pmtx);
    cout << "\t\tWaiting threads: ";
    print_waiting_threads();
    cout << endl;
    pthread_mutex_unlock(&pmtx);
    if (processed_at_least_one && !Q.empty()) {
        process_pending_requests();
    }
}

void print_waiting_threads() {
    queue<pair<int, vector<int>>> temp = Q;
    bool first = true;
    while (!temp.empty()) {
        if (!first) cout << " ";
        cout << temp.front().first;
        temp.pop();
        first = false;
    }
}
