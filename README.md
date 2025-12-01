# OS Assignment: Compilation and Testing Instructions

## Project Structure

Create the following directory structure in your GitHub repository:

```
os-virtual-memory-assignment/
├── README.md
├── Makefile
├── src/
│   ├── tlb_pagetable.c
│   ├── clock_replacement.c
│   └── (bonus files if applicable)
├── test/
│   ├── page_references.txt
│   ├── page_references_large.txt
│   └── test_all.sh
├── docs/
│   ├── answers.md
│   └── diagrams/
└── results/
    └── performance_analysis.txt
```

## Compilation Instructions

make all


## Testing Instructions

### Task 1: TLB and Two-Level Page Table

**Basic Test:**
```bash
./build/tlb_pagetable
```


### Task 2: Clock Page Replacement

**Basic Test (Default Sequence):**
```bash
./build/clock_replacement -f 4
```

**Verbose Mode:**
```bash
./build/clock_replacement -f 4 -v
```

**Test with Custom Input File:**
Run:
```bash
./build/clock_replacement -f 4 -i test/page_references.txt -v
```