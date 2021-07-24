#include <algorithm>
#include <cmath>
#include <iterator>
#include <stdexcept>
#include <utility>

#include "request_handler.h"
#include "geo.h"
#include "serialization.h"

namespace request_handler {

    using namespace transport_catalogue;
    using namespace map_renderer;

    ProgrammType ParseProgrammType(int argc, const char** argv) {
        using namespace std::literals;

        if (argc >= 2) {
            const std::string_view argument(argv[1]);
            if (argument == "make_base"sv) {
                return ProgrammType::MAKE_BASE;
            }
            else if (argument == "process_requests"sv) {
                return ProgrammType::PROCESS_REQUESTS;
            }
            else if (argument == "old_tests") {
                return ProgrammType::OLD_TESTS;
            }
        }

        return ProgrammType::UNKNOWN;
    }

    // ---------- RequestHandler --------------------------------------------------

    RequestHandler::RequestHandler(transport_catalogue::TransportCatalogue& catalogue)
        : catalogue_(catalogue) {
    }

    void RequestHandler::AddStop(std::string&& name, Coordinates&& coord) {
        catalogue_.AddStop(std::move(name), std::move(coord));
    }

    void RequestHandler::AddStop(size_t id, std::string&& name, Coordinates&& coord) {
        catalogue_.AddStop(id, std::move(name), std::move(coord));
    }

    void RequestHandler::AddDistance(std::string_view name_from, std::string_view name_to, double distance) {
        catalogue_.AddDistanceBetweenStops(name_from, name_to, distance);
    }

    void RequestHandler::AddBus(transport_catalogue::bus_catalogue::BusHelper&& bus_helper) {
        catalogue_.AddBus(std::move(bus_helper.Build(catalogue_.GetStops())));
    }

    void RequestHandler::RenderMap(MapRendererSettings&& settings) {
        map_render_settings_ = std::move(settings);

        MapRenderer render(
            map_render_settings_.value(),
            catalogue_.GetStops(),
            catalogue_.GetBuses());

        std::ostringstream oss;
        render.Render(oss);

        map_renderer_value_ = oss.str();
    }

    bool RequestHandler::IsRouteValid(
        const transport_catalogue::stop_catalogue::Stop* from,
        const transport_catalogue::stop_catalogue::Stop* to,
        const transport_catalogue::bus_catalogue::Bus* bus,
        int span, double time) const {
        (void)span;
        (void)time;

        if (from != to) {
            if (!bus) {
                return false;
            }

            auto it_from = std::find(bus->route.rbegin(), bus->route.rend(), from);
            if (it_from == bus->route.rend()) {
                return false;
            }

            auto it_to = std::find(bus->route.rbegin(), bus->route.rend(), to);
            if (it_to == bus->route.rend()) {
                return false;
            }
        }
        return true;
    }

    std::optional<RequestHandler::RouteData> RequestHandler::GetRoute(std::string_view from, std::string_view to) const {
        using namespace transport_graph;

        InitRouter();

        auto stop_from = catalogue_.GetStops().At(from);
        auto stop_to = catalogue_.GetStops().At(to);

        if (stop_from && stop_to) {
            return router_->GetRoute(*stop_from, *stop_to);
        }
        else {
            return std::nullopt;
        }
    }

    void RequestHandler::InitRouter() const {
        using namespace transport_graph;

        if (!graph_) {
            graph_ = std::make_unique<TransportGraph>(catalogue_);
        }

        if (!router_) {
            router_ = std::make_unique<TransportRouter>(*graph_);
        }
    }

    std::vector<const transport_catalogue::stop_catalogue::Stop*> RequestHandler::GetStops() const {
        std::vector<const transport_catalogue::stop_catalogue::Stop*> stops;

        for (const auto& [name, stop] : catalogue_.GetStops()) {
            stops.push_back(stop);
        }

        return stops;
    }

    std::vector<const transport_catalogue::bus_catalogue::Bus*> RequestHandler::GetBuses() const {
        std::vector<const transport_catalogue::bus_catalogue::Bus*> buses;

        for (const auto& [name, bus] : catalogue_.GetBuses()) {
            buses.push_back(bus);
        }

        return buses;
    }

    namespace detail_base {

        void RequestBaseStopProcess(
            RequestHandler& request_handler,
            const json::Node* node) {
            using namespace std::literals;

            const json::Dict& request = node->AsMap();

            std::string name = request.at("name"s).AsString();

            request_handler.AddStop(std::move(name), Coordinates{ request.at("latitude"s).AsDouble(), request.at("longitude"s).AsDouble() });
        }

        RouteSettings CreateRouteSettings(const std::unordered_map<std::string_view, const json::Node*> input_route_settings) {
            using namespace std::literals;

            RouteSettings settings{};

            if (!input_route_settings.empty()) {
                settings.bus_velocity = input_route_settings.at("bus_velocity"sv)->AsDouble();
                settings.bus_wait_time = input_route_settings.at("bus_wait_time"sv)->AsInt();
            }

            return settings;
        }

        transport_catalogue::bus_catalogue::BusHelper RequestBaseBusProcess(const json::Node* node) {
            using namespace std::literals;
            using namespace bus_catalogue;

            const json::Dict& request = node->AsMap();

            std::string name = request.at("name"s).AsString();

            transport_catalogue::RouteType type = (request.at("is_roundtrip"s).AsBool())
                ? transport_catalogue::RouteType::Round
                : transport_catalogue::RouteType::BackAndForth;

            std::vector<std::string_view> route;
            for (const json::Node& node_stops : request.at("stops"s).AsArray()) {
                route.push_back(node_stops.AsString());
            }

            return BusHelper().SetName(std::move(name)).SetStopNames(std::move(route)).SetRouteType(type);
        }

        svg::Color ParseColor(const json::Node* node) {
            using namespace std::string_literals;

            if (node->IsString()) {
                return svg::Color(node->AsString());
            }
            else if (node->IsArray()) {
                const json::Array& array_color = node->AsArray();
                int r = array_color.at(0).AsInt();
                int g = array_color.at(1).AsInt();
                int b = array_color.at(2).AsInt();
                if (array_color.size() == 3) {
                    return svg::Color(svg::Rgb(r, g, b));
                }
                else {
                    double opacity = array_color.at(3).AsDouble();
                    return svg::Color(svg::Rgba(r, g, b, opacity));
                }
            }
            throw json::ParsingError("ParseColor error"s);
            return svg::Color{};
        }

        std::vector<svg::Color> ParsePaletteColors(const json::Array& array_color_palette) {
            std::vector<svg::Color> color_palette;
            for (const json::Node& node : array_color_palette) {
                color_palette.push_back(std::move(ParseColor(&node)));
            }
            return color_palette;
        }

        svg::Point ParseOffset(const json::Array& offset) {
            return svg::Point{ offset.at(0).AsDouble(), offset.at(1).AsDouble() };
        }

        void RequestBaseMapProcess(
            RequestHandler& request_handler,
            const std::unordered_map<std::string_view, const json::Node*>& render_settings) {
            using namespace std::literals;

            MapRendererSettings settings;

            settings.width = render_settings.at("width"sv)->AsDouble();
            settings.height = render_settings.at("height"sv)->AsDouble();
            settings.padding = render_settings.at("padding"sv)->AsDouble();
            settings.line_width = render_settings.at("line_width"sv)->AsDouble();
            settings.stop_radius = render_settings.at("stop_radius"sv)->AsDouble();
            settings.bus_label_font_size = render_settings.at("bus_label_font_size"sv)->AsInt();
            settings.bus_label_offset = std::move(ParseOffset(render_settings.at("bus_label_offset"sv)->AsArray()));
            settings.stop_label_font_size = render_settings.at("stop_label_font_size"sv)->AsInt();
            settings.stop_label_offset = std::move(ParseOffset(render_settings.at("stop_label_offset"sv)->AsArray()));
            settings.underlayer_color = std::move(ParseColor(render_settings.at("underlayer_color"sv)));
            settings.underlayer_width = render_settings.at("underlayer_width"sv)->AsDouble();
            settings.color_palette = std::move(ParsePaletteColors(render_settings.at("color_palette"sv)->AsArray()));

            request_handler.RenderMap(std::move(settings));
        }

    } // namespace detail_base

    namespace detail_stat {

        void RequestStatStopProcess(
            json::Builder& builder,
            const RequestHandler& request_handler,
            const json::Dict& request) {
            using namespace std::literals;
            using namespace transport_catalogue::stop_catalogue;

            std::string_view name = request.at("name"s).AsString();
            int id = request.at("id"s).AsInt();

            if (request_handler.DoesStopExist(name)) {
                json::Array buses_arr;
                for (const std::string_view bus : request_handler.GetStopBuses(name)) {
                    buses_arr.push_back(std::string(bus));
                }

                builder
                    .StartDict()
                    .Key("buses"s).Value(std::move(buses_arr))
                    .Key("request_id"s).Value(id)
                    .EndDict();
            }
            else {
                builder
                    .StartDict()
                    .Key("error_message"s).Value("not found"s)
                    .Key("request_id"s).Value(id)
                    .EndDict();
            }
        }

        void RequestStatBusProcess(
            json::Builder& builder,
            const RequestHandler& request_handler,
            const json::Dict& request) {
            using namespace std::literals;

            std::string_view name = request.at("name"s).AsString();
            int id = request.at("id"s).AsInt();

            const auto opt_bus = request_handler.GetBus(name);

            if (opt_bus) {
                const transport_catalogue::bus_catalogue::Bus* bus = *opt_bus;
                double curvature = (std::abs(bus->route_geo_length) > 1e-6) ? bus->route_true_length / bus->route_geo_length : 0.0;

                builder
                    .StartDict()
                    .Key("curvature"s).Value(curvature)
                    .Key("request_id"s).Value(id)
                    .Key("route_length"s).Value(bus->route_true_length)
                    .Key("stop_count"s).Value(static_cast<int>(bus->stops_on_route))
                    .Key("unique_stop_count"s).Value(static_cast<int>(bus->unique_stops))
                    .EndDict();
            }
            else {
                builder
                    .StartDict()
                    .Key("error_message"s).Value("not found"s)
                    .Key("request_id"s).Value(id)
                    .EndDict();
            }
        }

        void RequestMapProcess(
            json::Builder& builder,
            const RequestHandler& request_handler,
            const json::Dict& request) {
            using namespace std::literals;

            if (!request_handler.GetMap()) {
                throw std::logic_error("Map hasn't been rendered!"s);
            }

            int id = request.at("id"s).AsInt();

            builder
                .StartDict()
                .Key("map"s).Value(*request_handler.GetMap())
                .Key("request_id"s).Value(id)
                .EndDict();
        }

        void RequestRouteProcess(
            json::Builder& builder,
            const RequestHandler& request_handler,
            const json::Dict& request) {
            using namespace std::literals;

            std::string_view name_from = request.at("from"s).AsString();
            std::string_view name_to = request.at("to"s).AsString();

            int id = request.at("id"s).AsInt();

            const auto route_data = request_handler.GetRoute(name_from, name_to);

            if (route_data) {
                double check_total_time = 0.0;
                builder.StartDict()
                    .Key("items"s)
                    .StartArray();
                
                for (const auto& [from, to, bus, span, time] : route_data->route) {
                    assert(request_handler.IsRouteValid(from, to, bus, span, time));
                    check_total_time += time;

                    if (from == to) {
                        builder.StartDict()
                            .Key("stop_name"s).Value(from->name)
                            .Key("time"s).Value(time)
                            .Key("type"s).Value("Wait"s)
                            .EndDict();
                    }
                    else {
                        builder.StartDict()
                            .Key("bus"s).Value(bus->name)
                            .Key("span_count"s).Value(span)
                            .Key("time"s).Value(time)
                            .Key("type"s).Value("Bus"s)
                            .EndDict();
                    }
                }
                assert(std::abs(check_total_time - route_data->time) < 1e-6);
                
                builder.EndArray()
                    .Key("request_id"s).Value(id)
                    .Key("total_time"s).Value(route_data->time)
                    .EndDict();
            }
            else {
                builder
                    .StartDict()
                    .Key("error_message"s).Value("not found"s)
                    .Key("request_id"s).Value(id)
                    .EndDict();
            }
        }

        void RequestStatProcess(
            json::Builder& builder,
            const RequestHandler& request_handler,
            const json::Node* node) {
            using namespace std::literals;

            const json::Dict& request = node->AsMap();
            std::string_view type = request.at("type"s).AsString();

            if (type == "Stop"sv) {
                RequestStatStopProcess(builder, request_handler, request);
            }
            else if (type == "Bus"sv) {
                RequestStatBusProcess(builder, request_handler, request);
            }
            else if (type == "Map"sv) {
                RequestMapProcess(builder, request_handler, request);
            }
            else if (type == "Route"sv) {
                RequestRouteProcess(builder, request_handler, request);
            }
            else {
                throw json::ParsingError("Unknown type "s + std::string(type) + " in RequestStatProcess"s);
            }
        }

    } // namespace detail_stat

    void RequestHandlerProcess::RunOldTests() {
        ExecuteBaseProcess();
        ExecuteStatProcess();
    }

    void RequestHandlerProcess::ExecuteMakeBaseRequests() {
        using namespace std::literals;

        ExecuteBaseProcess();

        handler_.InitRouter();

        std::ofstream out(
            reader_.SerializationSettings().at("file"sv)->AsString(),
            std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);

        transport_serialization::Serialize(out, handler_);
    }

    void RequestHandlerProcess::ExecuteProcessRequests() {
        using namespace std::literals;

        std::ifstream in(
            reader_.SerializationSettings().at("file"sv)->AsString(),
            std::ofstream::in | std::ofstream::binary);

        transport_serialization::Deserialize(handler_, in);

        ExecuteStatProcess();
    }

    void RequestHandlerProcess::ExecuteBaseProcess() {
        {
            // Добавляемые остановки
            for (const json::Node* node : reader_.StopRequests()) {
                detail_base::RequestBaseStopProcess(handler_, node);
            }
        }

        {
            // Добавляемые реальные расстояния между остановками
            for (const auto& [name_from, distances] : reader_.RoadDistances()) {
                for (const auto& [name_to, distance] : *distances) {
                    handler_.AddDistance(name_from, name_to, distance.AsDouble());
                }
            }
        }

        {
            // Создаём переменную с настройками маршрута
            catalogue_.SetBusRouteCommonSettings(detail_base::CreateRouteSettings(reader_.RoutingSettings()));

            // Добавляемые автобусные маршруты
            for (const json::Node* node : reader_.BusRequests()) {
                bus_catalogue::BusHelper helper = detail_base::RequestBaseBusProcess(node);
                handler_.AddBus(std::move(helper));
            }
        }

        {
            // Инициализируем карту маршрутов
            if (!reader_.RenderSettings().empty()) {
                detail_base::RequestBaseMapProcess(handler_, reader_.RenderSettings());
            }
        }
    }

    void RequestHandlerProcess::ExecuteStatProcess() {
        json::Builder builder;

        {
            // Получаем результат
            builder.StartArray();
            for (const json::Node* node : reader_.StatRequests()) {
                detail_stat::RequestStatProcess(builder, handler_, node);
            }
            builder.EndArray();
        }

        {
            // Выводим результат
            json::Print(json::Document(builder.Build()), output_);
        }
    }

} // namespace request_handler
