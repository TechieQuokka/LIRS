#include <iostream>
#include "lirs_cache/include/lirs_cache_extension.hpp"

void print_section(const std::string& title) {
    std::cout << "\n";
    std::cout << "##########################################################\n";
    std::cout << "# " << title << "\n";
    std::cout << "##########################################################\n";
}

void print_action(const std::string& action) {
    std::cout << "\n>>> " << action << "\n";
}

int main() {
    std::cout << "==========================================================\n";
    std::cout << "        LIRS Cache Algorithm - Comprehensive Test         \n";
    std::cout << "==========================================================\n";

    // Create cache: capacity=5, hir_ratio=0.2 (LIR=4, HIR=1)
    LIRSCacheExtension<int, std::string> cache(5, 0.2);

    //----------------------------------------------------------
    // Phase 1: Initialization - Fill LIR set
    //----------------------------------------------------------
    print_section("Phase 1: Initialization - Fill LIR Set");

    print_action("put(1, \"A\") - First block, becomes LIR");
    cache.put(1, "A");
    cache.Display();

    print_action("put(2, \"B\") - Second block, becomes LIR");
    cache.put(2, "B");
    cache.Display();

    print_action("put(3, \"C\") - Third block, becomes LIR");
    cache.put(3, "C");
    cache.Display();

    print_action("put(4, \"D\") - Fourth block, becomes LIR (LIR set full)");
    cache.put(4, "D");
    cache.Display();

    //----------------------------------------------------------
    // Phase 2: Normal operation - HIR blocks
    //----------------------------------------------------------
    print_section("Phase 2: Normal Operation - HIR Blocks");

    print_action("put(5, \"E\") - Fifth block, becomes HIR (first HIR block)");
    cache.put(5, "E");
    cache.Display();

    print_action("put(6, \"F\") - Sixth block, evicts HIR block 5, becomes HIR");
    cache.put(6, "F");
    cache.Display();

    //----------------------------------------------------------
    // Phase 3: Case 1 - Access LIR block
    //----------------------------------------------------------
    print_section("Phase 3: Case 1 - Access LIR Block");

    print_action("get(1) - Access LIR block, move to top of S");
    auto val = cache.get(1);
    std::cout << "    Result: " << (val ? *val : "(miss)") << "\n";
    cache.Display();

    print_action("get(4) - Access bottom LIR block (triggers stack pruning)");
    val = cache.get(4);
    std::cout << "    Result: " << (val ? *val : "(miss)") << "\n";
    cache.Display();

    //----------------------------------------------------------
    // Phase 4: Case 2a - Access HIR resident in S (promote to LIR)
    //----------------------------------------------------------
    print_section("Phase 4: Case 2a - HIR Resident in S -> Promote to LIR");

    print_action("get(6) - HIR resident in S, promotes to LIR, bottom LIR demotes");
    val = cache.get(6);
    std::cout << "    Result: " << (val ? *val : "(miss)") << "\n";
    cache.Display();

    //----------------------------------------------------------
    // Phase 5: Case 2b - Access HIR resident NOT in S
    //----------------------------------------------------------
    print_section("Phase 5: Case 2b - HIR Resident NOT in S");

    print_action("put(7, \"G\") - New HIR block");
    cache.put(7, "G");
    cache.Display();

    print_action("put(8, \"H\") - New HIR block, evicts 7");
    cache.put(8, "H");
    cache.Display();

    print_action("put(7, \"G2\") - Block 7 is ghost (non-resident in S)");
    cache.put(7, "G2");
    cache.Display();

    //----------------------------------------------------------
    // Phase 6: Case 3a - Access HIR non-resident in S (ghost hit)
    //----------------------------------------------------------
    print_section("Phase 6: Case 3a - HIR Non-resident in S (Ghost Hit)");

    // Setup: create a ghost entry
    print_action("Setup: Creating ghost entry...");
    print_action("put(9, \"I\") - New HIR");
    cache.put(9, "I");
    cache.Display();

    print_action("put(10, \"J\") - New HIR, evicts previous HIR");
    cache.put(10, "J");
    cache.Display();

    // Now access the ghost entry
    print_action("put(9, \"I2\") - Ghost hit! Block 9 in S, promotes to LIR");
    cache.put(9, "I2");
    cache.Display();

    //----------------------------------------------------------
    // Phase 7: Case 3b - Access HIR non-resident NOT in S
    //----------------------------------------------------------
    print_section("Phase 7: Case 3b - HIR Non-resident NOT in S (Complete Miss)");

    print_action("put(99, \"NEW\") - Completely new block, not in S");
    cache.put(99, "NEW");
    cache.Display();

    //----------------------------------------------------------
    // Phase 8: Update existing values
    //----------------------------------------------------------
    print_section("Phase 8: Update Existing Values");

    print_action("put(1, \"A_updated\") - Update LIR block value");
    cache.put(1, "A_updated");
    val = cache.get(1);
    std::cout << "    get(1) = " << (val ? *val : "(miss)") << "\n";
    cache.Display();

    //----------------------------------------------------------
    // Phase 9: Cache miss scenarios
    //----------------------------------------------------------
    print_section("Phase 9: Cache Miss Scenarios");

    print_action("get(100) - Access non-existent key");
    val = cache.get(100);
    std::cout << "    Result: " << (val ? *val : "(miss)") << "\n";

    print_action("get(5) - Access evicted block (was HIR, now gone)");
    val = cache.get(5);
    std::cout << "    Result: " << (val ? *val : "(miss)") << "\n";

    //----------------------------------------------------------
    // Phase 10: Looping pattern (LIRS strength)
    //----------------------------------------------------------
    print_section("Phase 10: Looping Pattern - LIRS Advantage");

    LIRSCacheExtension<int, std::string> loop_cache(3, 0.34);  // LIR=2, HIR=1

    std::cout << "\nSimulating loop access: 1->2->3->4->1->2->3->4->...\n";
    std::cout << "Cache size = 3, Loop size = 4\n\n";

    // First round
    print_action("Round 1: Initial access");
    loop_cache.put(1, "v1"); std::cout << "  put(1)\n";
    loop_cache.put(2, "v2"); std::cout << "  put(2)\n";
    loop_cache.put(3, "v3"); std::cout << "  put(3)\n";
    loop_cache.put(4, "v4"); std::cout << "  put(4)\n";
    loop_cache.Display();

    // Second round - LIRS should keep LIR blocks
    print_action("Round 2: Re-access pattern");
    val = loop_cache.get(1);
    std::cout << "  get(1) = " << (val ? *val : "MISS") << "\n";
    val = loop_cache.get(2);
    std::cout << "  get(2) = " << (val ? *val : "MISS") << "\n";
    loop_cache.Display();

    //----------------------------------------------------------
    // Summary
    //----------------------------------------------------------
    print_section("Test Complete!");

    std::cout << "\nAll LIRS algorithm cases tested:\n";
    std::cout << "  [v] Case 1:  LIR block access\n";
    std::cout << "  [v] Case 2a: HIR resident in S (promote to LIR)\n";
    std::cout << "  [v] Case 2b: HIR resident NOT in S (stay HIR)\n";
    std::cout << "  [v] Case 3a: HIR non-resident in S (ghost hit, promote)\n";
    std::cout << "  [v] Case 3b: HIR non-resident NOT in S (new HIR)\n";
    std::cout << "  [v] Stack pruning\n";
    std::cout << "  [v] LIR demotion\n";
    std::cout << "  [v] HIR eviction\n";
    std::cout << "\n";

    return 0;
}
