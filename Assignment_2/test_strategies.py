"""
Test demonstrations for heap allocation strategies:
    - Best Fit
    - Pooling Allocation
"""

from heap_changed import Heap


# ------------------------------------------------------------
# BEST FIT DEMO
# ------------------------------------------------------------
print("\n" + "="*60)
print("BEST FIT ALLOCATION STRATEGY")
print("="*60)

heap_bf = Heap(100, strategy="best_fit")
heap_bf.dump()

p1 = heap_bf.malloc(20)
p2 = heap_bf.malloc(30)
p3 = heap_bf.malloc(15)
heap_bf.dump()

print("\n--- Freeing blocks to create holes ---")
heap_bf.free(p1)  # Free 20-byte hole
heap_bf.free(p3)  # Free 15-byte hole
heap_bf.dump()

print("\n--- Best Fit chooses smallest suitable hole ---")
p4 = heap_bf.malloc(18)  # Fits best into 20-byte hole
heap_bf.dump()


# ------------------------------------------------------------
# POOLING DEMO
# ------------------------------------------------------------
print("\n" + "="*60)
print("POOLING ALLOCATION STRATEGY")
print("="*60)

heap_pool = Heap(128, strategy="pooling")
heap_pool.dump()

print("\n--- Allocating from pools (fixed chunk sizes) ---")
a = heap_pool.malloc(20)   # Uses 32-byte pool chunk
b = heap_pool.malloc(50)   # Uses 64-byte pool chunk
c = heap_pool.malloc(60)   # Uses 64-byte pool chunk
heap_pool.dump()

print("\n--- Freeing one pooled block ---")
heap_pool.free(b)
heap_pool.dump()

print("\n--- Pooling reuses freed chunk instantly ---")
d = heap_pool.malloc(40)   # Should reuse the 64-byte chunk
heap_pool.dump()


# ------------------------------------------------------------
# COMPARISON PATTERN TEST
# ------------------------------------------------------------
print("\n" + "="*60)
print("COMPARISON: SAME PATTERN, DIFFERENT STRATEGIES")
print("="*60)

def run_allocation_pattern(strategy_name):
    print(f"\n--- Strategy: {strategy_name.upper()} ---")

    heap = Heap(200, strategy=strategy_name)

    # Step 1: Allocate blocks
    a = heap.malloc(30)
    b = heap.malloc(40)
    c = heap.malloc(30)
    d = heap.malloc(40)

    # Step 2: Free alternating blocks
    heap.free(b)
    heap.free(d)

    print("\nHeap after freeing alternate blocks:")
    heap.dump()

    # Step 3: Try allocation
    print("\nRequesting 35 bytes:")
    try:
        e = heap.malloc(35)
        heap.dump()
        print(f"[SUCCESS] Allocated at address {e}")

    except RuntimeError as ex:
        print(f"[FAILED] {ex}")
        heap.dump()


# Run comparison
run_allocation_pattern("best_fit")
run_allocation_pattern("pooling")
