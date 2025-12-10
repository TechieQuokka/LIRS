#ifndef LIRS_CACHE_HPP
#define LIRS_CACHE_HPP

/*
 * Low Inter-reference Recency Set (LIRS) Cache
 *
 *    ┌─────────────────────────────────────────────────────────────┐
 *    │                    Stack S (LIRS Stack)                     │
 *    │  ┌─────────────────────────────────────────────────────┐    │
 *    │  │ LIR blocks + some HIR blocks (resident/non-resident)│    │
 *    │  │ Bottom is always LIR block                          │    │
 *    │  └─────────────────────────────────────────────────────┘    │
 *    └─────────────────────────────────────────────────────────────┘
 *
 *    ┌─────────────────────────────────────────────────────────────┐
 *    │                    Stack Q (HIR resident)                   │
 *    │  ┌─────────────────────────────────────────────────────┐    │
 *    │  │ All resident HIR blocks (eviction candidates)       │    │
 *    │  └─────────────────────────────────────────────────────┘    │
 *    └─────────────────────────────────────────────────────────────┘
 *
 * Block states:
 *   - LIR (Low IRR): Always resident, protected from eviction
 *   - HIR resident: In cache but can be evicted from Q's bottom
 *   - HIR non-resident: Ghost entry in S (metadata only)
 */

#include <list>
#include <unordered_map>
#include <cstddef>
#include <stdexcept>
#include <optional>
#include <algorithm>

template <typename K, typename V>
class LIRSCache {
protected:
  using Pair = std::pair<K, V>;
  using List = std::list<Pair>;
  using ListIter = typename List::iterator;
  using KeyList = std::list<K>;
  using KeyIter = typename KeyList::iterator;

  struct Entry {
    bool is_LIR;        // LIR status
    bool is_resident;   // Cache residency status
    bool in_lirs_stack; // Presence in LIRS stack (S)
    bool in_hir_stack;  // Presence in HIR resident stack (Q)
    ListIter data_iter; // Position in cache data list
    KeyIter lirs_iter;  // Position in LIRS stack (S)
    KeyIter hir_iter;   // Position in HIR resident stack (Q)
  };

  using Map = std::unordered_map<K, Entry>;

  std::size_t capacity_;
  std::size_t hir_capacity_;
  std::size_t lir_capacity_;
  std::size_t lir_count_;

  List cache_;
  KeyList lirs_stack_;
  KeyList hir_stack_;
  Map map_;

public:
  explicit LIRSCache(std::size_t capacity, double hir_ratio = 0.01)
    : capacity_(capacity)
    , hir_capacity_(std::max<std::size_t>(1, static_cast<std::size_t>(capacity * hir_ratio)))
    , lir_capacity_(capacity - this->hir_capacity_), lir_count_(0) {

    if (capacity == 0) throw std::invalid_argument("Capacity must be greater than 0");
    if (hir_ratio <= 0.0 || hir_ratio >= 1.0) throw std::invalid_argument("HIR ratio must be in range(0, 1)");

    return;
  }

  LIRSCache(const LIRSCache&) = delete;
  LIRSCache& operator=(const LIRSCache&) = delete;

  std::optional<V> get(const K& key) {

    // find key in map
    Map& map = this->map_;
    auto iter = map.find(key);

    // key not found
    if (iter == map.end()) return std::nullopt;

    // get entry
    struct Entry& entry = iter->second;

    // key found but not resident (ghost entry)
    if (!entry.is_resident) return std::nullopt;

    // update access based on block state
    if (entry.is_LIR) this->access_lir(key, entry, this->lirs_stack_, map);
    else this->access_hir_resident(key, entry, this->lirs_stack_, this->hir_stack_, map, this->lir_count_);

    // return value
    return entry.data_iter->second;
  }

  void put(const K& key, const V& value) {

    // find key in map
    Map& map = this->map_;
    auto iter = map.find(key);

    // new key
    if (iter == map.end()) {

      this->insert_new(key, value, this->cache_, this->lirs_stack_, this->hir_stack_, map, this->lir_count_, this->lir_capacity_);
      return;
    }

    // get entry
    struct Entry& entry = iter->second;

    // LIR hit
    if (entry.is_LIR) {

      entry.data_iter->second = value;
      this->access_lir(key, entry, this->lirs_stack_, map);
      return;
    }

    // HIR resident hit
    if (entry.is_resident) {

      entry.data_iter->second = value;
      this->access_hir_resident(key, entry, this->lirs_stack_, this->hir_stack_, map, this->lir_count_);
      return;
    }

    // HIR non-resident (ghost hit)
    this->access_hir_non_resident(key, value, entry, this->cache_, this->lirs_stack_, this->hir_stack_, map, this->lir_count_);
    return;
  }

  std::size_t size() const { return this->cache_.size(); }
  std::size_t capacity() const { return this->capacity_; }
  bool empty() const { return this->cache_.empty(); }

private:
  void insert_new(const K& key, const V& value,
                    List& cache, KeyList& lirs_stack, KeyList& hir_stack,
                    Map& map, std::size_t& lir_count, std::size_t lir_capacity) {

    // Initialization phase: fill the LIR set
    if (lir_count < lir_capacity) {

      cache.push_front({key, value});
      lirs_stack.push_front(key);

      map[key] = Entry {
          true,           // is_LIR
          true,           // is_resident
          true,           // in_lirs_stack
          false,          // in_hir_stack
          cache.begin(),  // data_iter
          lirs_stack.begin(),  // lirs_iter
          {}              // hir_iter (default)
      };
      lir_count++;
      return;
    }

    // normal phase: insert as HIR
    this->evict_hir_resident(cache, hir_stack, map);

    cache.push_front({key, value});
    lirs_stack.push_front(key);
    hir_stack.push_front(key);

    map[key] = Entry {
        false,                // is_LIR
        true,                 // is_resident
        true,                 // in_lirs_stack
        true,                 // in_hir_stack
        cache.begin(),        // data_iter
        lirs_stack.begin(),   // lirs_iter
        hir_stack.begin()     // hir_iter
    };
    return;
  }

  void access_lir(const K& key, Entry& entry, KeyList& lirs_stack, Map& map) {

    bool was_bottom = lirs_stack.back() == key;

    // remove from S and add of the top
    lirs_stack.erase(entry.lirs_iter);
    lirs_stack.push_front(key);
    entry.lirs_iter = lirs_stack.begin();

    if (was_bottom) this->stack_pruning(lirs_stack, map);
    return;
  }

  void access_hir_resident(const K& key, Entry& entry,
                             KeyList& lirs_stack, KeyList& hir_stack,
                             Map& map, std::size_t& lir_count) {

    if (entry.in_lirs_stack) {

      // if in S, promote to LIR
      this->promote_to_lir(key, entry, lirs_stack, hir_stack, map, lir_count);
      return;
    }

    // if not in S, move to the top of both S and Q.
    lirs_stack.push_front(key);
    entry.lirs_iter = lirs_stack.begin();
    entry.in_lirs_stack = true;

    hir_stack.erase(entry.hir_iter);
    hir_stack.push_front(key);
    entry.hir_iter = hir_stack.begin();
    return;
  }

  void access_hir_non_resident(const K& key, const V& value, Entry& entry,
                                  List& cache, KeyList& lirs_stack, KeyList& hir_stack,
                                  Map& map, std::size_t& lir_count) {

    // Victim block replacement
    this->evict_hir_resident(cache, hir_stack, map);

    // load data
    cache.push_front({key, value});
    entry.data_iter = cache.begin();
    entry.is_resident = true;

    if (entry.in_lirs_stack) {

      // if in S, promote to LIR
      this->promote_to_lir(key, entry, lirs_stack, hir_stack, map, lir_count);
      return;
    }

    // if not in S, keep as HIR and add to both S and Q
    lirs_stack.push_front(key);
    entry.lirs_iter = lirs_stack.begin();
    entry.in_lirs_stack = true;

    hir_stack.push_front(key);
    entry.hir_iter = hir_stack.begin();
    entry.in_hir_stack = true;
    return;
  }

  void promote_to_lir(const K& key, Entry& entry,
                        KeyList& lirs_stack, KeyList& hir_stack,
                        Map& map, std::size_t& lir_count) {

    // HIR -> LIR
    entry.is_LIR = true;
    lir_count++;

    // Remove from S and add to the top of Q
    lirs_stack.erase(entry.lirs_iter);
    lirs_stack.push_front(key);
    entry.lirs_iter = lirs_stack.begin();

    // remove from Q
    if (entry.in_hir_stack) {

      hir_stack.erase(entry.hir_iter);
      entry.in_hir_stack = false;
    }

    // Demotion to bottom LIR + pruning
    this->demote_bottom_lir(lirs_stack, hir_stack, map, lir_count);
    this->stack_pruning(lirs_stack, map);
    return;
  }

  void demote_bottom_lir(KeyList& lirs_stack, KeyList& hir_stack, Map& map, std::size_t& lir_count) {

    if (lirs_stack.empty()) return;

    K bottom_key = lirs_stack.back();
    struct Entry& entry = map[bottom_key];

    if (!entry.is_LIR) return;

    // LIR -> HIR
    entry.is_LIR = false;
    lir_count--;

    // remove from S
    lirs_stack.pop_back();
    entry.in_lirs_stack = false;

    // Add the top of Q
    hir_stack.push_front(bottom_key);
    entry.hir_iter = hir_stack.begin();
    entry.in_hir_stack = true;
    return;
  }

  void stack_pruning(KeyList& lirs_stack, Map& map) {

    while (lirs_stack.empty() == false) {

      K bottom_key = lirs_stack.back();
      struct Entry& entry = map[bottom_key];

      if (entry.is_LIR) break;

      lirs_stack.pop_back();
      entry.in_lirs_stack = false;

      if (!entry.is_resident) map.erase(bottom_key);
    }
    return;
  }

  void evict_hir_resident(List& cache, KeyList& hir_stack, Map& map) {

    if (hir_stack.empty()) return;

    K victim_key = hir_stack.back();
    hir_stack.pop_back();

    struct Entry& entry = map[victim_key];

    cache.erase(entry.data_iter);
    entry.is_resident = false;
    entry.in_hir_stack = false;

    if (!entry.in_lirs_stack) map.erase(victim_key);
    return;
  }
};

#endif