#  Virtual Memory Concepts and Challenges

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
