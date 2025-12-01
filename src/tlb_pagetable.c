#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#define PAGE_SIZE 4096
#define OUTER_PAGE_BITS 10
#define INNER_PAGE_BITS 10
#define OFFSET_BITS 12
#define TLB_SIZE 16

#define OUTER_MASK 0xFFC00000  // Top 10 bits
#define INNER_MASK 0x003FF000  // Middle 10 bits
#define OFFSET_MASK 0x00000FFF // Bottom 12 bits

typedef struct {
    bool valid;
    uint32_t vpn;           // Virtual page number (20 bits)
    uint32_t frame_number;  // Physical frame number
    uint64_t access_time;   // For LRU replacement
} TLBEntry;

typedef struct {
    bool present;
    uint32_t frame_number;
    bool dirty;
    bool referenced;
} PageTableEntry;

typedef struct {
    PageTableEntry *entries;  // Array of inner page table entries
} InnerPageTable;

typedef struct {
    InnerPageTable **outer_table;  // Array of pointers to inner tables
    TLBEntry tlb[TLB_SIZE];
    uint64_t tlb_hits;
    uint64_t tlb_misses;
    uint64_t page_table_accesses;
    uint64_t access_counter;
} TwoLevelPageTable;

// Initialize the two-level page table
TwoLevelPageTable* init_page_table() {
    TwoLevelPageTable *pt = malloc(sizeof(TwoLevelPageTable));
    
    // Allocate outer page table (1024 entries)
    pt->outer_table = calloc(1024, sizeof(InnerPageTable*));
    
    // Initialize TLB
    for (int i = 0; i < TLB_SIZE; i++) {
        pt->tlb[i].valid = false;
        pt->tlb[i].access_time = 0;
    }
    
    pt->tlb_hits = 0;
    pt->tlb_misses = 0;
    pt->page_table_accesses = 0;
    pt->access_counter = 0;
    
    return pt;
}

// Extract page number components from virtual address
void extract_address_components(uint32_t virtual_addr, 
                                uint32_t *outer, 
                                uint32_t *inner, 
                                uint32_t *offset) {
    *outer = (virtual_addr & OUTER_MASK) >> (INNER_PAGE_BITS + OFFSET_BITS);
    *inner = (virtual_addr & INNER_MASK) >> OFFSET_BITS;
    *offset = virtual_addr & OFFSET_MASK;
}

// Look up address in TLB
bool tlb_lookup(TwoLevelPageTable *pt, uint32_t vpn, uint32_t *frame) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (pt->tlb[i].valid && pt->tlb[i].vpn == vpn) {
            *frame = pt->tlb[i].frame_number;
            pt->tlb[i].access_time = pt->access_counter++;
            pt->tlb_hits++;
            return true;
        }
    }
    pt->tlb_misses++;
    return false;
}

// Update TLB with new translation
void tlb_update(TwoLevelPageTable *pt, uint32_t vpn, uint32_t frame) {
    int replace_index = -1;
    
    // Find an invalid entry
    for (int i = 0; i < TLB_SIZE; i++) {
        if (!pt->tlb[i].valid) {
            replace_index = i;
            break;
        }
    }
    
    // If no invalid entry, use LRU
    if (replace_index == -1) {
        uint64_t oldest_time = pt->tlb[0].access_time;
        replace_index = 0;
        for (int i = 1; i < TLB_SIZE; i++) {
            if (pt->tlb[i].access_time < oldest_time) {
                oldest_time = pt->tlb[i].access_time;
                replace_index = i;
            }
        }
    }
    
    pt->tlb[replace_index].valid = true;
    pt->tlb[replace_index].vpn = vpn;
    pt->tlb[replace_index].frame_number = frame;
    pt->tlb[replace_index].access_time = pt->access_counter++;
}

// Translate virtual address to physical address
uint32_t translate_address(TwoLevelPageTable *pt, uint32_t virtual_addr, 
                           bool *page_fault) {
    uint32_t outer_index, inner_index, offset;
    extract_address_components(virtual_addr, &outer_index, &inner_index, &offset);
    
    uint32_t vpn = (virtual_addr >> OFFSET_BITS) & 0xFFFFF;
    uint32_t frame_number;
    
    // Check TLB first
    if (tlb_lookup(pt, vpn, &frame_number)) {
        *page_fault = false;
        return (frame_number << OFFSET_BITS) | offset;
    }
    
    // TLB miss - walk page table
    pt->page_table_accesses++;
    
    // Check if outer page table entry exists
    if (pt->outer_table[outer_index] == NULL) {
        *page_fault = true;
        return 0;
    }
    
    pt->page_table_accesses++;
    
    // Check if page is present
    PageTableEntry *pte = &pt->outer_table[outer_index]->entries[inner_index];
    if (!pte->present) {
        *page_fault = true;
        return 0;
    }
    
    frame_number = pte->frame_number;
    
    // Update TLB
    tlb_update(pt, vpn, frame_number);
    
    *page_fault = false;
    return (frame_number << OFFSET_BITS) | offset;
}

// Page mapping
void map_page(TwoLevelPageTable *pt, uint32_t virtual_addr, uint32_t frame) {
    uint32_t outer_index, inner_index, offset;
    extract_address_components(virtual_addr, &outer_index, &inner_index, &offset);
    
    // Allocate inner table if needed
    if (pt->outer_table[outer_index] == NULL) {
        pt->outer_table[outer_index] = malloc(sizeof(InnerPageTable));
        pt->outer_table[outer_index]->entries = calloc(1024, sizeof(PageTableEntry));
    }
    
    // Set up page table entry
    PageTableEntry *pte = &pt->outer_table[outer_index]->entries[inner_index];
    pte->present = true;
    pte->frame_number = frame;
    pte->dirty = false;
    pte->referenced = true;
}

// Print statistics
void print_statistics(TwoLevelPageTable *pt) {
    printf("\n Translation Statistics \n");
    printf("TLB Hits: %lu\n", pt->tlb_hits);
    printf("TLB Misses: %lu\n", pt->tlb_misses);
    
    if (pt->tlb_hits + pt->tlb_misses > 0) {
        double hit_rate = (double)pt->tlb_hits / (pt->tlb_hits + pt->tlb_misses) * 100;
        printf("TLB Hit Rate: %.2f%%\n", hit_rate);
    }
    
    printf("Page Table Accesses: %lu\n", pt->page_table_accesses);
}

// Free memory
void free_page_table(TwoLevelPageTable *pt) {
    for (int i = 0; i < 1024; i++) {
        if (pt->outer_table[i] != NULL) {
            free(pt->outer_table[i]->entries);
            free(pt->outer_table[i]);
        }
    }
    free(pt->outer_table);
    free(pt);
}

int main() {
    printf("Two-Level Page Table with TLB Simulator\n");
    
    TwoLevelPageTable *pt = init_page_table();
    
    // Set up some test mappings
    printf("Setting up test page mappings...\n");
    map_page(pt, 0x00000000, 100);  // Map virtual page 0 to frame 100
    map_page(pt, 0x00001000, 101);  // Map virtual page 1 to frame 101
    map_page(pt, 0x00400000, 200);  // Map a page in different outer table entry
    map_page(pt, 0x00800000, 300);
    map_page(pt, 0x10000000, 400);
    
    // Test addresses
    uint32_t test_addresses[] = {
        0x00000000,  // Page 0, offset 0
        0x00000ABC,  // Page 0, offset 0xABC (test TLB hit)
        0x00001000,  // Page 1, offset 0
        0x00001234,  // Page 1, offset 0x234 (test TLB hit)
        0x00000500,  // Page 0 again (test TLB hit)
        0x00400000,  // Different outer table entry
        0x00800000,  
        0x10000000,
        0x00002000,  // Unmapped page
        0x00001FFF   // Page 1, last byte (test TLB hit)
    };
    
    int num_tests = sizeof(test_addresses) / sizeof(test_addresses[0]);
    
    printf("\nTranslating addresses:\n");
    printf("%-12s %-12s %-12s %-10s\n", 
           "Virtual", "Physical", "Status", "TLB");
    
    for (int i = 0; i < num_tests; i++) {
        bool page_fault;
        uint64_t tlb_hits_before = pt->tlb_hits;
        
        uint32_t physical = translate_address(pt, test_addresses[i], &page_fault);
        
        bool was_tlb_hit = (pt->tlb_hits > tlb_hits_before);
        
        printf("0x%08X   ", test_addresses[i]);
        if (page_fault) {
            printf("PAGE FAULT   %-10s %-10s\n", "FAULT", "MISS");
        } else {
            printf("0x%08X   %-10s %-10s\n", 
                   physical, 
                   "SUCCESS", 
                   was_tlb_hit ? "HIT" : "MISS");
        }
    }
    
    print_statistics(pt);
    
    // Demonstrate address breakdown
    printf("\n=== Address Component Breakdown ===\n");
    uint32_t test_addr = 0x00401ABC;
    uint32_t outer, inner, offset;
    extract_address_components(test_addr, &outer, &inner, &offset);
    
    printf("Virtual Address: 0x%08X\n", test_addr);
    printf("  Outer Index: %u (bits 31-22)\n", outer);
    printf("  Inner Index: %u (bits 21-12)\n", inner);
    printf("  Offset: 0x%03X (bits 11-0)\n", offset);
    
    free_page_table(pt);
    
    return 0;
}