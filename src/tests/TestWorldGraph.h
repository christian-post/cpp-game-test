#pragma once
#include "WorldGraph.h"
#include <iostream>
#include <vector>

int runWorldGraphTest() {
    std::cout << "### Running WorldGraphTest ###" << std::endl;
    WorldGraph graph;

    // Create nodes
    graph.add_node("Start", 0, false);
    graph.add_node("Room1", 1, true);
    graph.add_node("Room2", 2, true);
    graph.add_node("Room3", 3, true);
    graph.add_node("Room4", 4, true);
    graph.add_node("BossRoom", 5, false);

    // Add edges (Room2 is locked behind "Key")
    graph.add_edge("Start", "Room1", {});
    graph.add_edge("Room1", "Room2", {});
    graph.add_edge("Room1", "Room4", { "Key" });
    graph.add_edge("Room2", "Room3", { "Key" });
    graph.add_edge("Room2", "BossRoom", { "Sword" });

    // Set starting point
    graph.set_start("Start");

    // Define item pool (must match required items)
    std::vector<std::string> items = { "Key", "Key", "Sword" };
    graph.initialize_items(items);

    // Forward fill items
    graph.forward_fill();

    // === Output results ===
    std::cout << "Items placed:\n";
    for (const auto& [name, node] : graph.nodes) {
        std::cout << "- " << name;
        if (node->item.has_value())
            std::cout << ": " << *node->item;
        else
            std::cout << ": None";
        std::cout << "\n";
    }

    std::cout << "\nOwned items after search:\n";
    for (const auto& item : graph.owned_items) {
        std::cout << "- " << item << "\n";
    }

    // Simple check: all items should be owned if graph is completable
    if (graph.item_pool.empty()) {
        std::cout << "\nTest passed: all items placed and reachable.\n";
        return 0;
    }
    else {
        std::cout << "\nTest failed: unplaced items remain.\n";
        std::cout << "Remaining items in pool:\n";
        for (const auto& item : graph.item_pool) {
            std::cout << "- " << item << "\n";
        }
        return 1;
    }
}
