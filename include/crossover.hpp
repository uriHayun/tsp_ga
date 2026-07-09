#pragma once

#include "tour.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <unordered_set>
#include <utility>
#include <vector>

class EAX {
    public:
        Tour crossover(const Tour &parentA, const Tour &parentB);

    private:
        // Represents an edge between 2 cities (e.g., (4, 6))
        struct Edge {
            int from;
            int to;
        };

        // Label for whether edge is from parent A or B
        enum class Parent {
            A,
            B
        };

        // Associates an ordinary edge with its originating parent
        struct TaggedEdge {
            Edge edge;
            Parent parent;
        };

        using EdgeKey = std::pair<int, int>;

        struct EdgeKeyHash {
            std::size_t operator()(const EdgeKey &key) const noexcept;
        };

        using EdgeSet = std::unordered_set<EdgeKey, EdgeKeyHash>;
        using TaggedEdges = std::vector<TaggedEdge>;
        using ABGraph = std::vector<std::vector<TaggedEdge>>;


        // Extracts all edges from a tour
        static std::vector<Edge> getEdges(const Tour &tour);

        // Returns the canonical representation of an edge
        static EdgeKey getEdgeKey(const Edge &edge);

        // Checks whether an edge exists in an edge-set
        static bool containsEdge(const EdgeSet &edgeSet, const Edge &edge);

        // Builds a hash set for fast edge lookups
        static EdgeSet buildEdgeSet(const std::vector<Edge> &edges);

        // Returns the edges unique to the source collection.
        static std::vector<Edge> getUniqueEdges(
            const std::vector<Edge> &src,
            const std::vector<Edge> &other);

        // Returns the edges unique to the source collection
        static TaggedEdges tagEdgesWithParent(
            const std::vector<Edge> &edgesA,
            const std::vector<Edge> &edgesB);
        
        // Tags each edge with its originating parent
        static ABGraph buildAdjacencyGraph(
            const std::vector<TaggedEdge> &edges,
            int numCities);     
};