# LIRS Cache

A C++17 header-only implementation of the **Low Inter-reference Recency Set (LIRS)** cache replacement algorithm.

## Overview

LIRS is a cache replacement algorithm that significantly outperforms LRU for workloads with weak locality. It was introduced in the paper:

> **"LIRS: An Efficient Low Inter-reference Recency Set Replacement Policy to Improve Buffer Cache Performance"**
> Song Jiang and Xiaodong Zhang
> IEEE Transactions on Computers, Vol. 54, No. 8, August 2005

## Key Concepts

### Inter-Reference Recency (IRR)

Unlike LRU which only considers **recency** (time since last access), LIRS uses **IRR** - the number of distinct blocks accessed between two consecutive references to the same block.

### Block Classification

| Status | Description |
|--------|-------------|
| **LIR** (Low IRR) | Blocks with small IRR, always resident in cache |
| **HIR resident** | Blocks with high IRR, can be evicted |
| **HIR non-resident** | Ghost entries (metadata only) |

### Data Structures

```
+------------------------------------------+
|           Stack S (LIRS Stack)           |
|  - LIR blocks                            |
|  - Some HIR blocks (resident/non-resident)|
|  - Bottom is always LIR block            |
+------------------------------------------+

+------------------------------------------+
|         Stack Q (HIR Resident)           |
|  - All resident HIR blocks               |
|  - Eviction candidates (FIFO from bottom)|
+------------------------------------------+
```

## Why LIRS over LRU?

| Problem | LRU | LIRS |
|---------|-----|------|
| Sequential scan | Evicts hot blocks | Protects LIR blocks |
| Loop > cache size | ~0% hit rate | Proportional hit rate |
| Mixed frequency | Equal treatment | Distinguishes by IRR |

## Installation

Header-only library. Just include the header:

```cpp
#include "lirs_cache/include/lirs_cache.hpp"
// or with Display() support:
#include "lirs_cache/include/lirs_cache_extension.hpp"
```

## Requirements

- C++17 or later
- CMake 3.10+ (for building examples)

## Usage

### Basic Usage

```cpp
#include "lirs_cache/include/lirs_cache.hpp"

int main() {
    // Create cache with capacity=100, HIR ratio=1% (default)
    LIRSCache<int, std::string> cache(100);

    // Insert
    cache.put(1, "value1");
    cache.put(2, "value2");

    // Retrieve
    auto val = cache.get(1);
    if (val) {
        std::cout << *val << std::endl;  // "value1"
    }

    // Check state
    std::cout << "Size: " << cache.size() << std::endl;
    std::cout << "Empty: " << cache.empty() << std::endl;

    return 0;
}
```

### With Debug Display

```cpp
#include "lirs_cache/include/lirs_cache_extension.hpp"

int main() {
    LIRSCacheExtension<int, std::string> cache(5, 0.2);

    cache.put(1, "A");
    cache.put(2, "B");
    cache.put(3, "C");

    cache.Display();  // Print cache state

    return 0;
}
```

### Display Output Example

```
================== LIRS Cache State ==================

[Capacity]
  Total: 5 | LIR: 4 | HIR: 1
  LIR count: 3 | Cache size: 3

[Stack S - LIRS Stack] (top -> bottom)
  [3] LIR
  [2] LIR
  [1] LIR

[Stack Q - HIR Resident] (top -> bottom)
  (empty)

[Cache Contents]
  {3: C} [LIR]
  {2: B} [LIR]
  {1: A} [LIR]

======================================================
```

## API Reference

### LIRSCache<K, V>

| Method | Description |
|--------|-------------|
| `LIRSCache(capacity, hir_ratio=0.01)` | Constructor |
| `std::optional<V> get(const K& key)` | Get value (returns nullopt on miss) |
| `void put(const K& key, const V& value)` | Insert or update |
| `std::size_t size()` | Current cache size |
| `std::size_t capacity()` | Maximum capacity |
| `bool empty()` | Check if empty |

### LIRSCacheExtension<K, V>

Inherits from `LIRSCache<K, V>` and adds:

| Method | Description |
|--------|-------------|
| `void Display()` | Print cache state to stdout |

## Algorithm Details

### Three Access Cases

1. **LIR block access**: Move to top of S, prune if was at bottom
2. **HIR resident access**:
   - In S: Promote to LIR, demote bottom LIR
   - Not in S: Move to top of S and Q
3. **HIR non-resident access** (ghost hit):
   - In S: Promote to LIR, demote bottom LIR
   - Not in S: Insert as HIR

### Stack Pruning

Removes HIR blocks from bottom of S until an LIR block is at bottom. Non-resident HIR blocks are completely removed from tracking.

## Building

```bash
mkdir build && cd build
cmake ..
cmake --build .
./main
```

## Project Structure

```
LIRS/
├── lirs_cache/
│   └── include/
│       ├── lirs_cache.hpp           # Core implementation
│       └── lirs_cache_extension.hpp # Display extension
├── CMakeLists.txt
├── main.cpp                         # Usage examples
└── README.md
```

## Complexity

| Operation | Time | Space |
|-----------|------|-------|
| get() | O(1) | - |
| put() | O(1) | - |
| Total | - | O(n) |

## References

- [Original Paper (IEEE)](https://doi.org/10.1109/TC.2005.130)
- [LIRS on Wikipedia](https://en.wikipedia.org/wiki/LIRS_caching_algorithm)

## License

MIT License

## Author

TechieQuokka

## See Also

- [ARC Cache](https://github.com/TechieQuokka/ARC) - Adaptive Replacement Cache implementation
