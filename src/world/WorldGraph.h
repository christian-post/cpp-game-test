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

class Edge 
{
    // Represents a connection between two Nodes
    // "requirements" are a set of items that the player needs to have in order to traverse this edge
    // "target" represents the Node that this edge goes to
public:
    std::shared_ptr<Node> target;
    std::unordered_set<std::string> requirements;

    Edge(std::shared_ptr<Node> target, const std::unordered_set<std::string>& requirements);
};

class Node 
{
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

class WorldGraph 
{
public:
    std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
    std::shared_ptr<Node> start;
    std::unordered_set<std::string> ownedItems;
    std::vector<std::string> itemPool;

    WorldGraph();

    std::shared_ptr<Node> addNode(const std::string& name, const size_t id, const bool canHaveItem);
    void addEdge(const std::string& from, const std::string& to, const std::unordered_set<std::string>& requirements);

    void setStart(const std::string& name);
    void initializeItems(const std::vector<std::string>& items);

    void forwardFill();

    void logDebug() const; // TODO just for testing
    bool testReachability();

private:
    bool isFilled = false; // store whether the forwardFill function was called
    int totalNumItems = 0; // just for debugging
    std::unordered_set<std::shared_ptr<Node>> getReachableNodes();
    bool hasNullNode() const; // are there any nodes that could have an item, but haven't gotten one yet?
    void search();
    std::default_random_engine rng{ std::random_device{}() }; // initialize the random engine once
};