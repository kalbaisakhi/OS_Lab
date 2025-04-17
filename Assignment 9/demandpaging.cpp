#include <iostream>
#include <fstream>
#include <vector>
#include <deque>
#include <iomanip>

struct Process {
    int size;
    std::vector<int> search_keys;
    unsigned short* page_table;
    int searches_done;
    bool is_active;
    Process() : size(0), search_keys(), page_table(nullptr), searches_done(0), is_active(false) {}
    ~Process() {
        if (page_table) {
            delete[] page_table;
        }
    }
};
int n, m;
int page_faults = 0;
int page_accesses = 0;
int swaps = 0;
int min_active_processes;
std::vector<Process> processes;
std::deque<int> ready_queue;
std::deque<int> swapped_out_queue;
std::deque<int> free_frames;
bool load_page(Process& process, int page_index);
void swap_out_process(int process_number);
bool binary_search(int process_number);
void swap_in_process();
void run_simulation();
void print_results();
void read_input();

bool load_page(Process& process, int page_index) {
    page_faults++;
    if (!free_frames.empty()) {
        int frame = free_frames.front();
        free_frames.pop_front();
        process.page_table[page_index] = frame;

        process.page_table[page_index] |= (1 << 15);
        return true;
    }
    return false;
}

void swap_out_process(int process_number) {
    Process& proc = processes[process_number];
    proc.is_active = false;
    for (int i = 0; i < 2048; i++) {
        if (proc.page_table[i] & (1 << 15)) {
            int frame = proc.page_table[i] & 0x3FFF;
            proc.page_table[i] &= ~(1 << 15);
            free_frames.push_back(frame);
        }
    }
    if (proc.searches_done < m) {
        swapped_out_queue.push_back(process_number);
        std::cout << "+++ Swapping out process " << std::setw(4) << process_number 
                  << "  [" << ready_queue.size() << " active processes]" << std::endl;
        if (ready_queue.size() < min_active_processes) {
            min_active_processes = ready_queue.size();
        }
        swaps++;
    }
}

bool binary_search(int process_number) {
    Process& proc = processes[process_number];
    int search_key = proc.search_keys[proc.searches_done];
    int L = 0, R = proc.size - 1;
    while (L < R) {
        int M = (L + R) / 2;
        int page_index = 10 + M / 1024;
        page_accesses++;
        if (!(proc.page_table[page_index] & (1 << 15))) {
            if (!load_page(proc, page_index)) {
                swap_out_process(process_number);
                return false;
            }
        }
        if (search_key <= M)
            R = M;
        else
            L = M + 1;
    }
    proc.searches_done++;
    return true;
}

void swap_in_process() {
    if (free_frames.size() < 10)
        return;
    if (swapped_out_queue.empty())
        return;
    int process_number = swapped_out_queue.front();
    swapped_out_queue.pop_front();
    Process& proc = processes[process_number];
    for (int i = 0; i < 10; i++) {
        int frame = free_frames.front();
        free_frames.pop_front();
        proc.page_table[i] = frame;

        proc.page_table[i] |= (1 << 15);
    }
    proc.is_active = true;
    std::cout << "+++ Swapping in process " << std::setw(5) << process_number 
              << "  [" << ready_queue.size() + 1 << " active processes]" << std::endl;
    ready_queue.push_front(process_number);
}

void run_simulation() {
    std::cout << "+++ Kernel data initialized" << std::endl;
    min_active_processes = n;
    while (true) {
        bool all_finished = true;
        for (int i = 0; i < n; i++) {
            if (processes[i].searches_done < m) {
                all_finished = false;
                break;
            }
        }
        if (all_finished)
            break;
        int curr_process = ready_queue.front();
        ready_queue.pop_front();
        Process& proc = processes[curr_process];
        if (proc.is_active && proc.searches_done < m) {
#ifdef VERBOSE
            std::cout << "\tSearch " << proc.searches_done + 1 << " by Process " << curr_process << std::endl;
#endif
            if (binary_search(curr_process)) {
                if (proc.searches_done < m)
                    ready_queue.push_back(curr_process);
                else {
                    swap_out_process(curr_process);
                    swap_in_process();
                }
            }
        }
    }
}

void print_results() {
    std::cout << "\n +++ Page access summary" << std::endl;
    std::cout << " Total number of page accesses = " << page_accesses << std::endl;
    std::cout << " Total number of page faults = " << page_faults << std::endl;
    std::cout << " Total number of swaps = " << swaps << std::endl;
    std::cout << " Degree of multiprogramming = " << min_active_processes << std::endl;
}

void read_input() {
    std::ifstream infile("search.txt");
    if (!infile) {
        std::cerr << "Error opening input file" << std::endl;
        exit(1);
    }
    infile >> n >> m;
    processes.resize(n);
    for (int i = 0; i < 12288; i++) {
        free_frames.push_back(i);
    }
    
    for (int i = 0; i < n; i++) {
        Process& proc = processes[i];
        proc.is_active = true;
        proc.searches_done = 0;
        proc.page_table = new unsigned short[2048]();
        infile >> proc.size;
        proc.search_keys.resize(m);
        for (int j = 0; j < m; j++)
            infile >> proc.search_keys[j];
        for (int j = 0; j < 10; j++) {
            int frame = free_frames.front();
            free_frames.pop_front();
            proc.page_table[j] = frame;
            proc.page_table[j] |= (1 << 15);
        }
        ready_queue.push_back(i);
    }
    std::cout << "+++ Simulation data read from file" << std::endl;
}

int main() {
    page_faults = 0;
    page_accesses = 0;
    swaps = 0;
    read_input();
    run_simulation();
    print_results();
    return 0;
}
