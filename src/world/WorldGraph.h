#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <random>
#include <optional>

// taken from this paper:
// https://github.com/cjohnson57/RandomizerAlgorithms/blob/master/Paper%2FPaper.pdf

// TODO rename from snake to camel case


class Node;

class Edge {
    // Represents a connection between two Nodes
    // "requirements" are a set of items that the player needs to have in order to traverse this edge
    // "target" represents the Node that this edge goes to
public:
    std::shared_ptr<Node> target;
    std::unordered_set<std::string> requirements;

    Edge(std::shared_ptr<Node> target, const std::unordered_set<std::string>& requirements);
};

class Node {
    // Represents a room in a dungeon or overworld
    // "name" is used as an identifier
    // "item" corresponds to a key in ItemData. I might expand this to event triggers as well.
public:
    std::string name;
    size_t id = 999; // corresponds to Room index
    bool isStart = false;
    bool isBoss = false;
    bool canHaveItem = false;
    std::vector<Edge> edges;
    std::optional<std::string> item; // empty string = no item

    explicit Node(const std::string& name);
};

class WorldGraph {
public:
    std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
    std::shared_ptr<Node> start;
    std::unordered_set<std::string> owned_items;
    std::vector<std::string> item_pool;

    WorldGraph();

    std::shared_ptr<Node> add_node(const std::string& name, const size_t id, const bool canHaveItem);
    void add_edge(const std::string& from, const std::string& to, const std::unordered_set<std::string>& requirements);

    void set_start(const std::string& name);
    void initialize_items(const std::vector<std::string>& items);

    void forward_fill();

    void log_debug() const; // TODO just for testing

private:
    std::unordered_set<std::shared_ptr<Node>> getReachableNodes();
    bool hasNullNode() const;
    void search();
    std::default_random_engine rng{ std::random_device{}() }; // initialize the random engine once
};