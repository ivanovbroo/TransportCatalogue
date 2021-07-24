#include <algorithm>
#include <iomanip>
#include <numeric>
#include <unordered_set>
#include <utility>

#include "domain.h"

namespace transport_catalogue {

    namespace stop_catalogue {

        using namespace detail;

        std::ostream& operator<<(std::ostream& out, const BusesToStopNames& buses) {
            bool first = true;
            for (const std::string_view& bus : buses) {
                if (!first) {
                    out << std::string(", ");	/// почему не out << ", " или out << ", "sv ?
                }
                out << std::string("\"");	/// почемe не out << "\""sv << bus << "\""sv; ?
                out << bus;
                out << std::string("\"");
                first = false;
            }
            return out;
        }

/// используете в методах только move-семантику, а для некоторых полей это излишне.
/// без дополнительных аналогичных методов с константными ссылками такими методами будет не удобно пользоваться.
/// нельзя будет вызвать просто Push("blabla", "coord"), а только через сохранение в промежуточных переменных
/// лучше тогда переделать в универсальные ссылки

        const Stop* Catalogue::Push(std::string&& name, std::string&& string_coord) {	/// зачем move-семантика для string_coord? в методе дальше не передается
            return Push(std::move(name), Coordinates::ParseFromStringView(string_coord));
        }

        const Stop* Catalogue::Push(std::string&& name, Coordinates&& coord) {		/// Coordinates - тип без динамического выделения памяти, нет смысла использовать move-семантику
            const Stop* stop = CatalogueTemplate::PushData({ std::move(name), std::move(coord) });
            stop_buses_.insert({ stop, {} });
            return stop;
        }

        const Stop* Catalogue::Push(size_t id, std::string&& name, Coordinates&& coord) {
            const Stop* stop = CatalogueTemplate::PushData(id, { std::move(name), std::move(coord) });
            stop_buses_.insert({ stop, {} });
            return stop;
        }

        const Stop* Catalogue::Push(size_t id, Stop&& stop_value) {
            const Stop* stop = CatalogueTemplate::PushData(id, std::move(stop_value));
            stop_buses_.insert({ stop, {} });
            return stop;
        }

        void Catalogue::PushBusToStop(const Stop* stop, const std::string_view& bus_name) {	/// string_view считается простым типом, достаточно передавать по значению
            stop_buses_.at(stop).insert(bus_name);
        }

        void Catalogue::AddDistance(const Stop* stop_1, const Stop* stop_2, double distance) {
            PointerPair<Stop> stop_pair_direct = { stop_1, stop_2 };
            PointerPair<Stop> stop_pair_reverse = { stop_2, stop_1 };

            distances_between_stops_[stop_pair_direct] = distance;

            if (distances_between_stops_.count(stop_pair_reverse) == 0) {
                distances_between_stops_[stop_pair_reverse] = distance;
            }
        }

    } // namespace stop_catalogue

    namespace bus_catalogue {

        using namespace detail;
        using namespace stop_catalogue;

        Bus BusHelper::Build(const stop_catalogue::Catalogue& stops_catalogue) {
            Bus bus;

            for (const std::string_view& stop_name : stop_names_) {
                auto stop = stops_catalogue.At(stop_name);
                if (stop) {
                    bus.route.push_back(*stop);
                }
            }

            bus.name = std::move(name_);	/// а если повторно вызвать Build, вроде как не запрещено, что будет в имени?
            bus.route_type = route_type_;
            bus.route_geo_length = CalcRouteGeoLength(bus.route, route_type_);
            bus.route_true_length = CalcRouteTrueLength(bus.route, stops_catalogue.GetDistances(), route_type_);
            bus.stops_on_route = bus.route.size();
            bus.route_settings = std::move(settings_);

            std::unordered_set<std::string_view> unique_stops_names;
            for (const Stop* stop : bus.route) {
                unique_stops_names.insert(stop->name);
            }
            bus.unique_stops = unique_stops_names.size();

            if (route_type_ == RouteType::BackAndForth && bus.stops_on_route > 0) {
                bus.stops_on_route = bus.stops_on_route * 2 - 1;
            }

            return bus;
        }

        double BusHelper::CalcRouteGeoLength(const std::deque<const Stop*>& route, RouteType route_type) const {
            double length = 0.0;
            if (route.size() > 0) {
                std::vector<double> distance(route.size());
                std::transform(
                    route.begin(), route.end() - 1,
                    route.begin() + 1, distance.begin(),
                    [](const Stop* from, const Stop* to) {
                        return ComputeDistance(from->coord, to->coord);
                    });
                length = std::reduce(distance.begin(), distance.end());

                if (route_type == RouteType::BackAndForth) {
                    length *= 2.0;
                }
            }
            return length;
        }

        double BusHelper::CalcRouteTrueLength(const std::deque<const Stop*>& route, const DistancesContainer& stops_distances, RouteType route_type) const {
            double length = 0.0;
            if (route.size() > 0) {
                std::vector<double> distance(route.size());
                std::transform(
                    route.begin(), route.end() - 1,
                    route.begin() + 1, distance.begin(),
                    [&stops_distances](const Stop* from, const Stop* to) {
                        return stops_distances.at(PointerPair<Stop>{ from, to });
                    });
                length = std::reduce(distance.begin(), distance.end());

                if (route_type == RouteType::BackAndForth) {
                    std::transform(
                        route.rbegin(), route.rend() - 1,
                        route.rbegin() + 1, distance.begin(),
                        [&stops_distances](const Stop* from, const Stop* to) {
                            return stops_distances.at(PointerPair<Stop>{ from, to });
                        });
                    length += std::reduce(distance.begin(), distance.end());
                }
            }
            return length;
        }

        std::ostream& operator<<(std::ostream& out, const Bus& bus) {
/// Замечание не буду делать, но несколько грамозко выглядит (с отдельными константами). Не знаю причины, но лучше так не делать, локализовать будет не удобно
            static const char* str_bus = "Bus ";
            static const char* str_sep = ": ";
            static const char* str_comma = ", ";
            static const char* str_space = " ";
            static const char* str_stops_on_route = "stops on route";
            static const char* str_unique_stops = "unique stops";
            static const char* str_route_length = "route length";
            static const char* str_curvature = "curvature";

            double curvature = bus.route_true_length / bus.route_geo_length;

            out << str_bus << bus.name << str_sep;
            out << bus.stops_on_route << str_space << str_stops_on_route << str_comma;
            out << bus.unique_stops << str_space << str_unique_stops << str_comma;
            out << std::setprecision(6) << bus.route_true_length << str_space << str_route_length << str_comma;
            out << std::setprecision(6) << curvature << str_space << str_curvature;

            return out;
        }
    } // namespace bus_catalogue
} // namespace transport_catalogue