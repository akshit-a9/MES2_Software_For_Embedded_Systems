"""
Heap Crash, Fragmentation, and Pooling Error Scenarios
"""

from heap_changed import Heap

# ------------------------------------------------------------
# SCENARIO 1: FRAGMENTATION ERROR (BEST FIT)
# ------------------------------------------------------------
print("="*60)
print("SCENARIO 1: FRAGMENTATION ERROR (BEST FIT)")
print("="*60)

heap = Heap(100, strategy="best_fit")

a = heap.malloc(20)
b = heap.malloc(20)
c = heap.malloc(20)
d = heap.malloc(20)
heap.dump()

heap.free(b)
heap.free(d)
heap.dump()

print("\nRequesting 25 bytes (total free=40, largest=20)")
try:
    heap.malloc(25)
except RuntimeError as ex:
    print(f"[SUCCESS] Expected error caught: {ex}")

# ------------------------------------------------------------
# SCENARIO 2: OUT OF MEMORY (BEST FIT)
# ------------------------------------------------------------
print("\n" + "="*60)
print("SCENARIO 2: OUT OF MEMORY (BEST FIT)")
print("="*60)

heap2 = Heap(50, strategy="best_fit")

heap2.malloc(30)
heap2.malloc(15)
heap2.dump()

print("\nRequesting 10 bytes (only 5 available)")
try:
    heap2.malloc(10)
except RuntimeError as ex:
    print(f"[SUCCESS] Expected error caught: {ex}")

# ------------------------------------------------------------
# SCENARIO 3: INVALID FREE
# ------------------------------------------------------------
print("\n" + "="*60)
print("SCENARIO 3: INVALID FREE")
print("="*60)

heap3 = Heap(100, strategy="best_fit")
heap3.malloc(20)

try:
    heap3.free(999)
except RuntimeError as ex:
    print(f"[SUCCESS] Expected error caught: {ex}")

# ------------------------------------------------------------
# SCENARIO 4: DOUBLE FREE
# ------------------------------------------------------------
print("\n" + "="*60)
print("SCENARIO 4: DOUBLE FREE")
print("="*60)

heap4 = Heap(100, strategy="best_fit")
p = heap4.malloc(20)
heap4.free(p)

try:
    heap4.free(p)
except RuntimeError as ex:
    print(f"[SUCCESS] Expected error caught: {ex}")

# ------------------------------------------------------------
# SCENARIO 5: POOLING — NORMAL REUSE
# ------------------------------------------------------------
print("\n" + "="*60)
print("SCENARIO 5: POOLING — REUSE FROM POOL")
print("="*60)

heap5 = Heap(128, strategy="pooling")

a = heap5.malloc(40)   # Uses 64-byte pool
b = heap5.malloc(50)   # Uses 64-byte pool
heap5.dump()

heap5.free(a)
heap5.dump()

print("\nRequesting 32 bytes (should reuse pool)")
c = heap5.malloc(32)
heap5.dump()

# ------------------------------------------------------------
# SCENARIO 6: POOLING — OUT OF MEMORY (NO FRAGMENTATION)
# ------------------------------------------------------------
print("\n" + "="*60)
print("SCENARIO 6: POOLING — OUT OF MEMORY")
print("="*60)

heap6 = Heap(128, strategy="pooling")

allocs = []
try:
    while True:
        ptr = heap6.malloc(60)  # Each consumes 64 bytes
        allocs.append(ptr)
        print("Allocated at", ptr)
except RuntimeError as ex:
    print(f"[SUCCESS] Pooling OOM caught: {ex}")
    heap6.dump()

# ------------------------------------------------------------
# SCENARIO 7: POOLING — LARGE REQUEST FAILS
# ------------------------------------------------------------
print("\n" + "="*60)
print("SCENARIO 7: POOLING — REQUEST TOO LARGE")
print("="*60)

heap7 = Heap(256, strategy="pooling")

print("Requesting 1024 bytes (larger than any pool & heap)")
try:
    heap7.malloc(1024)
except RuntimeError as ex:
    print(f"[SUCCESS] Expected error caught: {ex}")
