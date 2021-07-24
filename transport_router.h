#pragma once

#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "domain.h"
#include "router.h"
#include "transport_catalogue.h"

namespace transport_graph {

    using namespace transport_catalogue;

    using TransportTime = double;

    struct TransportGraphData {
        const stop_catalogue::Stop* from;
        const stop_catalogue::Stop* to;
        const bus_catalogue::Bus* bus;
        int stop_count;
        double time;
    };

    struct VertexIdLoop {
        graph::VertexId id{};
        graph::VertexId transfer_id{};
    };

    using EdgesData = std::unordered_map<graph::VertexId, std::unordered_map<graph::VertexId, TransportGraphData>>;

    class TransportGraphDeserialization;

    class TransportGraph {
    public:
        explicit TransportGraph(const TransportCatalogue& catalogue)
            : graph_(2 * catalogue.GetStops().Size()) {
            InitVertexId(catalogue);
            CreateDiagonalEdges(catalogue);
            CreateGraph(catalogue);
        }

        const graph::DirectedWeightedGraph<TransportTime>& GetGraph() const {
            return graph_;
        }

        const std::unordered_map<graph::EdgeId, TransportGraphData>& GetEdgeIdToGraphData() const {
            return edge_id_to_graph_data_;
        }

        const std::unordered_map<const stop_catalogue::Stop*, VertexIdLoop>& GetStopToVertexId() const {
            return stop_to_vertex_id_;
        }

    public:
        friend class TransportGraphDeserialization;

    private:
        TransportGraph() = default;

    private:
        static constexpr double TO_MINUTES = (3.6 / 60.0);

    private:
        void InitVertexId(const TransportCatalogue& catalogue);

        void CreateDiagonalEdges(const TransportCatalogue& catalogue);

        void CreateGraph(const TransportCatalogue& catalogue);

        template <typename It>
        std::vector<TransportGraphData> CreateTransportGraphData(const ranges::BusRange<It>& bus_range, const TransportCatalogue& catalogue);

        void CreateEdges(EdgesData& edges, std::vector<TransportGraphData>&& data);

        void AddEdgesToGraph(EdgesData& edges);

    private:
        std::unordered_map<graph::EdgeId, TransportGraphData> edge_id_to_graph_data_{};

        std::unordered_map<const stop_catalogue::Stop*, VertexIdLoop> stop_to_vertex_id_{};

        graph::DirectedWeightedGraph<TransportTime> graph_{};
    };

    template <typename It>
    inline std::vector<TransportGraphData> TransportGraph::CreateTransportGraphData(const ranges::BusRange<It>& bus_range, const TransportCatalogue& catalogue) {
        const auto& stop_distances = catalogue.GetStops().GetDistances();
        const double bus_velocity = catalogue.GetBuses().GetRouteSettings().bus_velocity;

        std::vector<TransportGraphData> data;

        for (auto it_from = bus_range.begin(); it_from != bus_range.end(); ++it_from) {
            const stop_catalogue::Stop* stop_from = *it_from;
            const stop_catalogue::Stop* previous_stop = stop_from;

            double full_distance = 0.0;
            int stop_count = 0;

            for (auto it_to = it_from + 1; it_to != bus_range.end(); ++it_to) {
                const stop_catalogue::Stop* stop_to = *it_to;

                if (stop_from != stop_to) {
                    full_distance += stop_distances.at({ previous_stop, stop_to });
                    stop_count++;

                    data.push_back({ stop_from, stop_to, bus_range.GetPtr(), stop_count, (full_distance / bus_velocity) * TO_MINUTES });
                }

                previous_stop = stop_to;
            }
        }

        return data;
    }

    class TransportGraphDeserialization {
    public:
        TransportGraphDeserialization() = default;

        TransportGraphDeserialization& SetEdgeIdToGraphData(std::unordered_map<graph::EdgeId, TransportGraphData>&& edge_id_to_graph_data) {
            transport_graph_.edge_id_to_graph_data_ = std::move(edge_id_to_graph_data);
            return *this;
        }

        TransportGraphDeserialization& SetStopToVertexId(std::unordered_map<const stop_catalogue::Stop*, VertexIdLoop>&& stop_to_vertex_id) {
            transport_graph_.stop_to_vertex_id_ = std::move(stop_to_vertex_id);
            return *this;
        }

        TransportGraphDeserialization& CreateGraph(
            std::vector<graph::Edge<TransportTime>>&& edges,
            std::vector<std::vector<graph::EdgeId>>&& incidence_list) {

            graph::GraphDeserialization<TransportTime> deserializer;
            deserializer.SetEdges(std::move(edges));
            deserializer.SetIncidenceList(std::move(incidence_list));

            transport_graph_.graph_ = std::move(deserializer.Build());

            return *this;
        }

        TransportGraph&& Build() {
            return std::move(transport_graph_);
        }

    private:
        TransportGraph transport_graph_;
    };

    class TransportRouterGetter;
    class TransportRouterCreator;

    class TransportRouter {
    public:
        struct TransportRouterData {
            std::vector<TransportGraphData> route{};
            TransportTime time{};
        };

    public:
        explicit TransportRouter(const TransportGraph& transport_graph)
            : transport_graph_(transport_graph)
            , router_(transport_graph.GetGraph()) {
        }

        std::optional<TransportRouter::TransportRouterData> GetRoute(const stop_catalogue::Stop* from, const stop_catalogue::Stop* to) const;

    public:
        friend class TransportRouterGetter;
        friend class TransportRouterCreator;

    private:
        TransportRouter(
            const TransportGraph& transport_graph,
            graph::Router<TransportTime>&& router)
            : transport_graph_(transport_graph)
            , router_(std::move(router)) {
        }

    private:
        const TransportGraph& transport_graph_;
        graph::Router<TransportTime> router_;
    };

    class TransportRouterGetter {
    public:
        static const auto& GetRouter(const TransportRouter& router) {
            return router.router_;
        }
    };

    class TransportRouterCreator {
    public:
        static TransportRouter Build(
            const TransportGraph& transport_graph,
            graph::Router<TransportTime>&& router) {
            return { transport_graph, std::move(router) };
        }
    };

} // namespace transport_graph
