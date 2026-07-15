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

        // Hash function for using a tagged-edge-key in an unordered_set
        struct TaggedEdgeHash {
            std::size_t operator()(const TaggedEdge &te) const noexcept;
        };

        // Equality function for using a TaggedEdge in an unordered_set
        struct TaggedEdgeEqual {
            bool operator()(const TaggedEdge &lhs, const TaggedEdge &rhs) const noexcept;
        };

        using EdgeKey = std::pair<int, int>;

        // Hash function for using an edge-key in an unordered_set
        struct EdgeHash {
            std::size_t operator()(const EdgeKey &key) const noexcept;
        };

        using EdgeSet = std::unordered_set<EdgeKey, EdgeHash>;
        using TaggedEdgeSet = std::unordered_set<TaggedEdge, TaggedEdgeHash, TaggedEdgeEqual>;
        using TaggedEdges = std::vector<TaggedEdge>;
        using ABGraph = std::vector<std::vector<TaggedEdge>>;
        using ABCycle = std::vector<TaggedEdge>;

        // Extracts all edges from a tour
        static std::vector<Edge> getEdges(const Tour &tour);

        // Returns the canonical representation of an edge
        static EdgeKey normalizeEdge(const Edge &edge);

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
        static ABGraph buildAdjGraph(
            const std::vector<TaggedEdge> &edges,
            int numCities);

        // Given endpoint of an edge, returns other endpoint
        static int getOtherEndpoint(const TaggedEdge &te, int currCity);

        // Builds the initial collection of unused edges,
        // each edge being tracked once rather than once per each endpoint
        static TaggedEdgeSet buildUnusedEdgesSet(const std::vector<TaggedEdge> &edges);

        // Transforms the AB-graph into AB-cycles by repeatedly walking an alternating
        // path between A/B edges from an arbitrary edge until returning to the starting
        // city
        static std::vector<ABCycle> getABCycles(
            const ABGraph &graph, 
            const TaggedEdges &edges);
};