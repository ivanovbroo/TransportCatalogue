#pragma once

#include <cstdlib>
#include <vector>
#include <utility>

#include "ranges.h"

namespace graph {

    using VertexId = size_t;
    using EdgeId = size_t;

    template <typename Weight>
    struct Edge {
        VertexId from;
        VertexId to;
        Weight weight;
    };

    template <typename Weight>
    class DirectedWeightedGraph;

    template <typename Weight>
    class GraphSerialization {
    public:
        GraphSerialization() = default;

        const auto& GetEdges(const DirectedWeightedGraph<Weight>& graph) const {
            return graph.edges_;
        }

        const auto& GetIncidenceList(const DirectedWeightedGraph<Weight>& graph) const {
            return graph.incidence_lists_;
        }
    };

    template <typename Weight>
    class GraphDeserialization {
    public:
        GraphDeserialization() = default;

        GraphDeserialization& SetEdges(std::vector<Edge<Weight>>&& edges) {
            graph_.edges_ = std::move(edges);
            return *this;
        }

        GraphDeserialization& SetIncidenceList(std::vector<std::vector<EdgeId>>&& incidence_list) {
            graph_.incidence_lists_ = std::move(incidence_list);
            return *this;
        }

        DirectedWeightedGraph<Weight>&& Build() {
            return std::move(graph_);
        }

    private:
        DirectedWeightedGraph<Weight> graph_{};
    };

    template <typename Weight>
    class DirectedWeightedGraph {
    public:
        using IncidenceList = std::vector<EdgeId>;
        using IncidentEdgesRange = ranges::Range<typename IncidenceList::const_iterator>;

    public:
        DirectedWeightedGraph() = default;
        explicit DirectedWeightedGraph(size_t vertex_count);
        EdgeId AddEdge(const Edge<Weight>& edge);

        size_t GetVertexCount() const;
        size_t GetEdgeCount() const;
        const Edge<Weight>& GetEdge(EdgeId edge_id) const;
        IncidentEdgesRange GetIncidentEdges(VertexId vertex) const;

    public:
        friend class GraphSerialization<Weight>;
        friend class GraphDeserialization<Weight>;

    private:
        std::vector<Edge<Weight>> edges_;
        std::vector<IncidenceList> incidence_lists_;
    };

    template <typename Weight>
    DirectedWeightedGraph<Weight>::DirectedWeightedGraph(size_t vertex_count)
        : incidence_lists_(vertex_count) {
    }

    template <typename Weight>
    EdgeId DirectedWeightedGraph<Weight>::AddEdge(const Edge<Weight>& edge) {
        edges_.push_back(edge);
        const EdgeId id = edges_.size() - 1;
        incidence_lists_.at(edge.from).push_back(id);
        return id;
    }

    template <typename Weight>
    size_t DirectedWeightedGraph<Weight>::GetVertexCount() const {
        return incidence_lists_.size();
    }

    template <typename Weight>
    size_t DirectedWeightedGraph<Weight>::GetEdgeCount() const {
        return edges_.size();
    }

    template <typename Weight>
    const Edge<Weight>& DirectedWeightedGraph<Weight>::GetEdge(EdgeId edge_id) const {
        return edges_.at(edge_id);
    }

    template <typename Weight>
    typename DirectedWeightedGraph<Weight>::IncidentEdgesRange
        DirectedWeightedGraph<Weight>::GetIncidentEdges(VertexId vertex) const {
        return ranges::AsRange(incidence_lists_.at(vertex));
    }
}  // namespace graph
