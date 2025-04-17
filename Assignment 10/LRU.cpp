#include <iostream>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <queue>
#include <ctime>
#include <iomanip>
#include <climits>
#include <cstdint>

// Constants
const int TOTAL_MEMORY = 64 * 1024 * 1024;           // 64 MB
const int OS_USAGE = 16 * 1024 * 1024;               // 16 MB
const int USER_SPACE = 48 * 1024 * 1024;             // 48 MB
const int PAGE_SIZE = 4 * 1024;                      // 4 KB
const int NUM_USER_FRAMES = USER_SPACE / PAGE_SIZE;  // 12288 frames
const int VIRTUAL_MEMORY_SIZE = 2048;                // 2048 pages per process
const int ESSENTIAL_PAGES = 10;                      // 10 essential pages per process
const int ARRAY_PAGES = VIRTUAL_MEMORY_SIZE - ESSENTIAL_PAGES; // 2038 pages for array
const int NFFMIN = 1000;                             // Minimum free frames to maintain

// Page table entry masks
const uint16_t VALID_MASK = 0x8000;  // Bit 15 (MSB)
const uint16_t REF_MASK = 0x4000;    // Bit 14
const uint16_t FRAME_MASK = 0x3FFF;  // Bits 0-13

// Global statistics
unsigned long total_page_accesses = 0;
unsigned long total_page_faults = 0;
unsigned long total_page_replacements = 0;
unsigned long total_attempt1 = 0;
unsigned long total_attempt2 = 0;
unsigned long total_attempt3 = 0;
unsigned long total_attempt4 = 0;

// Free frame list entry structure
struct FreeFrameEntry {
    int frame_number;
    int last_owner;     // PID of last owner (-1 if none)
    int last_page;      // Page number of last owner (-1 if none)

    FreeFrameEntry(int f = -1, int o = -1, int p = -1) 
        : frame_number(f), last_owner(o), last_page(p) {}
};

// Process Control Block
struct Process {
    int pid;
    int s;                  // Size of array A
    int m;                  // Number of searches
    int current_search;     // Current search index
    std::vector<int> keys;  // Search keys
    
    // Page table and history
    std::vector<uint16_t> page_table;
    std::vector<uint16_t> history;
    
    // Frame to page mapping for quick lookup
    std::vector<int> frame_to_page;
    
    // Statistics
    int accesses;
    int faults;
    int replacements;
    int attempt1;
    int attempt2;
    int attempt3;
    int attempt4;
    
    Process(int id) : pid(id), current_search(0), 
                    page_table(VIRTUAL_MEMORY_SIZE, 0),
                    history(VIRTUAL_MEMORY_SIZE, 0xFFFF),
                    frame_to_page(NUM_USER_FRAMES, -1),
                    accesses(0), faults(0), replacements(0),
                    attempt1(0), attempt2(0), attempt3(0), attempt4(0) {}
};

// Free Frame List
std::vector<FreeFrameEntry> FFLIST;
int NFF; // Number of free frames

// Initialize the free frame list
void init_free_frames() {
    FFLIST.clear();
    FFLIST.reserve(NUM_USER_FRAMES);
    
    for (int i = 0; i < NUM_USER_FRAMES; i++) {
        FFLIST.push_back(FreeFrameEntry(i, -1, -1));
    }
    
    NFF = NUM_USER_FRAMES;
}

// Get a frame from FFLIST and remove it
FreeFrameEntry get_free_frame(int index) {
    FreeFrameEntry frame = FFLIST[index];
    FFLIST[index] = FFLIST.back();
    FFLIST.pop_back();
    NFF--;
    return frame;
}

// Add a frame to FFLIST
void add_free_frame(const FreeFrameEntry& frame) {
    FFLIST.push_back(frame);
    NFF++;
}

// Set a page table entry
void set_page_table_entry(Process& proc, int page, int frame) {
    proc.page_table[page] = VALID_MASK | (frame & FRAME_MASK);
    proc.history[page] = 0xFFFF; // Most recently used
    proc.frame_to_page[frame] = page;
}

// Invalidate a page
void invalidate_page(Process& proc, int page) {
    int frame = proc.page_table[page] & FRAME_MASK;
    if (proc.page_table[page] & VALID_MASK) {
        proc.page_table[page] &= ~VALID_MASK;
        proc.frame_to_page[frame] = -1;
    }
}

// Find victim page using approximate LRU
int find_victim_page(Process& proc) {
    int victim = -1;
    uint16_t min_history = 0xFFFF;
    
    // Essential pages are never replaced
    for (int page = ESSENTIAL_PAGES; page < VIRTUAL_MEMORY_SIZE; page++) {
        if ((proc.page_table[page] & VALID_MASK) && proc.history[page] < min_history) {
            min_history = proc.history[page];
            victim = page;
        }
    }
    
    return victim;
}

// Page replacement algorithm
int replace_page(Process& proc, int new_page) {
    int frame = -1;
    bool found = false;
    
    // Attempt 1: Find a free frame with last owner = proc.pid and last page = new_page
    for (size_t i = 0; i < FFLIST.size() && !found; i++) {
        if (FFLIST[i].last_owner == proc.pid && FFLIST[i].last_page == new_page) {
            frame = FFLIST[i].frame_number;
            FFLIST[i] = FFLIST.back();
            FFLIST.pop_back();
            NFF--;
            found = true;
            proc.attempt1++;
            total_attempt1++;
            
#ifdef VERBOSE
            std::cout << "\tAttempt 1: Found frame " << frame 
                      << " with last owner PID=" << proc.pid 
                      << " and page=" << new_page << std::endl;
#endif
        }
    }
    
    // Attempt 2: Find a free frame with no owner
    if (!found) {
        for (size_t i = 0; i < FFLIST.size() && !found; i++) {
            if (FFLIST[i].last_owner == -1) {
                frame = FFLIST[i].frame_number;
                FFLIST[i] = FFLIST.back();
                FFLIST.pop_back();
                NFF--;
                found = true;
                proc.attempt2++;
                total_attempt2++;
                
#ifdef VERBOSE
                std::cout << "\tAttempt 2: Found frame " << frame 
                          << " with no owner" << std::endl;
#endif
            }
        }
    }
    
    // Attempt 3: Find a free frame owned by this process
    if (!found) {
        for (size_t i = 0; i < FFLIST.size() && !found; i++) {
            if (FFLIST[i].last_owner == proc.pid) {
                frame = FFLIST[i].frame_number;
                FFLIST[i] = FFLIST.back();
                FFLIST.pop_back();
                NFF--;
                found = true;
                proc.attempt3++;
                total_attempt3++;
                
#ifdef VERBOSE
                std::cout << "\tAttempt 3: Found frame " << frame 
                          << " previously owned by PID=" << proc.pid 
                          << " for page=" << FFLIST[i].last_page << std::endl;
#endif
            }
        }
    }
    
    // Attempt 4: Pick a random free frame
    if (!found && !FFLIST.empty()) {
        int idx = std::rand() % FFLIST.size();
        frame = FFLIST[idx].frame_number;
        FFLIST[idx] = FFLIST.back();
        FFLIST.pop_back();
        NFF--;
        found = true;
        proc.attempt4++;
        total_attempt4++;
        
#ifdef VERBOSE
        std::cout << "\tAttempt 4: Randomly selected frame " << frame << std::endl;
#endif
    }
    
    // Find a victim page to replace
    int victim_page = find_victim_page(proc);
    if (victim_page != -1) {
        int victim_frame = proc.page_table[victim_page] & FRAME_MASK;
        
        // Add the victim's frame to FFLIST
        FreeFrameEntry victim_entry(victim_frame, proc.pid, victim_page);
        add_free_frame(victim_entry);
        
        // Invalidate the victim page
        invalidate_page(proc, victim_page);
        
        proc.replacements++;
        total_page_replacements++;
        
#ifdef VERBOSE
        std::cout << "\tReplacing page " << victim_page << " in frame " << victim_frame 
                  << " with history " << proc.history[victim_page] << std::endl;
#endif
    }
    
    return frame;
}

// Update history bits after each binary search
void update_history(Process& proc) {
    for (int page = 0; page < VIRTUAL_MEMORY_SIZE; page++) {
        if (proc.page_table[page] & VALID_MASK) {
            // Shift history right and insert reference bit at MSB
            uint16_t ref_bit = (proc.page_table[page] & REF_MASK) ? 1 : 0;
            proc.history[page] = (proc.history[page] >> 1) | (ref_bit << 15);
            
            // Clear reference bit
            proc.page_table[page] &= ~REF_MASK;
        }
    }
}

// Allocate a frame for a page
int allocate_frame(Process& proc, int page) {
    // If we're at minimum free frames, use page replacement
    if (NFF <= NFFMIN) {
        return replace_page(proc, page);
    }
    
    // Otherwise, allocate a free frame
    FreeFrameEntry entry = get_free_frame(NFF - 1);
    return entry.frame_number;
}

// Access page and handle page fault if needed
bool access_page(Process& proc, int page) {
    proc.accesses++;
    total_page_accesses++;
    
    if (!(proc.page_table[page] & VALID_MASK)) {
        // Page fault
        proc.faults++;
        total_page_faults++;
        
#ifdef VERBOSE
        std::cout << "\tPage fault on page " << page << std::endl;
#endif
        
        // Allocate a frame
        int frame = allocate_frame(proc, page);
        if (frame < 0) {
            std::cerr << "Error: No frames available!" << std::endl;
            return false;
        }
        
        // Set the page table entry
        set_page_table_entry(proc, page, frame);
    }
    
    // Set reference bit
    proc.page_table[page] |= REF_MASK;
    return true;
}

// Release all frames held by a process
void release_process_frames(Process& proc) {
    for (int page = 0; page < VIRTUAL_MEMORY_SIZE; page++) {
        if (proc.page_table[page] & VALID_MASK) {
            int frame = proc.page_table[page] & FRAME_MASK;
            
            // Add frame to FFLIST with no owner (process is exiting)
            FreeFrameEntry entry(frame, -1, -1);
            add_free_frame(entry);
            
            // Invalidate the page
            invalidate_page(proc, page);
        }
    }
    
#ifdef VERBOSE
    std::cout << "+++ Process " << proc.pid << " exited and released all frames" << std::endl;
#endif
}

// Perform binary search for a process
bool binary_search(Process& proc, int key) {
#ifdef VERBOSE
    std::cout << "+++ Process " << proc.pid << ": Search " << proc.current_search + 1 << std::endl;
#endif
    
    int L = 0;
    int R = proc.s - 1;
    
    while (L < R) {
        int M = (L + R) / 2;
        
        // Calculate page number for A[M]
        int page = ESSENTIAL_PAGES + (M / (PAGE_SIZE / sizeof(int)));
        
        if (!access_page(proc, page)) {
            return false;
        }
        
        if (key <= M) {
            R = M;
        } else {
            L = M + 1;
        }
    }
    
    // Update history bits after search
    update_history(proc);
    return true;
}

// Initialize essential pages for a process
void init_essential_pages(Process& proc) {
    for (int page = 0; page < ESSENTIAL_PAGES; page++) {
        if (NFF <= 0) {
            std::cerr << "Error: No frames available for essential pages!" << std::endl;
            exit(1);
        }
        
        FreeFrameEntry entry = get_free_frame(NFF - 1);
        set_page_table_entry(proc, page, entry.frame_number);
        
    }
}

// Print statistics
void print_statistics(const std::vector<Process>& processes) {
    std::cout << "+++ Page access summary" << std::endl;
    std::cout << "PID     Accesses            Faults                    Replacements                Attempts" << std::endl;
    
    for (size_t i = 0; i < processes.size(); i++) {
        const Process& proc = processes[i];
        int total_attempts = proc.attempt1 + proc.attempt2 + proc.attempt3 + proc.attempt4;
        if (total_attempts == 0) total_attempts = 1; // Avoid division by zero
        
        double fault_percent = 100.0 * proc.faults / (proc.accesses ? proc.accesses : 1);
        double replace_percent = 100.0 * proc.replacements / (proc.accesses ? proc.accesses : 1);
        double a1_percent = 100.0 * proc.attempt1 / total_attempts;
        double a2_percent = 100.0 * proc.attempt2 / total_attempts;
        double a3_percent = 100.0 * proc.attempt3 / total_attempts;
        double a4_percent = 100.0 * proc.attempt4 / total_attempts;
        
        std::cout << std::setw(5) << proc.pid << std::setw(18) << proc.accesses 
                  << std::setw(8) << proc.faults << " (" << std::fixed << std::setprecision(2) << fault_percent << "%)"
                  << std::setw(12) << proc.replacements << " (" << std::fixed << std::setprecision(2) << replace_percent << "%)"
                  << std::setw(8) << proc.attempt1 << " +" << std::setw(5) << proc.attempt2 
                  << " +" << std::setw(5) << proc.attempt3 << " +" << std::setw(5) << proc.attempt4 
                  << " (" << std::fixed << std::setprecision(2) << a1_percent << "% +" 
                  << std::setw(7) << a2_percent << "% +" << std::setw(7) << a3_percent 
                  << "% +" << std::setw(7) << a4_percent << "%)" << std::endl;
    }
    
    // Print total statistics
    int total_attempts = total_attempt1 + total_attempt2 + total_attempt3 + total_attempt4;
    if (total_attempts == 0) total_attempts = 1; // Avoid division by zero
    
    double fault_percent = 100.0 * total_page_faults / (total_page_accesses ? total_page_accesses : 1);
    double replace_percent = 100.0 * total_page_replacements / (total_page_accesses ? total_page_accesses : 1);
    double a1_percent = 100.0 * total_attempt1 / total_attempts;
    double a2_percent = 100.0 * total_attempt2 / total_attempts;
    double a3_percent = 100.0 * total_attempt3 / total_attempts;
    double a4_percent = 100.0 * total_attempt4 / total_attempts;
    
    std::cout << "Total" << std::setw(18) << total_page_accesses 
              << std::setw(8) << total_page_faults << " (" << std::fixed << std::setprecision(2) << fault_percent << "%)"
              << std::setw(12) << total_page_replacements << " (" << std::fixed << std::setprecision(2) << replace_percent << "%)"
              << std::setw(8) << total_attempt1 << " +" << std::setw(5) << total_attempt2 
              << " +" << std::setw(5) << total_attempt3 << " +" << std::setw(5) << total_attempt4 
              << " (" << std::fixed << std::setprecision(2) << a1_percent << "% +" 
              << std::setw(7) << a2_percent << "% +" << std::setw(7) << a3_percent 
              << "% +" << std::setw(7) << a4_percent << "%)" << std::endl;
}

int main() {
    // Seed random number generator
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    
    // Initialize free frame list
    init_free_frames();
    
    // Read input from search.txt
    std::ifstream infile("search.txt");
    if (!infile) {
        std::cerr << "Error opening input file 'search.txt'" << std::endl;
        return 1;
    }
    
    int n, m;
    infile >> n >> m;
    
    // Create processes
    std::vector<Process> processes;
    for (int i = 0; i < n; i++) {
        Process proc(i);
        proc.m = m;
        
        // Read array size
        infile >> proc.s;
        
        // Read search keys
        proc.keys.resize(m);
        for (int j = 0; j < m; j++) {
            infile >> proc.keys[j];
        }
        
        // Initialize essential pages
        init_essential_pages(proc);
        
        processes.push_back(proc);
    }
    
    infile.close();
    
    // Round-robin process scheduling
    std::queue<int> ready_queue;
    for (int i = 0; i < n; i++) {
        ready_queue.push(i);
    }
    
    // Process each search
    while (!ready_queue.empty()) {
        int pid = ready_queue.front();
        ready_queue.pop();
        
        Process& proc = processes[pid];
        
        // Perform binary search
        int key = proc.keys[proc.current_search];
        if (binary_search(proc, key)) {
            proc.current_search++;
            
            // If process has more searches, put it back in the queue
            if (proc.current_search < proc.m) {
                ready_queue.push(pid);
            } else {
                // Process is done, release its frames
                release_process_frames(proc);
            }
        } else {
            // Search failed, put process back in queue to retry
            ready_queue.push(pid);
        }
    }
    
    // Print statistics
    print_statistics(processes);
    
    return 0;
}