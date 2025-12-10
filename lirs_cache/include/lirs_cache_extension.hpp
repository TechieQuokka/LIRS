#ifndef LIRS_CACHE_EXTENSION_HPP
#define LIRS_CACHE_EXTENSION_HPP

#include "lirs_cache.hpp"
#include <iostream>
#include <string>

template <typename K, typename V>
class LIRSCacheExtension : public LIRSCache<K, V> {
private:
  using Base = LIRSCache<K, V>;
  using typename Base::Entry;
  using typename Base::KeyList;
  using typename Base::Map;

public:
  explicit LIRSCacheExtension(std::size_t capacity, double hir_ratio = 0.01)
    : Base(capacity, hir_ratio) {}

  void Display() const {
    std::cout << "\n";
    std::cout << "================== LIRS Cache State ==================\n";
    std::cout << "\n";

    // Capacity info
    std::cout << "[Capacity]\n";
    std::cout << "  Total: " << this->capacity_ << " | ";
    std::cout << "LIR: " << this->lir_capacity_ << " | ";
    std::cout << "HIR: " << this->hir_capacity_ << "\n";
    std::cout << "  LIR count: " << this->lir_count_ << " | ";
    std::cout << "Cache size: " << this->cache_.size() << "\n";
    std::cout << "\n";

    // Stack S (LIRS stack)
    std::cout << "[Stack S - LIRS Stack] (top -> bottom)\n";
    if (this->lirs_stack_.empty()) {
      std::cout << "  (empty)\n";
    } else {
      for (const K& key : this->lirs_stack_) {
        auto it = this->map_.find(key);
        if (it != this->map_.end()) {
          const Entry& entry = it->second;
          std::cout << "  [" << key << "] ";
          if (entry.is_LIR) {
            std::cout << "LIR";
          } else if (entry.is_resident) {
            std::cout << "HIR-resident";
          } else {
            std::cout << "HIR-non-resident (ghost)";
          }
          std::cout << "\n";
        }
      }
    }
    std::cout << "\n";

    // Stack Q (HIR resident stack)
    std::cout << "[Stack Q - HIR Resident] (top -> bottom)\n";
    if (this->hir_stack_.empty()) {
      std::cout << "  (empty)\n";
    } else {
      for (const K& key : this->hir_stack_) {
        std::cout << "  [" << key << "]\n";
      }
    }
    std::cout << "\n";

    // Cache contents
    std::cout << "[Cache Contents]\n";
    if (this->cache_.empty()) {
      std::cout << "  (empty)\n";
    } else {
      for (const auto& pair : this->cache_) {
        auto it = this->map_.find(pair.first);
        if (it != this->map_.end()) {
          const Entry& entry = it->second;
          std::cout << "  {" << pair.first << ": " << pair.second << "} ";
          std::cout << (entry.is_LIR ? "[LIR]" : "[HIR]") << "\n";
        }
      }
    }

    std::cout << "======================================================\n";
    std::cout << "\n";
  }
};

#endif
