# Futex Implementation - Assignment 6

## Objective
Implement the **Readers-Writers problem** using Linux futexes (fast userspace mutexes).

**Configuration:** 2 Writers, 7 Readers, 2 iterations each

## How It Works

### Futex Functions
**`fwait(futex, name)`** - Acquire lock:
- Attempts atomic compare-and-exchange (1 → 0)
- If successful: lock acquired
- If failed: calls `FUTEX_WAIT` to block until another thread releases it
- When woken up: retries acquisition

**`fpost(futex, name)`** - Release lock:
- Atomic compare-and-exchange (0 → 1)
- Calls `FUTEX_WAKE` to wake one waiting thread

### Synchronization Logic

**Writer Thread:**
1. Acquire `writer_futex`
2. Perform write (exclusive access)
3. Release `writer_futex`

**Reader Thread:**
1. Acquire `reader_futex`, increment `readers` count
2. If first reader: acquire `writer_futex` (blocks all writers)
3. Release `reader_futex`
4. Perform read (multiple readers can read concurrently)
5. Acquire `reader_futex`, decrement `readers` count
6. If last reader: release `writer_futex` (allows writers again)
7. Release `reader_futex`

## Dry Run Example

```
Initial: reader_futex=1, writer_futex=1, readers=0

Reader 0:
  fwait(reader_futex) → acquired
  readers = 1 (first reader)
  fwait(writer_futex) → acquired (blocks writers)
  fpost(reader_futex) → released
  [Reading...]

Reader 1 (concurrent):
  fwait(reader_futex) → acquired
  readers = 2
  fpost(reader_futex) → released
  [Reading...] (concurrent with Reader 0)

Writer 0:
  fwait(writer_futex) → BLOCKED (Reader 0 holds it)
  [Waiting on futex...]

Reader 1 finishes:
  fwait(reader_futex) → acquired
  readers = 1
  fpost(reader_futex) → released

Reader 0 finishes:
  fwait(reader_futex) → acquired
  readers = 0 (last reader)
  fpost(writer_futex) → wakes Writer 0!
  fpost(reader_futex) → released

Writer 0 wakes up:
  fwait(writer_futex) → acquired
  [Writing...] (exclusive access)
  fpost(writer_futex) → released
```

## Compilation
```bash
gcc -pthread futex.c -o futex
```

## Run
```bash
./futex
```

## Expected Output
- Threads requesting/acquiring/blocking on futexes
- Concurrent readers executing simultaneously
- Writers blocked when readers are active
- Wake-up events via `FUTEX_WAKE`
