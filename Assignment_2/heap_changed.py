print("=== Heap Diagnostic Code Loaded ===")


class Heap:
    def __init__(self, size, strategy="best_fit"):

        # Main heap block list (start, size, free)
        self.blocks = [(0, size, True)]

        self.strategy = strategy


        self.pool_sizes = [32, 64, 128, 256, 512]

        # Pools store reusable freed addresses
        self.pools = {psize: [] for psize in self.pool_sizes}

    def dump(self):
        print(f"Heap [{self.strategy}]:")
        print("Blocks :", self.blocks)
        print("Pools  :", self.pools)

    def _coalesce(self):
        i = 0
        while i < len(self.blocks) - 1:
            start1, size1, free1 = self.blocks[i]
            start2, size2, free2 = self.blocks[i + 1]

            if free1 and free2 and (start1 + size1 == start2):
                self.blocks[i] = (start1, size1 + size2, True)
                self.blocks.pop(i + 1)
            else:
                i += 1

    def malloc(self, size):
        print(f"\nCALL: malloc({size})")

        # Dispatch allocation strategy
        if self.strategy == "best_fit":
            result = self._malloc_best_fit(size)

        elif self.strategy == "pooling":
            result = self._malloc_pooling(size)

        else:
            raise ValueError(f"Unknown strategy: {self.strategy}")

        if result is not None:
            print("ALLOCATED at address", result)
            return result

        # ---- FAILURE DIAGNOSTICS ----
        free_blocks = [bsize for _, bsize, free in self.blocks if free]
        total_free = sum(free_blocks)
        largest = max(free_blocks) if free_blocks else 0

        print(">>> ALLOCATION FAILED <<<")
        print("TOTAL FREE :", total_free)
        print("LARGEST BLK:", largest)
        print("REQUESTED :", size)

        if total_free >= size:
            raise RuntimeError("FRAGMENTATION ERROR DETECTED")
        else:
            raise RuntimeError("OUT OF MEMORY")


    def _malloc_best_fit(self, size):
        best_idx = -1
        best_size = float("inf")

        for i, (start, bsize, free) in enumerate(self.blocks):
            if free and bsize >= size and bsize < best_size:
                best_idx = i
                best_size = bsize

        if best_idx == -1:
            return None

        start, bsize, _ = self.blocks[best_idx]
        self.blocks[best_idx] = (start, size, False)

        if bsize > size:
            self.blocks.insert(best_idx + 1,
                               (start + size, bsize - size, True))

        return start


    def _malloc_pooling(self, size):

        chosen_pool = None
        for psize in self.pool_sizes:
            if psize >= size:
                chosen_pool = psize
                break

        if chosen_pool is None:
            return None  # Too large request

        if self.pools[chosen_pool]:
            addr = self.pools[chosen_pool].pop()
            print(f"REUSED chunk from pool {chosen_pool}")
            return addr

        for i, (start, bsize, free) in enumerate(self.blocks):

            if free and bsize >= chosen_pool:

                # Allocate full pool-sized chunk
                self.blocks[i] = (start, chosen_pool, False)

                # Split leftover memory
                if bsize > chosen_pool:
                    self.blocks.insert(i + 1,
                                       (start + chosen_pool,
                                        bsize - chosen_pool,
                                        True))

                print(f"NEW chunk allocated from pool {chosen_pool}")
                return start

        return None

    def free(self, ptr):
        print(f"\nCALL: free({ptr})")

        for i, (start, size, free) in enumerate(self.blocks):

            if start == ptr:

                if free:
                    raise RuntimeError("DOUBLE FREE DETECTED")

                # Mark as free
                self.blocks[i] = (start, size, True)

                # If block belongs to a pool, return it
                if size in self.pools:
                    self.pools[size].append(ptr)
                    print(f"RETURNED chunk to pool {size}")
                    return

                # Otherwise coalesce normally
                self._coalesce()
                print("FREED normally")
                return

        raise RuntimeError("INVALID FREE")
