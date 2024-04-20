
# Virtual Memory Management

## Project Overview

This project presents an advanced simulation of a Virtual Memory Manager (VMM) as part of an operating system's memory management subsystem. The simulation meticulously models the intricate processes involved in virtual-to-physical address translation, page replacement, and memory operations handling multiple processes. It is designed as a robust educational tool for students and professionals interested in deepening their understanding of operating systems, particularly in how they manage memory resources efficiently when faced with hardware constraints.

## Key Features

- **Multi-Process Simulation:** Handles multiple concurrent processes, each with its own segmented virtual address space.
- **Complex Memory Operations:** Simulates intricate memory operations including reads, writes, page faults, and context switches.
- **Diverse Page Replacement Algorithms:** Implements a range of algorithms from simple FIFO to more complex ones like Aging and Working Set, providing a comparative understanding of their efficiency in different scenarios.
- **Dynamic Memory Allocation:** Demonstrates the allocation and deallocation of memory across various processes, reflecting real-world operating system behavior.
- **Detailed Simulation Outputs:** Generates detailed logs and output metrics that can be used for in-depth analysis and academic reporting.

## Objectives

The primary educational objectives of this project are:
- To provide a hands-on experience in implementing and understanding key concepts of virtual memory management.
- To facilitate comparative studies of different page replacement algorithms under a controlled simulation environment.
- To develop an appreciation of the complexities involved in modern operating systems, particularly in their memory management modules.

## Technical Specifications

### Input Specification

Inputs to the program include:
- **Number of Processes:** Specifies how many separate processes the simulation will manage.
- **Virtual Memory Areas (VMAs):** Each process may have multiple VMAs defining attributes such as range of virtual pages, write protection, and file-mapping status.

### Core Concepts

#### Page Table Entries (PTEs)
PTEs play a crucial role in the simulation, with each entry containing flags for:
- **Presence:** Whether the corresponding page is in the physical memory.
- **Referenced and Modified:** Tracked for deciding victim pages in replacement algorithms.
- **Write Protection and Paged Out:** Additional attributes that affect how memory operations are processed.

#### Frame Table
A global frame table maintains a reverse mapping from physical frames back to the virtual pages and processes, essential for managing physical memory effectively.

### Page Replacement Algorithms
- **FIFO (First-In-First-Out)**
- **Random**
- **Clock**
- **Enhanced Second Chance / Not Recently Used (ESC/NRU)**
- **Aging**
- **Working Set**
Each algorithm is implemented with an eye toward real-world application, focusing on their strengths and weaknesses in various operational contexts.

## Building and Running the Simulation

### Compiling the Code
Ensure you have a C/C++ compiler like GCC installed. Compile the project using the provided Makefile:

\```bash
make all
\```

### Execution
Run the simulation with the following command structure:

\```bash
./mmu -f<num_frames> -a<algo> [-o<options>] inputfile randomfile
\```

`<num_frames>` specifies the number of physical frames available, `<algo>` denotes the page replacement algorithm, and `<options>` can be used to generate detailed outputs for debugging or analysis.

workings of an operating system's memory management subsystem.

## Input Format

The simulation initializes based on a structured input format that defines the memory management scenarios for multiple processes:

- **Number of Processes:** The first line of the input specifies the total number of processes.
- **Virtual Memory Areas (VMAs):** For each process, the VMAs are specified, which include:
  - **Start and end virtual page:** The range of virtual pages covered by this VMA.
  - **Write protection (0 or 1):** Whether the pages are write-protected.
  - **File-mapped (0 or 1):** Whether the pages are backed by a file.

This input format is critical for setting up the simulation environment, ensuring each process and its memory requirements are accurately represented.


## Simulation Output and Analysis

The output from the simulation includes per-process memory operation metrics, state of the page and frame tables, and detailed statistics that track the effectiveness of different memory management strategies. These outputs are crucial for:
- Analyzing the behavior of different page replacement algorithms.
- Understanding the impact of memory management on process performance.
- Developing strategies for optimizing memory usage in operating systems.

## Detailed Simulation Output Explanation

To explain the output of the simulation in detail, we need to consider how the virtual memory management system handles and records its operations throughout the execution of your program. This includes tracking page accesses, page faults, replacements, and other memory-related events. Here’s a detailed description of the typical outputs you might expect and how to interpret them:

### 1. **Page Table Output (PT)**
Every process in the simulation maintains a page table, which is a critical data structure for virtual memory management. The page table output shows the state of each page table entry (PTE) at specific points in the simulation, typically after all instructions have been executed.

- **Format:** The page table entries for each process might be displayed as follows:
  ```
  PT[0]: 0:RMS 1:RMS 2:RMS 3:R-S 4:R-S 5:RMS 6:R-S 7:R-S 8:RMS 9:R-S 10:RMS ...
  ```
  Each entry indicates the virtual page index followed by the status bits:
  - **R (Referenced):** Indicates that the page has been accessed.
  - **M (Modified):** Indicates that the page has been written to.
  - **S (Swapped out):** Indicates that the page has been swapped out to disk.
  Entries not currently valid (not in physical memory) are marked with `#` if they have been swapped out, or `*` if they never have been swapped or are not assigned any physical frame.

### 2. **Frame Table Output (FT)**
The frame table output provides a snapshot of the physical memory allocation at a particular time. It shows which virtual page of which process is mapped to each physical frame:
  ```
  FT: 0:32 0:42 0:4 1:8 * 0:39 0:3 0:44 1:19 0:29 1:61 * 1:58 0:6 0:27 1:34 ...
  ```
  Each entry in the frame table consists of:
  - **Frame Number:** The index of the frame in physical memory.
  - **Process:Virtual Page:** Indicates which process and virtual page are mapped to the frame. If a frame is currently not mapped, it is denoted by `*`.

### 3. **Simulation Statistics and Events**
The output also includes statistics and specific events that occurred during the simulation, such as:
- **Number of Page Faults:** Total page faults that occurred for each process.
- **Number of Writes, Reads, and Other Memory Operations:** Counts how many times these operations were performed.
- **Context Switches and Process Exits:** How many times control was transferred between processes and how many processes have completed and exited.

### 4. **Event-Specific Outputs**
Depending on the page replacement algorithm and simulation options specified (`-o` flag), the output might detail every significant event, such as:
- **UNMAP:** When a page is removed from a frame.
- **OUT:** When a modified page is written to disk before being removed from a frame.
- **IN:** When a page is loaded from disk into a frame.
- **ZERO:** When a frame is assigned to a page that has not been initialized yet (zeroed out).
  ```
  39: ==> r 5
  UNMAP 1:41
  OUT
  IN
  MAP 26
  ```

### 5. **Summary Output**
Finally, a summary of the total cost of operations in terms of cycles might be displayed, along with the count of different operations:
  ```
  TOTALCOST 31 1 0 52951 4
  ```
  This provides:
  - The total number of instructions processed.
  - The number of context switches.
  - The number of process exits.
  - The total cost in cycles.
  - The size of a page table entry.

### Conclusion
The detailed outputs from the simulation provide deep insights into how virtual memory management operates under various conditions and algorithms. These outputs are invaluable for debugging, performance tuning, and educational purposes, offering a clear view of the inner workings of an operating system's memory management subsystem.

## License
This project is open-sourced under the MIT License. See the `LICENSE` file for more details.

