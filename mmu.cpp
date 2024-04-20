#include <iostream>
#include <vector>
#include <bitset>
#include <cassert>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <cctype>
#include <algorithm>
#include <queue>
#include <unistd.h> // for getopt
#include <cstdlib>  // for exit and stoi
#include <climits>  // For UINT_MAX


// Constants
const int NUM_VIRTUAL_PAGES = 64; // Number of virtual pages per process
bool o_flag = false;

// Global variable for the number of frames, initialized later
int numFrames = 128; // Default value if not specified by the user
unsigned int TAU = 49;  // Time window
unsigned long inst_count = 0;  //


std::vector<int> randvals; // Store the random values read from file
int ofs = 0;          // Offset in the randvals vector

int myrandom(int burst) {
    int rand = 1 + (randvals[ofs] % burst);
    ofs = (ofs + 1) % randvals.size();
    return rand;
}

void readRandomNumbers(const std::string& filename) {
    std::ifstream file(filename);
    int num;
    int count;  // To hold the number of random numbers in the file

    if (file >> count) {  // Read the first number which is the count
        while (file >> num) {
            randvals.push_back(num);
        }
    }
    file.close();
}


struct ProcessStats {
    unsigned long maps = 0;
    unsigned long unmaps = 0;
    unsigned long ins = 0;
    unsigned long outs = 0;
    unsigned long fins = 0;
    unsigned long fouts = 0;
    unsigned long zeros = 0;
    unsigned long segv = 0;
    unsigned long segprot = 0;

    unsigned long long computeTotalCost() const {
        return maps * 350 + unmaps * 410 + ins * 3200 + outs * 2750 +
               fins * 2350 + fouts * 2800 + zeros * 150 + segv * 440 + segprot * 410;
    }
};


// Define a structure for Virtual Memory Areas
struct VMA {
    int start_vpage;
    int end_vpage;
    bool write_protected;
    bool file_mapped;

    VMA(int start, int end, bool wp, bool fm)
        : start_vpage(start), end_vpage(end), write_protected(wp), file_mapped(fm) {}
};

struct pte_t {
    unsigned int present       : 1;
    unsigned int referenced    : 1;
    unsigned int modified      : 1;
    unsigned int write_protect : 1;
    unsigned int paged_out     : 1;
    unsigned int file_mapped   : 1;  // Added to indicate file mapping
    // unsigned int isValid       : 2;  // 0: Not Initialized, 1: Initialized but not Valid, 2: Inititalized and Valid
    unsigned int frame_number  : 7;  // Enough for 128 frames
    unsigned int unused        : 19; // Adjusted for the additional bit

    // Default constructor to initialize all bits to zero
    pte_t() : present(0), write_protect(0), modified(0), referenced(0), paged_out(0), frame_number(0), file_mapped(0), unused(0) {}
};


// Frame Table Entry
// struct frame_t {
//     int process_id = -1;  // Process ID using this frame, -1 if free
//     int virtual_page = -1;// Virtual page number mapped to this frame
// };

class frame_t {
public:
    int process_id = -1;      // ID of the process using the frame
    int virtual_page = -1;    // Virtual page number mapped to the frame
    bool dirty = false;       // Indicates if the frame has been modified
    unsigned int age = 0;     // Aging register/Last time the frame was used
    unsigned int age_ = 0;

    std::vector<char> data;   // Simulated frame data

    frame_t() : data(4096, 0) {}  // Assume each frame is 4KB

    void clear() {
        std::fill(data.begin(), data.end(), 0);  // Reset all data to zero
        age = 0;
    }

    // Method to reset the age
    void resetAge() {
        age = 0;
        age_ = inst_count;
    }
};


// Global frame table
std::vector<frame_t> frame_table; 
std::deque<frame_t*> free_frames;

// Extend the Process structure to include VMAs
struct Process {
    int pid;                             // Process ID
    std::vector<VMA> vmas;               // Vector of VMAs
    std::vector<pte_t> page_table;         // Vector of PTEs
    ProcessStats stats; 

    Process(int id) : pid(id) {
        // Initialize the page table with the correct number of entries
        page_table.resize(NUM_VIRTUAL_PAGES); 
        for (int i = 0; i < NUM_VIRTUAL_PAGES; ++i) {
            // each PTE should have a default initialized
            page_table[i] = pte_t();  
        }
    }

    void addVMA(int start, int end, bool wp, bool fm) {
        vmas.emplace_back(start, end, wp, fm);

        for (int i = start; i <= end; ++i) {
            page_table[i].write_protect = wp;
            page_table[i].file_mapped = fm;
        }
    }
};

std::vector<Process> processes;

// Abstract base class for page replacement algorithms
class Pager {
public:
    virtual frame_t* select_victim_frame(std::vector<frame_t>& frame_table, std::vector<Process>& processes, unsigned long inst_count) = 0;
};

// First-in-first-out (FIFO) page replacement algorithm implementation
class FIFOPager : public Pager {
protected:
    int current_index;

public:
    FIFOPager() : current_index(0) {}

    frame_t* select_victim_frame(std::vector<frame_t>& frame_table, std::vector<Process>& processes, unsigned long inst_count) override {
        frame_t* victim_frame = &frame_table[current_index];
        current_index = (current_index + 1) % frame_table.size();
        return victim_frame;
    }
};

// Clock page replacement algorithm implementation
class ClockPager : public Pager {
private:
    int currentIndex;
    std::vector<frame_t>& frameTable;

public:
    ClockPager(std::vector<frame_t>& frames) : frameTable(frames), currentIndex(0) {}

    frame_t* select_victim_frame(std::vector<frame_t>& frame_table, std::vector<Process>& processes, unsigned long inst_count) override {
    
        if (currentIndex >= frame_table.size()) {
            currentIndex = 0;
        }

        while (true) {
            frame_t* candidate = &frame_table[currentIndex];
            pte_t& pte = processes[candidate->process_id].page_table[candidate->virtual_page];

            // Check if this frame can be used as a victim
            if (!pte.referenced) {
                currentIndex = (currentIndex + 1) % frame_table.size();  // Move hand for next use
                return candidate;
            }

            // Reset the referenced bit and move the clock hand
            pte.referenced = 0;
            currentIndex = (currentIndex + 1) % frame_table.size();
        }

        // Should not reach here
        return nullptr;
    }
};

// Random page replacement algorithm implementation
class RandomPager : public Pager {
public:
    RandomPager() {}

    frame_t* select_victim_frame(std::vector<frame_t>& frame_table, std::vector<Process>& processes, unsigned long inst_count) override {
        if (frame_table.empty()) return nullptr; // Safety check

        int randomIndex = myrandom(frame_table.size()) - 1; // myrandom returns 1-based index
        return &frame_table[randomIndex];
    }
};

// Aging page replacement algorithm implementation
class AgingPager : public Pager {
private:
    std::vector<frame_t>& frameTable;
    int current_index;

public:
    AgingPager(std::vector<frame_t>& frames) : frameTable(frames), current_index(0) {}

    frame_t* select_victim_frame(std::vector<frame_t>& frame_table, std::vector<Process>& processes, unsigned long inst_count) override {
        // Age all frames: right-shift and set the highest bit if referenced
        for (auto& frame : frame_table) {
            if (frame.process_id != -1) { // Only age frames that are in use
                frame.age >>= 1; // Right shift the age
                pte_t& pte = processes[frame.process_id].page_table[frame.virtual_page];
                if (pte.referenced) {
                    frame.age |= 0x80000000; // Set the highest bit
                    pte.referenced = 0; // Reset the referenced bit
                }
            }
        }

        // Find the frame with the smallest age (the oldest frame)
        int victim_index = current_index;
        for (int i = 0; i < frame_table.size(); ++i) {
            int idx = (current_index + i) % frame_table.size();
            if (frame_table[idx].age < frame_table[victim_index].age) {
                victim_index = idx;
            }
        }

        // Reset the age of the victim frame and update current_index
        frame_table[victim_index].age = 0;
        current_index = (victim_index + 1) % frame_table.size();

        return &frame_table[victim_index];
    }
};

// Working Set page replacement algorithm implementation
class WorkingSetPager : public Pager {
private:
    std::vector<frame_t>& frameTable;
    std::vector<Process>& processes;
    int hand;  // Index to start scanning from
    const int TAU = 49;  // Time threshold for the working set

public:
    WorkingSetPager(std::vector<frame_t>& frames, std::vector<Process>& proc)
        : frameTable(frames), processes(proc), hand(0) {}

    frame_t* select_victim_frame(std::vector<frame_t>& frame_table, std::vector<Process>& processes, unsigned long inst_count) {
        frame_t* victim = nullptr;
        int minAge = -1;  // Initialize tonegative number

        for (int i = 0; i < frame_table.size(); ++i) {
            int idx = (hand + i) % frame_table.size();
            frame_t& frame = frame_table[idx];
            pte_t& pte = processes[frame.process_id].page_table[frame.virtual_page];
            
            if (pte.referenced) {
                pte.referenced = 0;  // Reset the referenced bit
                frame.age_ = inst_count;  // Update the last use time to current instruction count
            } 

            int temp_age = inst_count - frame.age_;

            // std::cout<< "TEMP AGE: "<<temp_age<<std::endl;

            
            if (temp_age > TAU) {
                victim = &frame;
                hand = (idx + 1) % frame_table.size();
                return victim;  // Found a frame old enough to be evicted
            } else if (temp_age > minAge) {
                minAge = temp_age;
                victim = &frame;
            }
        }
        // If all frames were referenced within TAU, evict the least recently used
        if (!victim) {
            victim = &frame_table[hand];
        }
        hand = (victim - &frame_table[0] + 1) % frame_table.size();
        return victim;
    }
};

class NRUPager : public Pager {
private:
    std::vector<frame_t>& frameTable;
    std::vector<Process>& processes;
    int hand;  // Current position in the frame table for scanning
    unsigned long lastReset;  // Last instruction count at the time of the last REFERENCED bit reset

public:
    NRUPager(std::vector<frame_t>& frames, std::vector<Process>& proc)
        : frameTable(frames), processes(proc), hand(0), lastReset(0) {}

    frame_t* select_victim_frame(std::vector<frame_t>& frame_table, std::vector<Process>& processes, unsigned long inst_count) override {
        const int RESET_INTERVAL = 48;  // Interval for resetting the REFERENCED bits
        bool reset = (inst_count - lastReset >= RESET_INTERVAL);
        int victimIndex = -1;

        int class0Index = -1;
        int class1Index = -1;
        int class2Index = -1;
        int class3Index = -1;

        if (reset) {
            lastReset = inst_count;  // Update the last reset time
        };

        // Scan through the frame table to classify frames
        for (int i = 0; i < frame_table.size(); ++i) {
            int idx = (hand + i) % frame_table.size();
            frame_t& frame = frame_table[idx];
            // if (frame.process_id != -1) {  // Only consider frames that are in use
                pte_t& pte = processes[frame.process_id].page_table[frame.virtual_page];
                
                // Classify the frame using direct conditions
                if (!pte.referenced && !pte.modified) {
                    if (class0Index == -1) class0Index = idx;
                } else if (!pte.referenced && pte.modified) {
                    if (class1Index == -1) class1Index = idx;
                } else if (pte.referenced && !pte.modified) {
                    if (class2Index == -1) class2Index = idx;
                } else if (pte.referenced && pte.modified) {
                    if (class3Index == -1) class3Index = idx;
                }

                if (reset) {
                    pte.referenced = 0;  // Reset the referenced bit
                }

                // Stop early if we find a class 0 frame and we're not resetting
                if (!reset && class0Index != -1) {
                    victimIndex = class0Index;
                    break;
                    // continue;
                }
        }

        // Select the lowest class victim if no ideal candidate was immediately found
        // if (victimIndex == -1) {
            if (class0Index != -1) {
                victimIndex = class0Index;
            } else if (class1Index != -1) {
                victimIndex = class1Index;
            } else if (class2Index != -1) {
                victimIndex = class2Index;
            } else if (class3Index != -1) {
                victimIndex = class3Index;
            }
        // }

        if (victimIndex != -1) {
            hand = (victimIndex + 1) % numFrames; // Set hand for the next call
            return &frame_table[victimIndex];
        }

        return nullptr;  // No victim found (shouldn't happen if there are frames)
    }
};


Pager* THE_PAGER;

// FIFOPager* THE_PAGER = new FIFOPager();
// Pager* THE_PAGER = new FIFOPager();
// Pager* THE_PAGER = new ClockPager(frame_table);
// Pager* THE_PAGER = new RandomPager();
// Pager* THE_PAGER = new AgingPager(frame_table);
// Pager* THE_PAGER = new WorkingSetPager(frame_table, processes);
// Pager* THE_PAGER = new NRUPager(frame_table, processes);


frame_t* get_frame(std::vector<frame_t>& frame_table, std::vector<Process>& processes) {
    frame_t* frame;
    if (!free_frames.empty()) {
        frame = free_frames.front();
        free_frames.pop_front();
    } else {
        frame = THE_PAGER->select_victim_frame(frame_table, processes, inst_count);
    }
    return frame;
}


void load_processes(std::vector<Process>& processes, std::vector<frame_t>& frame_table, std::ifstream& file) {
    // std::ifstream file(filename);
    std::string line;
    
    // Skip the header and other non-essential information
    while (getline(file, line) && (line[0] == '#' || line.empty() || std::all_of(line.begin(), line.end(), isspace))) {
        // Just skip all comment and empty lines at the beginning
    }

    int num_processes;
    // First non-comment, non-empty line should be the number of processes
    if (!line.empty()) {
        try {
            num_processes = std::stoi(line);
            processes.reserve(num_processes);
            // std::cout << "Number of processes to load: " << num_processes << std::endl;
        } catch (const std::invalid_argument& e) {
            // std::cerr << "Invalid number format found, expected number of processes: " << line << std::endl;
            return; // Exit if we can't find a valid number of processes
        }
    }

    for (int i = 0; i < num_processes; ++i) {
        // Skipping until we find the process header
        while (getline(file, line) && (line.empty() || line[0] == '#' || !isdigit(line[0]))) {
            // Skip empty lines and comments until we find a numeric line which should indicate the start of VMA data
        }

        int pid = i; // Assign a process ID if not specified in the file
        int num_vmas = std::stoi(line);
        Process process(pid);
        // std::cout << "Reading VMAs for process " << pid << std::endl;

        for (int j = 0; j < num_vmas; ++j) {
            if (getline(file, line) && !line.empty() && line[0] != '#') {
                std::istringstream vma_stream(line);
                int start_vpage, end_vpage;
                bool write_protected, file_mapped;
                if (vma_stream >> start_vpage >> end_vpage >> write_protected >> file_mapped) {
                    process.addVMA(start_vpage, end_vpage, write_protected, file_mapped);
                    // std::cout << "VMA added: " << start_vpage << "-" << end_vpage << std::endl;
                } else {
                    // std::cerr << "Invalid VMA line: " << line << std::endl;
                }
            }
        }
        processes.push_back(process);

    }

}


bool get_next_instruction(std::ifstream& file, char& operation, int& vpage) {
    std::string line;
    while (getline(file, line)) {
        if (!line.empty() && line[0] == '#') {
            continue;
        }
        std::istringstream iss(line);
        if (iss >> operation >> vpage) {
            return true; // Successfully parsed an instruction
        }
    }
    return false; // No more instructions
}

// function to check if a virtual page is valid for the current process
bool isValidPage(int vpage, const Process* proc) {
    


    for (const auto& vma : proc->vmas) {
        if (vpage >= vma.start_vpage && vpage <= vma.end_vpage) {
            return true;
        }
    }
    return false;
}

bool isNewPage(int vpage, Process* process) {
    // Check the page table entry for the given virtual page
    pte_t* pte = &process->page_table[vpage];

    // Determine if the page is new by checking if it has never been marked present
    // and it has no record of being swapped out or file-mapped
    return !pte->present && !pte->paged_out;
}

void printFrameTable(const std::vector<frame_t>& frame_table) {
    std::cout << "FT: ";
    for (size_t i = 0; i < frame_table.size(); ++i) {
        const auto& frame = frame_table[i];
        if (frame.process_id != -1) {
            if (i == frame_table.size() - 1) {
                std::cout << frame.process_id << ":" << frame.virtual_page;
            } else {
                std::cout << frame.process_id << ":" << frame.virtual_page << " ";
            }
        } else {
            if (i == frame_table.size() - 1) {
                std::cout << "*"; 
            } else {
                std::cout << "* ";
            }
        }
    }
    std::cout << std::endl;
}


void out(frame_t* frame, Process& process) {
    pte_t& pte = process.page_table[frame->virtual_page];
    if (frame->dirty && frame->virtual_page != -1) {
        if (pte.file_mapped) {
            if (o_flag) std::cout << " FOUT" << std::endl;
            process.stats.fouts ++;

        } else {
            if (o_flag) std::cout << " OUT" << std::endl;
            pte.paged_out = 1;
            process.stats.outs ++;
        }
        frame->dirty = false;
    }
}

bool exitloop = false;

void unmap(frame_t* frame, Process& process) {
    if (frame->virtual_page != -1) {
        process.stats.unmaps ++;
        pte_t& pte = process.page_table[frame->virtual_page];
        if (o_flag) std::cout << " UNMAP " << process.pid << ":" << frame->virtual_page << std::endl;
        if (frame->dirty & !exitloop) {
            out(frame, process);
        }
        frame->process_id = -1;
        frame->virtual_page = -1;
        frame->dirty = false;
        // frame->resetAge();
        pte.present = 0;
        pte.modified = 0;
        pte.frame_number = 0;  // Optionally clear the frame number
    }
}

void unmap2(frame_t* frame, Process& process) {
    if (frame->virtual_page != -1) {
        process.stats.unmaps ++;

        pte_t& pte = process.page_table[frame->virtual_page];
        if (o_flag) std::cout << " UNMAP " << process.pid << ":" << frame->virtual_page << std::endl;
        // if (frame->dirty & !exitloop) {
        //     out(frame, process);
        // }
        
    // pte_t& pte = process.page_table[frame->virtual_page];
    if (frame->dirty && frame->virtual_page != -1) {
        if (pte.file_mapped) {

            if (o_flag) std::cout << " FOUT" << std::endl;
            process.stats.fouts ++;
        }
        frame->dirty = false;
    }
        frame->process_id = -1;
        frame->virtual_page = -1;
        frame->dirty = false;
        // frame->resetAge();
        pte.present = 0;
        pte.modified = 0;
        pte.frame_number = 0;  // Optionally clear the frame number
    }
}


void map(std::vector<frame_t>& frame_table, frame_t* frame, int vpage, Process& process) {
    pte_t& pte = process.page_table[vpage];

    // Set up the frame
    process.stats.maps ++;

    frame->process_id = process.pid;
    frame->virtual_page = vpage;
    // frame->age = 0; 
    frame->resetAge();

    // Update the PTE
    pte.frame_number = static_cast<int>(frame - frame_table.data());
    pte.present = 1;

    int frame_index = frame - &frame_table[0];

    if (o_flag) std::cout << " MAP " << frame_index << std::endl;
}


void zero(frame_t* frame, Process& process) {
    // Assume frame has a method to clear its content
    frame->clear();
    process.stats.zeros ++;
    if (o_flag) std::cout << " ZERO" << std::endl;
}

void in(frame_t* frame, int vpage, Process& process) {
    process.stats.ins ++;
    if (o_flag) std::cout << " IN" << std::endl;
    frame->dirty = false; // Reset the dirty bit when a page is brought in
}

void fin(frame_t* frame, int vpage, Process& process) {
    process.stats.fins ++;
    if (o_flag) std::cout << " FIN" << std::endl;
    frame->dirty = false; // Reset the dirty bit when a page is brought in from a file
}


void handle_page_fault(std::vector<frame_t>& frame_table, std::vector<Process>& processes, Process& current_process, int vpage) {
    pte_t& pte = current_process.page_table[vpage];
    bool valid = false;
    for (const auto& vma : current_process.vmas) {
        if (vpage >= vma.start_vpage && vpage <= vma.end_vpage) {
            valid = true;
            pte.file_mapped = vma.file_mapped;  // Set file_mapped directly from VMA when checking validity
            break;
        }
    }

    frame_t* frame = get_frame(frame_table, processes);
    if (frame->process_id != -1 && frame->virtual_page != -1) {
        Process& owning_process = processes[frame->process_id];
        unmap(frame, owning_process);
        out(frame, owning_process);  // Handles OUT or FOUT depending on file_mapped
        // std::cout<< "Yayyyyy"<<std::endl;
    }

    if (pte.file_mapped) {
        fin(frame, vpage, current_process);
    } else if (pte.paged_out) {
        in(frame, vpage, current_process);
    } else {
        zero(frame, current_process);
    }

    map(frame_table, frame, vpage, current_process);
    pte.present = 1;
    pte.frame_number = static_cast<int>(frame - frame_table.data());
}

void process_exit(Process& process, std::vector<frame_t>& frame_table) {
    exitloop = true;
    if (o_flag) std::cout << "EXIT current process " << process.pid << std::endl;
    for (auto& pte : process.page_table) {
        pte.paged_out = 0;
        if (pte.present) {
            frame_t* frame = &frame_table[pte.frame_number];
            unmap2(frame, process);  // Ensure that frame is unmapped
            free_frames.push_back(frame);  // Return to free pool
            exitloop = false;
        }
    }
}

unsigned long long cost = 0;
unsigned long ctx_switches = 0, process_exits = 0;
unsigned long rwcount = 0;
// unsigned long index = 0;

void simulate(std::vector<Process>& processes, std::vector<frame_t>& frame_table, std::ifstream& file) {
    char command;
    int vpage;
    Process* currentProcess = nullptr;

    while (get_next_instruction(file, command, vpage)) {
        if (o_flag) std::cout << inst_count << ": ==> " << command << " " << vpage << std::endl;
        // index++;
        if (command == 'c') {
            inst_count ++;
            if (vpage >= 0 && vpage < processes.size()) {
                currentProcess = &processes[vpage];
                ctx_switches++;
                // cost += 130;
                // std::cout << "Switched to process " << vpage << std::endl;
            }
        }
        else if (command == 'e') {
            inst_count ++;
            if (currentProcess && vpage < processes.size() && vpage >= 0) {
                process_exit(processes[vpage], frame_table); // Call the process exit function
                process_exits++;
                // std::cout << "EXIT current process " << vpage << std::endl;
            }
        }
        
        else if (currentProcess && !isValidPage(vpage, currentProcess)) {
            inst_count ++;
            rwcount++;
            if (o_flag) std::cout << " SEGV" << std::endl;
            currentProcess->stats.segv ++;
            continue;
        } else if (command == 'r' || command == 'w') {
            inst_count++;
            rwcount++;
            pte_t& pte = currentProcess->page_table[vpage];
            if (!pte.present) {
                handle_page_fault(frame_table, processes, *currentProcess, vpage);
                // cost += 350; // Cost for handling page fault
            }
            pte.referenced = 1;
            if (command == 'w') {
                if (!pte.write_protect) {
                    pte.modified = 1;
                    frame_table[pte.frame_number].dirty = true;  // Set the dirty flag on the frame
                } else {
                    if (o_flag) std::cout << " SEGPROT" << std::endl;
                    currentProcess->stats.segprot ++;
                }
            }
        }
        // printFrameTable(frame_table);
    }
}


void printPageTables(const std::vector<Process>& processes) {
    for (const auto& proc : processes) {
        std::cout << "PT[" << proc.pid << "]: ";
        for (int i = 0; i < NUM_VIRTUAL_PAGES; ++i) {
            const auto& pte = proc.page_table[i];
            if (pte.present) { // Check if the page is present
            if (i == NUM_VIRTUAL_PAGES - 1){
                std::cout << i << ":";
                std::cout << (pte.referenced ? "R" : "-");
                std::cout << (pte.modified ? "M" : "-");
                std::cout << (pte.paged_out ? "S" : "-");
            }
            else{
                std::cout << i << ":";
                std::cout << (pte.referenced ? "R" : "-");
                std::cout << (pte.modified ? "M" : "-");
                std::cout << (pte.paged_out ? "S" : "-") << " ";
            }
            } else {
                // Display '#' if the page was swapped out but not currently present
                // Display '*' if the page was never assigned or it was never swapped out
                if( i == NUM_VIRTUAL_PAGES - 1){
                    std::cout << (pte.paged_out ? "#" : "*");
                }
                else{
                    std::cout << (pte.paged_out ? "#" : "*") << " ";
                }
                
            }
        }
        std::cout << std::endl;
    }
}


void printProcessStats(const std::vector<Process>& processes) {
    for (const auto& proc : processes) {
        printf("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n",
               proc.pid,
               proc.stats.unmaps, proc.stats.maps, proc.stats.ins, proc.stats.outs,
               proc.stats.fins, proc.stats.fouts, proc.stats.zeros,
               proc.stats.segv, proc.stats.segprot);
    }
}

void printSimulationSummary(const std::vector<Process>& processes,
                            unsigned long inst_count,
                            unsigned long ctx_switches,
                            unsigned long process_exits,
                            unsigned long long total_cost) {
    // unsigned long long total_cost = 0;
    // for (const auto& process : processes) {
    //     total_cost += process.stats.computeTotalCost();
    // }
    printf("TOTALCOST %lu %lu %lu %llu %lu\n",
           inst_count, ctx_switches, process_exits, total_cost, sizeof(pte_t));
}

// Function to check if 'str' contains the substring 'substring'
bool containsSubstring(const std::string& str, const std::string& substring) {
    return str.find(substring) != std::string::npos;
}


int main(int argc, char* argv[]) {
    // int numFrames = 128;  // Default number of frames
    std::string algorithm;
    std::string options;

    FIFOPager pager; // Use the appropriate pager based on your needs

    int opt;
    while ((opt = getopt(argc, argv, "f:a:o:")) != -1) {
        switch (opt) {
            case 'f':
                numFrames = std::stoi(optarg);
                if (numFrames <= 0 || numFrames > 128) {
                    std::cerr << "Number of frames must be between 1 and 128.\n";
                    exit(EXIT_FAILURE);
                }
                break;
            case 'a':
                algorithm = optarg;
                break;
            case 'o':
                options = optarg;
                break;
            default: // '?'
                std::cerr << "Usage: " << argv[0] << " -f<num_frames> -a<algo> [-o<options>] inputfile randomfile\n";
                exit(EXIT_FAILURE);
        }
    }

    if (argc - optind != 2) {
        std::cerr << "Expected inputfile and randomfile after options\n";
        exit(EXIT_FAILURE);
    }


    std::string inputFile, randomFile;

    inputFile = argv[optind];
    randomFile = argv[optind + 1];

    // Read random numbers
    // string randomFile = argv[optind + 1]; // Assuming random file is the second non-option argument
    readRandomNumbers(randomFile);

    std::vector<frame_t> frame_table(numFrames);
    // std::cout << "Frame table initialized with size: " << frame_table.size() << std::endl;

    std::string filename = inputFile;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return 1;
    }
    // Initialize the free frames deque with all frames as available
    for (int i = 0; i < numFrames; ++i) {
        free_frames.push_back(&frame_table[i]);
    }


    // Now proceed with the rest of your main function logic
    load_processes(processes, frame_table, file);

    // Instantiate the Pager Replacement Algorithm
    if(algorithm == "a"){
        THE_PAGER = new AgingPager(frame_table);
    }else if (algorithm == "c"){
        THE_PAGER = new ClockPager(frame_table);
    } else if (algorithm == "e"){
        THE_PAGER = new NRUPager(frame_table, processes);     
    } else if (algorithm == "f"){
        THE_PAGER = new FIFOPager();
    } else if (algorithm == "r"){
        THE_PAGER = new RandomPager();
    } else if (algorithm == "w"){
        THE_PAGER = new WorkingSetPager(frame_table, processes);
    } else{
        std::cout<< "Allowed page replacement algorithm calls are : a/c/e/f/r/w";
    }

    if (containsSubstring(options, "O")){
        o_flag = true;
    }
    
    simulate(processes, frame_table, file);
    
    file.close();


    // Compute total cost here if not done elsewhere
    unsigned long long total_cost = 0; // You should calculate this based on your statistics
    for (const auto& proc : processes) {
        total_cost += proc.stats.computeTotalCost();
    }
    // Cost for each instruction
    total_cost += rwcount;
    
    // Cost for context switches
    total_cost += ctx_switches * 130;
    
    // Cost for process exits
    total_cost += process_exits * 1230;

    if (containsSubstring(options, "P")){
        printPageTables(processes);
    }
    if (containsSubstring(options, "F")){
        printFrameTable(frame_table);
    }
    if (containsSubstring(options, "S")){
        printProcessStats(processes);
        printSimulationSummary(processes, inst_count, ctx_switches, process_exits, total_cost);
    }

    return 0;
}

