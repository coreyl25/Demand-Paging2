#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAX_FRAMES 100
#define MAX_REFERENCES 10000

typedef struct {
    int page_number;      // -1 if frame is empty
    bool reference_bit;   // Reference (use) bit
    bool dirty_bit;       // Modified bit
} Frame;

typedef struct {
    Frame frames[MAX_FRAMES];
    int num_frames;
    int clock_hand;
    int page_faults;
    int page_replacements;
    int disk_writes;
} ClockPageReplacement;

// Initialize the Clock algorithm
ClockPageReplacement* init_clock(int num_frames) {
    ClockPageReplacement *clock = malloc(sizeof(ClockPageReplacement));
    
    clock->num_frames = num_frames;
    clock->clock_hand = 0;
    clock->page_faults = 0;
    clock->page_replacements = 0;
    clock->disk_writes = 0;
    
    // Initialize all frames as empty
    for (int i = 0; i < num_frames; i++) {
        clock->frames[i].page_number = -1;
        clock->frames[i].reference_bit = false;
        clock->frames[i].dirty_bit = false;
    }
    
    return clock;
}

// Check if page is in memory
int find_page(ClockPageReplacement *clock, int page_number) {
    for (int i = 0; i < clock->num_frames; i++) {
        if (clock->frames[i].page_number == page_number) {
            return i;
        }
    }
    return -1; 
}

// Find first empty frame
int find_empty_frame(ClockPageReplacement *clock) {
    for (int i = 0; i < clock->num_frames; i++) {
        if (clock->frames[i].page_number == -1) {
            return i;
        }
    }
    return -1;
}

// Clock algorithm to find victim page
int find_victim(ClockPageReplacement *clock) {
    int start_position = clock->clock_hand;
    
    // Keep sweeping page with reference bit = 0 is found
    while (true) {
        Frame *current = &clock->frames[clock->clock_hand];
        
        if (!current->reference_bit) {
            // Victim page
            int victim = clock->clock_hand;
            clock->clock_hand = (clock->clock_hand + 1) % clock->num_frames;
            return victim;
        }
        
        current->reference_bit = false;
        clock->clock_hand = (clock->clock_hand + 1) % clock->num_frames;
    }
}

// Access a page
void access_page(ClockPageReplacement *clock, int page_number, bool is_write) {
    // Check if page is already in memory
    int frame_index = find_page(clock, page_number);
    
    if (frame_index != -1) {
        // Page hit
        clock->frames[frame_index].reference_bit = true;
        if (is_write) {
            clock->frames[frame_index].dirty_bit = true;
        }
        return;
    }
    
    // Page fault occurred
    clock->page_faults++;
    
    // Try to find empty frame first
    frame_index = find_empty_frame(clock);
    
    if (frame_index == -1) {
        // No empty frame
        frame_index = find_victim(clock);
        
        // Check if victim page is dirty
        if (clock->frames[frame_index].dirty_bit) {
            clock->disk_writes++;
        }
        
        clock->page_replacements++;
    }
    
    // Load new page into frame
    clock->frames[frame_index].page_number = page_number;
    clock->frames[frame_index].reference_bit = true;
    clock->frames[frame_index].dirty_bit = is_write;
}

// Print current state of frames
void print_frames(ClockPageReplacement *clock) {
    printf("Current frames: [");
    for (int i = 0; i < clock->num_frames; i++) {
        if (clock->frames[i].page_number == -1) {
            printf(" - ");
        } else {
            printf("%2d", clock->frames[i].page_number);
            if (clock->frames[i].reference_bit) printf("R");
            if (clock->frames[i].dirty_bit) printf("D");
        }
        
        if (i == clock->clock_hand) {
            printf("* ");  // Mark clock hand position
        } else {
            printf("  ");
        }
        
        if (i < clock->num_frames - 1) printf("| ");
    }
    printf("]\n");
}

// Print statistics
void print_statistics(ClockPageReplacement *clock, int total_accesses) {
    printf("\n=== Clock Algorithm Statistics ===\n");
    printf("Total memory accesses: %d\n", total_accesses);
    printf("Page faults: %d\n", clock->page_faults);
    printf("Page replacements: %d\n", clock->page_replacements);
    printf("Disk writes (dirty pages): %d\n", clock->disk_writes);
    
    if (total_accesses > 0) {
        double fault_rate = (double)clock->page_faults / total_accesses * 100;
        printf("Page fault rate: %.2f%%\n", fault_rate);
    }
}

// Read page reference string from file
int read_references_from_file(const char *filename, int *references, 
                              bool *is_write, int max_refs) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        return -1;
    }
    
    int count = 0;
    char operation;
    int page;
    
    while (count < max_refs && fscanf(file, " %c %d", &operation, &page) == 2) {
        references[count] = page;
        is_write[count] = (operation == 'W' || operation == 'w');
        count++;
    }
    
    fclose(file);
    return count;
}

int main(int argc, char *argv[]) {
    printf("Clock Page Replacement Algorithm Simulator\n");
    
    int num_frames = 4;  // Default
    bool verbose = false;
    char *input_file = NULL;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            num_frames = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-v") == 0) {
            verbose = true;
        } else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            input_file = argv[++i];
        }
    }
    
    printf("Number of frames: %d\n\n", num_frames);
    
    ClockPageReplacement *clock = init_clock(num_frames);
    
    int references[MAX_REFERENCES];
    bool is_write[MAX_REFERENCES];
    int num_references;
    
    // Try to read from file, otherwise use default test sequence
    if (input_file) {
        num_references = read_references_from_file(input_file, references, 
                                                   is_write, MAX_REFERENCES);
        if (num_references < 0) {
            printf("Error: Could not open file %s\n", input_file);
            printf("Using default test sequence instead.\n\n");
            input_file = NULL;
        } else {
            printf("Loaded %d page references from %s\n\n", 
                   num_references, input_file);
        }
    }
    
    // Default test sequence if no file
    if (!input_file) {
        // Test sequence with read/write operations
        int default_refs[] = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5, 6, 7, 8, 9, 
                             7, 8, 9, 2, 3, 1};
        bool default_writes[] = {false, false, true, false, false, true, false,
                               false, false, true, false, false, false, true,
                               false, false, false, true, false, false, true, false};
        num_references = sizeof(default_refs) / sizeof(default_refs[0]);
        
        for (int i = 0; i < num_references; i++) {
            references[i] = default_refs[i];
            is_write[i] = default_writes[i];
        }
    }
    
    printf("Processing page references...\n");
    if (verbose) {
        printf("\n%-6s %-6s %-10s %s\n", "Step", "Page", "Operation", "Frames");
    }
    
    for (int i = 0; i < num_references; i++) {
        int page = references[i];
        bool write = is_write[i];
        
        int faults_before = clock->page_faults;
        
        access_page(clock, page, write);
        
        if (verbose) {
            printf("%-6d %-6d %-10s ", i + 1, page, write ? "WRITE" : "READ");
            
            if (clock->page_faults > faults_before) {
                printf("FAULT ");
            } else {
                printf("HIT   ");
            }
            
            print_frames(clock);
        }
    }
    
    if (!verbose) {
        printf("\nFinal memory state:\n");
        print_frames(clock);
    }
    
    print_statistics(clock, num_references);
    
    free(clock);
    
    return 0;
}