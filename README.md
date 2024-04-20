
# Virtual Memory Management

## Project Overview

This project presents an advanced simulation of a Virtual Memory Manager (VMM) as part of an operating system's memory management subsystem. The simulation meticulously models the intricate processes involved in virtual-to-physical address translation, page replacement, and memory operations handling multiple processes. It is designed as a robust educational tool for students and professionals interested in deepening their understanding of operating systems, particularly in how they manage memory resources efficiently when faced with hardware constraints.

## Key Features

- **Multi-Process Simulation:** Handles multiple concurrent processes, each with its own segmented virtual address space.
- **Complex Memory Operations:** Simulates intricate memory operations including reads, writes, page faults, and context switches.
- **Diverse Page Replacement Algorithms:** Implements a range of algorithms from simple FIFO to more complex ones like Aging and Working Set, providing a comparative understanding of their efficiency in different scenarios.
- **Dynamic Memory Allocation:** Demonstrates the allocation and deallocation of memory across various processes, reflecting real-world operating system behavior.
- **Detailed Simulation Outputs:** Generates detailed logs and output metrics that can be used for in-depth analysis and academic reporting.

## Educational Objectives

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

## Simulation Output and Analysis

The output from the simulation includes per-process memory operation metrics, state of the page and frame tables, and detailed statistics that track the effectiveness of different memory management strategies. These outputs are crucial for:
- Analyzing the behavior of different page replacement algorithms.
- Understanding the impact of memory management on process performance.
- Developing strategies for optimizing memory usage in operating systems.

## Conclusion

This project serves as a comprehensive platform for exploring and understanding the critical functions of virtual memory management in operating systems. It offers valuable insights into the challenges and solutions in efficient memory management, making it an ideal educational resource for advanced learners and professionals.

## License
This project is open-sourced under the MIT License. See the `LICENSE` file for more details.
