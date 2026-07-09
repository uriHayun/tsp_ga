#include "crossover.hpp"
#include "tour.hpp"

#include <algorithm>
#include <functional>

// Performs Edge Assembly Crossover (EAX) between 2 parent tours
// and returns the resulting offspring
Tour EAX::crossover(const Tour &parentA, const Tour &parentB) {

}

// Hash function for using an edge key in an unordered_set
std::size_t EAX::EdgeKeyHash::operator()(const EAX::EdgeKey &key) const noexcept {
    std::size_t h1 = std::hash<int>{}(key.first);
    std::size_t h2 = std::hash<int>{}(key.second);

    return h1 ^ (h2 << 1);
}

// Extracts all edges from a tour, including the closing edge 
// from the last city to the first using modulo the tour length
// e.g., {1, 4, 2, 3} to {(1,4), (4,2), (2,3), (3,1)}
std::vector<EAX::Edge> EAX::getEdges(const Tour &tour) {

    std::vector<EAX::Edge> edges;
    edges.reserve(tour.size());

    for (int fromIdx = 0, N = static_cast<int>(tour.size()); fromIdx < N; fromIdx++) {
        int toIdx = (fromIdx + 1) % N;

        edges.push_back({tour[fromIdx], tour[toIdx]});
    }

    return edges;
}

// Return a canoniacal representation of an edge
// so that (x, y) nd (y, x) are treated as identical
EAX::EdgeKey EAX::getEdgeKey(const EAX::Edge &edge) {
    return {
        std::min(edge.from, edge.to),
        std::max(edge.from, edge.to)
    };
}

// Returns whether an edge exists in an edge-set using an
// average O(1) hash-table lookup
bool EAX::containsEdge(const EAX::EdgeSet &edgeSet, const EAX::Edge &edge) {
    return edgeSet.contains(EAX::getEdgeKey(edge));
}

// Builds a hash set of edges for fast memory lookups
EAX::EdgeSet EAX::buildEdgeSet(const std::vector<EAX::Edge> &edges) {
    EAX::EdgeSet edgeSet;

    for (const EAX::Edge &edge : edges) {
        edgeSet.insert(EAX::getEdgeKey(edge));
    }

    return edgeSet;
}

// Return edges appear in "src" that do not appear in "other"
std::vector<EAX::Edge> EAX::getUniqueEdges(
    const std::vector<EAX::Edge> &src,
    const std::vector<EAX::Edge> &other) {

        EAX::EdgeSet otherSet = EAX::buildEdgeSet(other);

        std::vector<EAX::Edge> uniqueEdges;
        uniqueEdges.reserve(src.size());

        for (const EAX::Edge &edge : src) {
            if (!containsEdge(otherSet, edge)) {
                uniqueEdges.push_back(edge);
            }
        }

        return uniqueEdges;
    }

// Lables each edge with the parent (A or B) it originated from
EAX::TaggedEdges EAX::tagEdgesWithParent(
    const std::vector<EAX::Edge> &edgesA,
    const std::vector<EAX::Edge> &edgesB) {

        EAX::TaggedEdges edges;
        edges.reserve(edgesA.size() + edgesB.size());

        for (const EAX::Edge &edge : edgesA) {
            edges.push_back({edge, EAX::Parent::A});
        }

        for (const EAX::Edge &edge : edgesB) {
            edges.push_back({edge, EAX::Parent::B});
        }

        return edges;
    }

// Builds the AB adjacency graph used to search for AB-cycles
/*
Example AB adjacency graph:

graph[0] =
{
    {{0,1}, Parent::A},
    {{1,4}, Parent::B}
};
*/
EAX::ABGraph EAX::buildAdjacencyGraph(
    const std::vector<EAX::TaggedEdge> &edges,
    int numCities) {
        EAX::ABGraph graph(numCities);

        for (const EAX::TaggedEdge &edge : edges) {
            // Store each edge for both edgepoints
            // so the graph can be traversed from either city
            graph[edge.edge.from].push_back(edge);
            graph[edge.edge.to].push_back(edge);
        }

        return graph;
    }