#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PROCESSES 200
#define MAX_BURSTS 50
#define MAX_QUEUE 2000
#define INFINITE 1000000000

// Process states
#define READY 0
#define RUNNING 1
#define WAITING 2
#define FINISHED 3

// Event types
#define ARRIVAL 0
#define CPU_END 1
#define IO_END 2
#define TIMEOUT 3

// Process structure
typedef struct {
    int id;
    int arrival_time;
    int num_bursts;
    int cpu_bursts[MAX_BURSTS];
    int io_bursts[MAX_BURSTS];
    int current_burst;
    int state;
    int remaining_time;
    int total_runtime;
    int total_cpu_time;
    int finished;
} Process;

// Event structure
typedef struct {
    int time;
    int type;
    int process_id;
} Event;

// Global variables
Process processes[MAX_PROCESSES];
int num_processes = 0;
int current_time = 0;
int cpu_idle_time = 0;
int cpu_idle_start = -1;
int running_process = -1;
int completed_processes = 0;

// Ready Queue
int ready_queue[MAX_QUEUE];
int ready_front = 0;
int ready_rear = -1;
int ready_size = 0;

// Event Queue
Event event_queue[MAX_QUEUE];
int event_size = 0;

// Queue operations
void ready_queue_init() {
    ready_front = 0;
    ready_rear = -1;
    ready_size = 0;
}

void ready_queue_push(int process_id) {
    ready_rear = (ready_rear + 1) % MAX_QUEUE;
    ready_queue[ready_rear] = process_id;
    ready_size++;
}

int ready_queue_pop() {
    if (ready_size == 0) return -1;
    int process_id = ready_queue[ready_front];
    ready_front = (ready_front + 1) % MAX_QUEUE;
    ready_size--;
    return process_id;
}

int ready_queue_empty() {
    return ready_size == 0;
}

// Event comparison function
int compare_events(Event a, Event b) {
    if (a.time != b.time)
        return a.time - b.time;
    
    if (a.type != b.type) {
        if (a.type == ARRIVAL || a.type == IO_END)
            return -1;
        if (b.type == ARRIVAL || b.type == IO_END)
            return 1;
        if (a.type == TIMEOUT)
            return -1;
        if (b.type == TIMEOUT)
            return 1;
    }
    
    return processes[a.process_id].id - processes[b.process_id].id;
}

// Event Queue operations
void swap_events(int i, int j) {
    Event temp = event_queue[i];
    event_queue[i] = event_queue[j];
    event_queue[j] = temp;
}

void event_queue_push(Event event) {
    int current = event_size;
    event_queue[current] = event;
    event_size++;
    
    while (current > 0) {
        int parent = (current - 1) / 2;
        if (compare_events(event_queue[current], event_queue[parent]) < 0) {
            swap_events(current, parent);
            current = parent;
        } else {
            break;
        }
    }
}

Event event_queue_pop() {
    Event min_event = event_queue[0];
    event_queue[0] = event_queue[event_size - 1];
    event_size--;
    
    int current = 0;
    while (1) {
        int smallest = current;
        int left = 2 * current + 1;
        int right = 2 * current + 2;
        
        if (left < event_size && compare_events(event_queue[left], event_queue[smallest]) < 0)
            smallest = left;
            
        if (right < event_size && compare_events(event_queue[right], event_queue[smallest]) < 0)
            smallest = right;
        
        if (smallest == current)
            break;
            
        swap_events(current, smallest);
        current = smallest;
    }
    
    return min_event;
}

void read_input() {
    FILE *fp = fopen("proc.txt", "r");
    if (!fp) {
        printf("Error: Cannot open proc.txt\n");
        exit(1);
    }
    
    fscanf(fp, "%d", &num_processes);
    
    for (int i = 0; i < num_processes; i++) {
        Process *p = &processes[i];
        memset(p, 0, sizeof(Process));
        
        fscanf(fp, "%d %d", &p->id, &p->arrival_time);
        
        int burst_count = 0;
        p->total_runtime = 0;
        p->total_cpu_time = 0;
        
        while (1) {
            int burst;
            fscanf(fp, "%d", &burst);
            if (burst == -1) break;
            
            if (burst_count % 2 == 0) {
                p->cpu_bursts[burst_count/2] = burst;
                p->total_cpu_time += burst;
                p->total_runtime += burst;
            } else {
                p->io_bursts[burst_count/2] = burst;
                p->total_runtime += burst;
            }
            burst_count++;
        }
        
        p->num_bursts = (burst_count + 1) / 2;
        p->current_burst = 0;
        p->state = READY;
        p->remaining_time = p->cpu_bursts[0];
        p->finished = 0;
    }
    
    fclose(fp);
}

void init_simulation() {
    current_time = 0;
    cpu_idle_time = 0;
    cpu_idle_start = -1;
    running_process = -1;
    completed_processes = 0;
    event_size = 0;
    ready_queue_init();
    
    for (int i = 0; i < num_processes; i++) {
        processes[i].current_burst = 0;
        processes[i].state = READY;
        processes[i].remaining_time = processes[i].cpu_bursts[0];
        processes[i].finished = 0;
        
        Event arrival = {processes[i].arrival_time, ARRIVAL, i};
        event_queue_push(arrival);
    }
}

void start_cpu_idle() {
    if (running_process == -1 && cpu_idle_start == -1) {
        cpu_idle_start = current_time;
    }
}

void end_cpu_idle() {
    if (cpu_idle_start != -1) {
        cpu_idle_time += current_time - cpu_idle_start;
        cpu_idle_start = -1;
    }
}

void schedule_process(int quantum) {
    if (running_process != -1) return;
    
    if (!ready_queue_empty()) {
        running_process = ready_queue_pop();
        processes[running_process].state = RUNNING;
        
        int run_time = quantum;
        if (processes[running_process].remaining_time < quantum || quantum == INFINITE) {
            run_time = processes[running_process].remaining_time;
        }
        
        end_cpu_idle();
        
#ifdef VERBOSE
        printf("%d         : Process %d is scheduled to run for time %d\n", 
               current_time, processes[running_process].id, run_time);
#endif
        
        Event next_event;
        next_event.time = current_time + run_time;
        next_event.process_id = running_process;
        next_event.type = (run_time == processes[running_process].remaining_time) ? CPU_END : TIMEOUT;
        
        if (next_event.type == TIMEOUT) {
            processes[running_process].remaining_time -= run_time;
        }
        
        event_queue_push(next_event);
    } else {
        start_cpu_idle();
#ifdef VERBOSE
        if (current_time > 0) {
            printf("%d         : CPU goes idle\n", current_time);
        }
#endif
    }
}

void handle_process_completion(Process *p) {
    int turnaround_time = current_time - p->arrival_time;
    int wait_time = turnaround_time - p->total_runtime;
    // Fixed percentage calculation with proper rounding
    int turnaround_percentage = (int)((turnaround_time * 100.0) / p->total_runtime + 0.5);
    
    printf("%-10d: Process %6d exits. Turnaround time = %4d (%3d%%), Wait time = %d\n",
           current_time, p->id, turnaround_time, turnaround_percentage, wait_time);
}

void simulate_scheduler(int quantum) {
    init_simulation();
    
#ifdef VERBOSE
    printf("%d         : Starting\n", current_time);
#endif
    
    double total_wait_time = 0;
    
    while (event_size > 0 && completed_processes < num_processes) {
        Event event = event_queue_pop();
        
        if (running_process == -1 && cpu_idle_start != -1) {
            cpu_idle_time += event.time - cpu_idle_start;
            cpu_idle_start = event.time;
        }
        
        current_time = event.time;
        Process *p = &processes[event.process_id];
        
        if (p->finished) continue;
        
        switch (event.type) {
            case ARRIVAL:
            case IO_END:
                if (event.type == IO_END) {
                    p->state = READY;
                    p->remaining_time = p->cpu_bursts[p->current_burst];
                }
#ifdef VERBOSE
                printf("%d         : Process %d joins ready queue %s\n",
                       current_time, p->id, 
                       event.type == ARRIVAL ? "upon arrival" : "after IO completion");
#endif
                ready_queue_push(event.process_id);
                break;
                
            case CPU_END:
                running_process = -1;
                p->current_burst++;
                
                if (p->current_burst == p->num_bursts) {
                    p->state = FINISHED;
                    p->finished = 1;
                    completed_processes++;
                    int turnaround_time = current_time - p->arrival_time;
                    int wait_time = turnaround_time - p->total_runtime;
                    total_wait_time += wait_time;
                    handle_process_completion(p);
                } else {
                    p->state = WAITING;
                    Event io_completion = {
                        current_time + p->io_bursts[p->current_burst - 1],
                        IO_END,
                        event.process_id
                    };
                    event_queue_push(io_completion);
                }
                break;
                
            case TIMEOUT:
                running_process = -1;
#ifdef VERBOSE
                printf("%d         : Process %d joins ready queue after timeout\n",
                       current_time, p->id);
#endif
                ready_queue_push(event.process_id);
                break;
        }
        
        schedule_process(quantum);
    }
    
    if (cpu_idle_start != -1) {
        cpu_idle_time += current_time - cpu_idle_start;
    }
    
    // Fixed floating-point calculations with proper rounding
    double avg_wait_time = total_wait_time / num_processes;
    double cpu_active_time = current_time - cpu_idle_time;
    double cpu_util = (cpu_active_time * 100.0) / current_time;
    
    printf("\nAverage wait time = %.2f\n", avg_wait_time);
    printf("Total turnaround time = %d\n", current_time);
    printf("CPU idle time = %d\n", cpu_idle_time);
    printf("CPU utilization = %.2f%%\n\n", cpu_util);
}

int main() {
    read_input();
    
    printf("**** FCFS Scheduling ****\n");
    simulate_scheduler(INFINITE);
    
    printf("**** RR Scheduling with q = 10 ****\n");
    simulate_scheduler(10);
    
    printf("**** RR Scheduling with q = 5 ****\n");
    simulate_scheduler(5);
    
    return 0;
}