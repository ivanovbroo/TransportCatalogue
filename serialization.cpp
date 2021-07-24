#include <iostream>
#include <vector>

#include "map_renderer.h"
#include "svg.h"
#include "serialization.h"

namespace transport_serialization {

    namespace detail_serialization {

        transport_proto::Coordinates CreateProtoCoord(const Coordinates& coord) {
            transport_proto::Coordinates proto_coord;
            proto_coord.set_lat(coord.lat);
            proto_coord.set_lng(coord.lng);
            return proto_coord;
        }

        transport_proto::Stop CreateProtoStop(const transport_catalogue::stop_catalogue::Stop* stop, const request_handler::RequestHandler& rh) {
            transport_proto::Stop proto_stop;

            proto_stop.set_id(rh.GetId(stop));
            proto_stop.set_name(stop->name);
            *proto_stop.mutable_coord() = CreateProtoCoord(stop->coord);

            return proto_stop;
        }

        transport_proto::Bus CreateProtoBus(const transport_catalogue::bus_catalogue::Bus* bus, const request_handler::RequestHandler& rh) {
            transport_proto::Bus proto_bus;

            proto_bus.set_id(rh.GetId(bus));
            proto_bus.set_name(bus->name);
            for (const auto* stop : bus->route) {
                proto_bus.add_route(rh.GetId(stop));
            }
            proto_bus.set_type(static_cast<uint32_t>(bus->route_type));
            proto_bus.set_route_geo_length(bus->route_geo_length);
            proto_bus.set_route_true_length(bus->route_true_length);
            proto_bus.set_stops_on_route(bus->stops_on_route);
            proto_bus.set_unique_stops(bus->unique_stops);

            return proto_bus;
        }

        transport_proto::Point CreateProtoPoint(const svg::Point& point) {
            transport_proto::Point proto_point;

            proto_point.set_x(point.x);
            proto_point.set_y(point.y);

            return proto_point;
        }

        transport_proto::Color CreateProtoColor(const svg::Color& color) {
            transport_proto::Color proto_color;

            proto_color.set_type(svg::ColorTypeToInt(svg::GetColorType(color)));
            proto_color.set_name(svg::GetColorStringName(color));

            return proto_color;
        }

        transport_proto::MapRenderSettings CreateProtoMapRenderSettings(const map_renderer::MapRendererSettings& setting) {
            transport_proto::MapRenderSettings proto_settings;

            proto_settings.set_width(setting.width);
            proto_settings.set_height(setting.height);
            proto_settings.set_padding(setting.padding);
            proto_settings.set_line_width(setting.line_width);
            proto_settings.set_stop_radius(setting.stop_radius);
            proto_settings.set_bus_label_font_size(setting.bus_label_font_size);
            *proto_settings.mutable_bus_label_offset() = CreateProtoPoint(setting.bus_label_offset);
            proto_settings.set_stop_label_font_size(setting.stop_label_font_size);
            *proto_settings.mutable_stop_label_offset() = CreateProtoPoint(setting.stop_label_offset);
            *proto_settings.mutable_underlayer_color() = CreateProtoColor(setting.underlayer_color);
            proto_settings.set_underlayer_width(setting.underlayer_width);
            for (const svg::Color& color : setting.color_palette) {
                *proto_settings.add_color_palette() = CreateProtoColor(color);
            }

            return proto_settings;
        }

        transport_proto::RouteSettings CreateProtoRouteSetting(const transport_catalogue::RouteSettings& settings) {
            transport_proto::RouteSettings proto_settings;

            proto_settings.set_bus_velocity(settings.bus_velocity);
            proto_settings.set_bus_waiting_time(settings.bus_wait_time);

            return proto_settings;
        }

        transport_proto::Edge CreateProtoEdge(const graph::Edge<double>& edge) {
            transport_proto::Edge proto_edge;

            proto_edge.set_from(edge.from);
            proto_edge.set_to(edge.to);
            proto_edge.set_weight(edge.weight);

            return proto_edge;
        }

        transport_proto::IncidenceList CreateProtoIncidenceList(const graph::DirectedWeightedGraph<transport_graph::TransportTime>::IncidenceList& incidence_list) {
            transport_proto::IncidenceList proto_incidence_list;

            for (const auto& edge_id : incidence_list) {
                proto_incidence_list.add_id(edge_id);
            }

            return proto_incidence_list;
        }

        transport_proto::TransportGraphData CreateProtoTransportGraphData(const transport_graph::TransportGraphData& data, const request_handler::RequestHandler& rh) {
            transport_proto::TransportGraphData proto_data;

            proto_data.set_stop_to_id(rh.GetId(data.to));
            proto_data.set_stop_from_id(rh.GetId(data.from));
            if (data.bus) {
                proto_data.set_bus_id(rh.GetId(data.bus));
                proto_data.set_is_bus(true);
            }
            else {
                proto_data.set_is_bus(false);
            }
            proto_data.set_stop_count(data.stop_count);
            proto_data.set_time(data.time);

            return proto_data;
        }

        transport_proto::VertexIdLoop CreateProtoVertexIdLoop(const transport_graph::VertexIdLoop& vertex_id_loop) {
            transport_proto::VertexIdLoop proto_vertex_id_loop;

            proto_vertex_id_loop.set_id(vertex_id_loop.id);
            proto_vertex_id_loop.set_transfer_id(vertex_id_loop.transfer_id);

            return proto_vertex_id_loop;
        }

        transport_proto::Graph CreateProtoGraph(const transport_graph::TransportGraph& graph, const request_handler::RequestHandler& rh) {
            transport_proto::Graph proto_graph;

            graph::GraphSerialization<transport_graph::TransportTime> gs;

            for (const auto& edge : gs.GetEdges(graph.GetGraph())) {
                *proto_graph.add_edge() = CreateProtoEdge(edge);
            }

            for (const auto& incidence_list : gs.GetIncidenceList(graph.GetGraph())) {
                *proto_graph.add_incidence_list() = CreateProtoIncidenceList(incidence_list);
            }

            for (const auto& [edge_id, graph_data] : graph.GetEdgeIdToGraphData()) {
                (*proto_graph.mutable_edge_id_to_graph_data())[static_cast<uint32_t>(edge_id)] = CreateProtoTransportGraphData(graph_data, rh);
            }

            for (const auto& [stop_ptr, vertex_id_loop] : graph.GetStopToVertexId()) {
                (*proto_graph.mutable_stop_to_vertex_id())[rh.GetId(stop_ptr)] = CreateProtoVertexIdLoop(vertex_id_loop);
            }

            return proto_graph;
        }

        transport_proto::RouteInternalData CreateProtoRouteInternalData(size_t pos, const graph::Router<transport_graph::TransportTime>::RouteInternalData& data) {
            transport_proto::RouteInternalData proto_route_internal_data;

            proto_route_internal_data.set_pos(pos);
            proto_route_internal_data.set_weight(data.weight);
            if (data.prev_edge) {
                proto_route_internal_data.set_is_prev_edge(true);
                proto_route_internal_data.set_prev_edge_id(*data.prev_edge);
            }
            else {
                proto_route_internal_data.set_is_prev_edge(false);
            }

            return proto_route_internal_data;
        }

        transport_proto::Router CreateProtoRouter(const transport_graph::TransportRouter& transport_router) {
            transport_proto::Router proto_router;

            const auto& router = transport_graph::TransportRouterGetter::GetRouter(transport_router);

            transport_proto::RoutesInternalData routes_internal_data;
            for (const auto& vector_with_internal_data : graph::RouterDataGetter<transport_graph::TransportTime>::GetInternalData(router)) {
                transport_proto::RouteInternalDataVector data_vector;
                data_vector.set_size(vector_with_internal_data.size());
                size_t pos = 0;
                for (const auto& internal_data : vector_with_internal_data) {
                    if (internal_data) {
                        *data_vector.add_route_internal_data() = CreateProtoRouteInternalData(pos, *internal_data);
                    }
                    pos++;
                }
                *routes_internal_data.add_routes_internal_data_vector() = std::move(data_vector);
            }

            *proto_router.mutable_routes_internal_data() = std::move(routes_internal_data);

            return proto_router;
        }

    } // namespace detail_serialization

    namespace detail_deserialization {

        Coordinates CreateCoord(const transport_proto::Coordinates& proto_coord) {
            Coordinates coord;

            coord.lat = proto_coord.lat();
            coord.lng = proto_coord.lng();

            return coord;
        }

        transport_catalogue::stop_catalogue::Stop CreateStop(const transport_proto::Stop& proto_stop) {
            transport_catalogue::stop_catalogue::Stop stop;

            stop.name = proto_stop.name();
            stop.coord = CreateCoord(proto_stop.coord());

            return stop;
        }

        transport_catalogue::bus_catalogue::Bus CreateBus(const transport_proto::Bus& proto_bus, const request_handler::RequestHandler& rh) {
            transport_catalogue::bus_catalogue::Bus bus;

            bus.name = proto_bus.name();
            for (int i = 0; i < proto_bus.route_size(); ++i) {
                const transport_catalogue::stop_catalogue::Stop* stop = rh.GetStopById(proto_bus.route(i));
                bus.route.push_back(stop);
            }
            bus.route_type = transport_catalogue::RouteTypeFromInt(proto_bus.type());
            bus.route_geo_length = proto_bus.route_geo_length();
            bus.route_true_length = proto_bus.route_true_length();
            bus.stops_on_route = proto_bus.stops_on_route();
            bus.unique_stops = proto_bus.unique_stops();

            return bus;
        }

        svg::Point CreatePoint(const transport_proto::Point& proto_point) {
            svg::Point point;

            point.x = proto_point.x();
            point.y = proto_point.y();

            return point;
        }

        svg::Color CreateColor(const transport_proto::Color& proto_color) {
            svg::Color color; // monostate by default

            svg::ColorType type = svg::ColorTypeFromInt(proto_color.type());
            if (type == svg::ColorType::STRING) {
                color = proto_color.name();
            }
            else if (type == svg::ColorType::RGB) {
                color = svg::Rgb::FromStringView(proto_color.name());
            }
            else if (type == svg::ColorType::RGBA) {
                color = svg::Rgba::FromStringView(proto_color.name());
            }

            return color;
        }

        map_renderer::MapRendererSettings CreateMapRenderSettings(const transport_proto::MapRenderSettings& proto_settings) {
            map_renderer::MapRendererSettings settings;

            settings.width = proto_settings.width();
            settings.height = proto_settings.height();
            settings.padding = proto_settings.padding();
            settings.line_width = proto_settings.line_width();
            settings.stop_radius = proto_settings.stop_radius();
            settings.bus_label_font_size = proto_settings.bus_label_font_size();
            settings.bus_label_offset = CreatePoint(proto_settings.bus_label_offset());
            settings.stop_label_font_size = proto_settings.stop_label_font_size();
            settings.stop_label_offset = CreatePoint(proto_settings.stop_label_offset());
            settings.underlayer_color = CreateColor(proto_settings.underlayer_color());
            settings.underlayer_width = proto_settings.underlayer_width();
            for (int i = 0; i < proto_settings.color_palette_size(); ++i) {
                settings.color_palette.push_back(CreateColor(proto_settings.color_palette(i)));
            }

            return settings;
        }

        transport_catalogue::RouteSettings CreateRouteSettings(const transport_proto::RouteSettings& proto_settings) {
            transport_catalogue::RouteSettings settings;

            settings.bus_velocity = proto_settings.bus_velocity();
            settings.bus_wait_time = proto_settings.bus_waiting_time();

            return settings;
        }

        graph::Edge<transport_graph::TransportTime> CreateEdge(const transport_proto::Edge& proto_edge) {
            graph::Edge<transport_graph::TransportTime> edge{};

            edge.from = proto_edge.from();
            edge.to = proto_edge.to();
            edge.weight = proto_edge.weight();

            return edge;
        }

        graph::DirectedWeightedGraph<transport_graph::TransportTime>::IncidenceList CreateIncidenceList(const transport_proto::IncidenceList& proto_list) {
            graph::DirectedWeightedGraph<transport_graph::TransportTime>::IncidenceList list;

            for (int i = 0; i < proto_list.id_size(); ++i) {
                list.push_back(proto_list.id(i));
            }

            return list;
        }

        transport_graph::TransportGraphData CreateTransportGraphData(
            const transport_proto::TransportGraphData& proto_data, request_handler::RequestHandler& rh) {
            transport_graph::TransportGraphData data{};

            data.from = rh.GetStopById(proto_data.stop_from_id());
            data.to = rh.GetStopById(proto_data.stop_to_id());
            if (proto_data.is_bus()) {
                data.bus = rh.GetBusById(proto_data.bus_id());
            }
            else {
                data.bus = nullptr;
            }
            data.stop_count = proto_data.stop_count();
            data.time = proto_data.time();

            return data;
        }

        transport_graph::VertexIdLoop CreateVertexIdLoop(const transport_proto::VertexIdLoop& proto_vertex_id_loop) {
            transport_graph::VertexIdLoop vertex_id_loop;

            vertex_id_loop.id = proto_vertex_id_loop.id();
            vertex_id_loop.transfer_id = proto_vertex_id_loop.transfer_id();

            return vertex_id_loop;
        }

        transport_graph::TransportGraph CreateGraph(const transport_proto::Graph& proto_graph, request_handler::RequestHandler& rh) {
            using namespace transport_graph;

            std::vector<graph::Edge<TransportTime>> edges;
            std::vector<graph::DirectedWeightedGraph<TransportTime>::IncidenceList> incidence_lists;
            std::unordered_map<graph::EdgeId, TransportGraphData> edge_id_to_graph_data;
            std::unordered_map<const stop_catalogue::Stop*, VertexIdLoop> stop_to_vertex_id;

            for (int i = 0; i < proto_graph.edge_size(); ++i) {
                edges.push_back(CreateEdge(proto_graph.edge(i)));
            }

            for (int i = 0; i < proto_graph.incidence_list_size(); ++i) {
                incidence_lists.push_back(CreateIncidenceList(proto_graph.incidence_list(i)));
            }

            for (const auto& [proto_edge_id, proto_transport_graph_data] : proto_graph.edge_id_to_graph_data()) {
                edge_id_to_graph_data.emplace(proto_edge_id, CreateTransportGraphData(proto_transport_graph_data, rh));
            }

            for (const auto& [proto_stop_id, proto_vertex_id_loop] : proto_graph.stop_to_vertex_id()) {
                stop_to_vertex_id.emplace(rh.GetStopById(proto_stop_id), CreateVertexIdLoop(proto_vertex_id_loop));
            }

            TransportGraphDeserialization deserializer;

            deserializer.CreateGraph(std::move(edges), std::move(incidence_lists));
            deserializer.SetEdgeIdToGraphData(std::move(edge_id_to_graph_data));
            deserializer.SetStopToVertexId(std::move(stop_to_vertex_id));

            return deserializer.Build();
        }

        graph::Router<transport_graph::TransportTime>::RouteInternalData CreateRouteInternalData(const transport_proto::RouteInternalData& proto_data) {
            graph::Router<transport_graph::TransportTime>::RouteInternalData data;

            data.weight = proto_data.weight();
            if (proto_data.is_prev_edge()) {
                data.prev_edge = proto_data.prev_edge_id();
            }

            return data;
        }

        transport_graph::TransportRouter CreateRouter(const transport_graph::TransportGraph* ptr_graph, const transport_proto::Router& proto_router) {
            using namespace graph;
            using namespace transport_graph;

            Router<TransportTime>::RoutesInternalData routes_internal_data;

            for (int i = 0; i < proto_router.routes_internal_data().routes_internal_data_vector_size(); ++i) {
                const auto& proto_data_vector = proto_router.routes_internal_data().routes_internal_data_vector(i);

                std::vector<std::optional<Router<TransportTime>::RouteInternalData>> routes_internal_data_vector(proto_data_vector.size());

                for (int j = 0; j < proto_data_vector.route_internal_data_size(); ++j) {
                    const auto& proto_internal_data = proto_data_vector.route_internal_data(j);

                    routes_internal_data_vector.at(proto_internal_data.pos()) = CreateRouteInternalData(proto_internal_data);
                }

                routes_internal_data.emplace_back(std::move(routes_internal_data_vector));
            }

            return transport_graph::TransportRouterCreator::Build(
                *ptr_graph,
                graph::RouterCreator<transport_graph::TransportTime>::Build(
                    ptr_graph->GetGraph(),
                    std::move(routes_internal_data)
                )
            );
        }

    } // namespace detail_deserialization

    void Serialize(std::ofstream& out, const request_handler::RequestHandler& rh) {
        using namespace detail_serialization;

        transport_proto::TransportCatalogue tc;

        for (const transport_catalogue::stop_catalogue::Stop* stop : rh.GetStops()) {
            *tc.add_stop() = CreateProtoStop(stop, rh);
        }

        for (const transport_catalogue::bus_catalogue::Bus* bus : rh.GetBuses()) {
            *tc.add_bus() = CreateProtoBus(bus, rh);
        }

        const auto& map_render_settings = rh.GetMapRenderSettings();
        if (map_render_settings) {
            *tc.mutable_map_render_setting() = CreateProtoMapRenderSettings(map_render_settings.value());
        }

        *tc.mutable_route_settings() = CreateProtoRouteSetting(rh.GetRouteSettings());

        if (rh.GetGraph()) {
            *tc.mutable_graph() = CreateProtoGraph(*rh.GetGraph(), rh);
        }

        if (rh.GetRouter()) {
            *tc.mutable_router() = CreateProtoRouter(*rh.GetRouter());
        }

        tc.SerializeToOstream(&out);
    }

    void Deserialize(request_handler::RequestHandler& rh, std::ifstream& in) {
        using namespace detail_deserialization;

        transport_proto::TransportCatalogue tc;
        tc.ParseFromIstream(&in);

        for (int i = 0; i < tc.stop_size(); ++i) {
            const transport_proto::Stop& stop = tc.stop(i);
            rh.AddStop(stop.id(), CreateStop(stop));
        }

        for (int i = 0; i < tc.bus_size(); ++i) {
            const transport_proto::Bus& bus = tc.bus(i);
            rh.AddBus(bus.id(), CreateBus(bus, rh));
        }

        rh.RenderMap(CreateMapRenderSettings(tc.map_render_setting()));

        rh.SetRouteSettings(CreateRouteSettings(tc.route_settings()));

        if (tc.has_graph()) {
            rh.SetGraph(CreateGraph(tc.graph(), rh));
        }

        if (tc.has_router()) {
            rh.SetRouter(CreateRouter(rh.GetGraph(), tc.router()));
        }
    }

} // namespace transport_serialization
