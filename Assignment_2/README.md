# Assignment 2: Dynamic Memory Allocation Algorithms

## Overview

This assignment implements and compares three popular dynamic memory allocation algorithms:
- **First Fit**
- **Best Fit**
- **Polling (Next Fit)**

It also demonstrates various heap crash scenarios including fragmentation errors, out-of-memory conditions, invalid free operations, and double-free detection.

---

## Memory Allocation Concepts

### What is a Heap?

The **heap** is a region of memory used for dynamic memory allocation. Unlike the stack (used for local variables), the heap allows programs to request and release memory at runtime. Memory is allocated in blocks of varying sizes as needed.

### Memory Fragmentation

**Fragmentation** occurs when free memory is broken into small, non-contiguous blocks. Even if the total free memory is sufficient, an allocation request might fail if no single contiguous block is large enough.

**Types of Fragmentation:**
- **External Fragmentation**: Free memory exists but is scattered in small chunks
- **Internal Fragmentation**: Allocated blocks are larger than requested, wasting space

### Coalescing

**Coalescing** is the process of merging adjacent free blocks into a single larger block. This reduces fragmentation and improves allocation success rates.

---

## Allocation Algorithms

### 1. First Fit

**Description**: Searches the heap from the beginning and allocates the **first free block** that is large enough to satisfy the request.

**Algorithm:**
```
For each block in the heap:
    If block is free AND block.size >= requested_size:
        Allocate from this block
        Return success
Return failure
```

**Advantages:**
- ✓ Fast allocation (stops at first match)
- ✓ Simple to implement
- ✓ Works well for most general-purpose scenarios

**Disadvantages:**
- ✗ Can cause fragmentation at the beginning of the heap
- ✗ May leave small unusable blocks at the start

**Time Complexity:** O(n) where n = number of blocks

---

### 2. Best Fit

**Description**: Searches the **entire heap** and allocates from the **smallest free block** that can satisfy the request.

**Algorithm:**
```
best_block = None
best_size = infinity

For each block in the heap:
    If block is free AND block.size >= requested_size:
        If block.size < best_size:
            best_block = block
            best_size = block.size

If best_block exists:
    Allocate from best_block
    Return success
Return failure
```

**Advantages:**
- ✓ Minimizes wasted space (tight fit)
- ✓ Reduces internal fragmentation
- ✓ Good for memory-constrained systems

**Disadvantages:**
- ✗ Slower than First Fit (must search entire heap)
- ✗ Can create many tiny unusable fragments
- ✗ May lead to more external fragmentation

**Time Complexity:** O(n) where n = number of blocks (must check all blocks)

---

### 3. Polling (Next Fit)

**Description**: Similar to First Fit, but starts searching from the **last allocation position** instead of the beginning. Essentially a "roving pointer" approach.

**Algorithm:**
```
starting_position = last_allocation_index

For offset from 0 to number_of_blocks:
    current_index = (starting_position + offset) % number_of_blocks
    block = blocks[current_index]
    
    If block is free AND block.size >= requested_size:
        Allocate from this block
        last_allocation_index = current_index + 1
        Return success
Return failure
```

**Advantages:**
- ✓ Distributes allocations more evenly across the heap
- ✓ Avoids clustering at the beginning
- ✓ Can be faster than First Fit in some cases

**Disadvantages:**
- ✗ May fragment the entire heap uniformly
- ✗ Harder to predict behavior
- ✗ Can skip over suitable earlier blocks

**Time Complexity:** O(n) where n = number of blocks

---

## Heap Crash Scenarios

### 1. Fragmentation Error

**Cause**: Total free memory is sufficient, but no single contiguous block is large enough.

**Example:**
```
Heap: [USED(20)] [FREE(20)] [USED(20)] [FREE(20)] [USED(20)]
Request: malloc(25)
Result: FAILURE - Total free = 40, but largest block = 20
```

### 2. Out of Memory (OOM)

**Cause**: Total free memory is less than the requested size.

**Example:**
```
Heap: [USED(45)] [FREE(5)]
Request: malloc(10)
Result: FAILURE - Only 5 bytes available
```

### 3. Invalid Free

**Cause**: Attempting to free a pointer that was never allocated or doesn't point to a block start.

**Example:**
```
heap.free(999)  # 999 is not a valid allocated pointer
Result: RuntimeError("INVALID FREE")
```

### 4. Double Free

**Cause**: Attempting to free the same memory block twice.

**Example:**
```
ptr = heap.malloc(20)
heap.free(ptr)
heap.free(ptr)  # ERROR: already freed
Result: RuntimeError("DOUBLE FREE DETECTED")
```

### 5. Heap Overflow

**Cause**: Repeatedly allocating memory until the heap is completely exhausted.

**Example:**
```
heap = Heap(100)
for i in range(20):
    heap.malloc(10)  # After 10 allocations, heap is full
```

---

## Performance Comparison

| Metric | First Fit | Best Fit | Polling |
|--------|-----------|----------|---------|
| **Allocation Speed** | Fast ⚡ | Slow 🐢 | Medium ⚡ |
| **Search Strategy** | First match | All blocks | From last position |
| **Fragmentation** | Front of heap | Small fragments everywhere | Uniform distribution |
| **Memory Efficiency** | Medium | Best (tight fit) | Medium |
| **Predictability** | High | High | Medium |
| **Best Use Case** | General purpose | Memory-constrained systems | Uniform workloads |

### Detailed Analysis

#### **First Fit vs Best Fit**

| Aspect | First Fit | Best Fit |
|--------|-----------|----------|
| Speed | Faster (stops at first match) | Slower (searches entire heap) |
| Memory Waste | Can waste more space | Minimizes waste per allocation |
| Fragmentation Pattern | Clusters at beginning | Small fragments throughout |
| Implementation | Simpler | Requires full scan |

#### **First Fit vs Polling**

| Aspect | First Fit | Polling |
|--------|-----------|---------|
| Search Start | Always from beginning | From last allocation |
| Fragmentation Distribution | Front-heavy | More uniform |
| Locality | Better cache locality | Worse cache locality |
| Wraparound | No | Yes (circular) |

#### **Best Fit vs Polling**

| Aspect | Best Fit | Polling |
|--------|----------|---------|
| Goal | Minimize waste | Distribute allocations |
| Search Scope | Entire heap | From last position |
| Tiny Fragments | More likely | Less likely |
| Use Case | Constrained memory | Uniform workloads |

---

## Which Algorithm is Better?

**There is no universally "best" algorithm** - the choice depends on your specific use case:

### ✅ Choose **First Fit** if:
- You need **fast allocation** performance
- Your workload is **general-purpose** and unpredictable
- Simplicity and ease of implementation are priorities
- Cache locality is important

### ✅ Choose **Best Fit** if:
- **Memory is severely constrained**
- You want to **minimize wasted space**
- Allocation speed is less critical than efficiency
- You can afford the overhead of searching the entire heap

### ✅ Choose **Polling** if:
- You want to **avoid clustering** at the heap start
- Your workload has **uniform allocation patterns**
- You want to distribute wear evenly (e.g., flash memory)
- You can tolerate slightly unpredictable behavior

---

## Conclusion

**Winner: It Depends!**

For **most embedded systems** and **general-purpose applications**, **First Fit** is the pragmatic choice due to its:
- Good balance of speed and efficiency
- Predictable behavior
- Simple implementation

However:
- **Best Fit** shines in **memory-constrained IoT devices** where every byte counts
- **Polling** is useful in **specialized scenarios** requiring uniform distribution

In **real-world systems**, hybrid approaches often work best:
- Use **First Fit** for small allocations (< 64 bytes)
- Use **Best Fit** for large allocations (> 1KB)
- Implement **coalescing** regardless of strategy to combat fragmentation

---

## Files in This Assignment

- `heap.ipynb` - Original Jupyter notebook with basic implementation
- `heap_enhanced.py` - Enhanced heap implementation with all three strategies
- `test_strategies.py` - Demonstrations comparing all three algorithms
- `test_crash_scenarios.py` - Comprehensive crash scenario tests
- `README.md` - This documentation file

---

## How to Run

### Option 1: Using Jupyter Notebook (Original)
```bash
jupyter notebook heap.ipynb
```
Run each cell sequentially to see the basic First Fit implementation.

### Option 2: Using Enhanced Python Modules
```bash
# Test all three strategies
python test_strategies.py

# Test crash scenarios
python test_crash_scenarios.py
```

### Option 3: Interactive Python
```python
from heap_enhanced import Heap

# Try First Fit
heap = Heap(100, strategy='first_fit')
ptr = heap.malloc(20)
heap.dump()
heap.free(ptr)

# Try Best Fit
heap2 = Heap(100, strategy='best_fit')
# ... allocate and test

# Try Polling
heap3 = Heap(100, strategy='polling')
# ... allocate and test
```

---

## Key Takeaways

1. **No single "best" allocation algorithm exists** - choose based on your constraints
2. **Fragmentation is inevitable** - use coalescing to mitigate it
3. **First Fit is the pragmatic default** for general-purpose systems
4. **Best Fit minimizes waste** but at the cost of speed
5. **Polling distributes allocations** but may skip better earlier matches
6. **Always implement error detection** (double-free, invalid free, etc.)

---

**Author**: Assignment 2  
**Course**: Software for Embedded Systems  
**Topic**: Dynamic Memory Allocation Algorithms
