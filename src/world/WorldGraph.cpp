#include "WorldGraph.h"
#include "Utils.h"
#include <stdexcept>
#include <algorithm>
#include <random>
#include <deque>
#include <unordered_set>



Edge::Edge(std::shared_ptr<Node> target, const std::unordered_set<std::string>& requirements) : target{ target }, requirements{ requirements }{}
Node::Node(const std::string& name) : name{ name }{}

WorldGraph::WorldGraph(){}

std::shared_ptr<Node> WorldGraph::add_node(const std::string& name, const size_t id, const bool canHaveItem)
{
	auto node = std::make_shared<Node>(name);
	nodes[name] = node;
    node->id = id;
    node->canHaveItem = canHaveItem;
	return node;
}

void WorldGraph::add_edge(
    const std::string& from,
    const std::string& to,
    const std::unordered_set<std::string>& requirements
){
    auto from_it = nodes.find(from);
    auto to_it = nodes.find(to);

    if (from_it == nodes.end() || to_it == nodes.end()) {
        TraceLog(LOG_ERROR,
            "Failed add_edge: `from` exists=%d, `to` exists=%d - from='%s', to='%s'",
            (from_it != nodes.end()), (to_it != nodes.end()),
            from.c_str(), to.c_str());
        throw std::runtime_error("Cannot add edge: node not found: " + from + " or " + to);
    }

    from_it->second->edges.emplace_back(to_it->second, requirements);
}

void WorldGraph::set_start(const std::string& name)
{
    start = nodes[name];
}

void WorldGraph::initialize_items(const std::vector<std::string>& items)
{
    for (auto& [_, node] : nodes) {
        node->item.reset();
    }
    item_pool = items;
    std::shuffle(std::begin(item_pool), std::end(item_pool), rng);
    owned_items.clear();
}

void WorldGraph::forward_fill()
{
    while (hasNullNode() && !item_pool.empty()) {
        // get all nodes that don't have an item, 
        // and aren't the start or boss room
        std::vector<std::shared_ptr<Node>> nullNodes;
        for (const auto& node : getReachableNodes()) {
            if (node->canHaveItem && !node->item.has_value()) {
                nullNodes.push_back(node);
            }
        }
        if (nullNodes.empty())
            break;
        // pick a random node
        std::uniform_int_distribution<std::size_t> dist(0, nullNodes.size() - 1);
        size_t randomIdx = dist(rng);
        const std::shared_ptr<Node>& chosenNode = nullNodes[randomIdx];
        std::string item = item_pool.back();;
        item_pool.pop_back();
        chosenNode->item = item;
        owned_items.insert(item);
        // update the list of reachable items
        search();
    }
}

void WorldGraph::log_debug() const
{
    for (const auto& [name, node] : nodes) {
        if (node->item.has_value())
            TraceLog(LOG_INFO, "[%s] item: %s", name.c_str(), node->item->c_str());
        else
            TraceLog(LOG_INFO, "[%s] item: None", name.c_str());

        for (const auto& edge : node->edges) {
            std::string reqs;
            for (const auto& req : edge.requirements) {
                reqs += req + ", ";
            }
            if (!reqs.empty())
                reqs.pop_back(), reqs.pop_back(); // remove trailing comma and space for printing

            TraceLog(LOG_INFO, "  -> %s [%s]", edge.target->name.c_str(), reqs.c_str());
        }
    }
}

void WorldGraph::search()
{
    // breadth first search
    while (true) {
        if (!start) {
            return;
        }
        std::unordered_set<std::shared_ptr<Node>> visited;
        std::deque<std::shared_ptr<Node>> queue;
        queue.push_back(start);
        visited.insert(start);

        std::unordered_set<std::string> newlyAquired;
        while (!queue.empty()) {
            const std::shared_ptr<Node>& node = queue.front();
            if (node->item.has_value() && owned_items.find(*node->item) == owned_items.end())
                // if this node has an item and it's not in owned_items, add it to the newly aquired items
                newlyAquired.insert(*node->item);
            for (auto& edge : node->edges) {
                if (isSubset(edge.requirements, owned_items) && visited.find(edge.target) == visited.end()) {
                    queue.push_back(edge.target);
                    visited.insert(edge.target);
                }
            }
            queue.pop_front();
        }

        if (newlyAquired.empty()) // no new items were found
            break;

        // merge the aquired items into the owned items set
        owned_items.insert(newlyAquired.begin(), newlyAquired.end());
    }
}

std::unordered_set<std::shared_ptr<Node>> WorldGraph::getReachableNodes()
{
    std::unordered_set<std::shared_ptr<Node>> visited;
    if (!start) {
        return visited;
    }

    std::deque<std::shared_ptr<Node>> queue;
    queue.push_back(start);
    visited.insert(start);

    while (!queue.empty()) {
        const std::shared_ptr<Node>& node = queue.front();

        for (const auto& edge : node->edges) {
            bool all_requirements_met = true;
            for (const auto& req : edge.requirements) {
                if (owned_items.count(req) == 0) {
                    all_requirements_met = false;
                    break;
                }
            }
            if (all_requirements_met && visited.count(edge.target) == 0) {
                queue.push_back(edge.target);
                visited.insert(edge.target);
            }
        }
        queue.pop_front();
    }

    return visited;
}

bool WorldGraph::hasNullNode() const
{
    for (const auto& pair : nodes) {
        const auto& node = pair.second;
        if (node->canHaveItem && !node->item.has_value()) {
            return true;
        }
    }
    return false;
}
