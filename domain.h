#pragma once

#include <cassert>
#include <deque>
#include <functional>
#include <optional>
#include <set>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "geo.h"

namespace transport_catalogue {

    enum class RouteType {
        Direct = 0,
        BackAndForth = 1,
        Round = 2,
        Unknown = 3
    };

    inline RouteType RouteTypeFromInt(uint32_t type) {
        if (type == 0) {
            return RouteType::Direct;
        }
        else if (type == 1) {
            return RouteType::BackAndForth;
        }
        else {
            return RouteType::Round;
        }
        return RouteType::Unknown;
    }

    struct RouteSettings {
        double bus_velocity = 0.0;
        int bus_wait_time = 0;
    };

    namespace detail {

        template <typename Type>
        class CatalogueTemplate {
        public:
            CatalogueTemplate() = default;

            const Type* PushData(Type&& data) {
                return PushData(id_to_data_.size(), std::move(data));
            }

            const Type* PushData(size_t id, Type&& data) {
                const Type& emplaced = data_.emplace_back(data);
                name_to_data_.emplace(emplaced.name, &emplaced);
                id_to_data_.emplace(id, &emplaced);
                data_to_id_.emplace(&emplaced, id);
                return &emplaced;
            }

            size_t Size() const {
                return data_.size();
            }

            std::optional<const Type*> At(std::string_view name) const {
                if (name_to_data_.count(name) > 0) {
                    return name_to_data_.at(name);
                }
                return std::nullopt;
            }

            std::optional<const Type*> At(size_t id) const {
                if (id_to_data_.count(id) > 0) {
                    return id_to_data_.at(id);
                }
                return std::nullopt;
            }

            size_t GetId(const Type* data) const {
                if (!data || !data_to_id_.count(data)) {
                    throw std::logic_error("Couldn't find this data or data is nullptr");
                }
                return data_to_id_.at(data);
            }

            auto begin() const {
                return name_to_data_.begin();
            }

            auto end() const {
                return name_to_data_.end();
            }

        private:
            std::deque<Type> data_ = {};
            std::unordered_map<std::string_view, const Type*> name_to_data_ = {};
            std::unordered_map<size_t, const Type*> id_to_data_ = {};
            std::unordered_map<const Type*, size_t> data_to_id_ = {};
        };

        template <typename Pointer>
        using PointerPair = std::pair<const Pointer*, const Pointer*>;

        template <typename Pointer>
        class PointerPairHasher {
        public:
            size_t operator() (const PointerPair<Pointer>& stop_pair) const {
                return hasher_(stop_pair.first) * 47 + hasher_(stop_pair.second);
            }
        private:
            std::hash<const Pointer*> hasher_;
        };

    } // namespace detail

    namespace stop_catalogue {

        struct Stop {
            std::string name;
            Coordinates coord;

            bool operator== (const Stop& other) const {
                return name == other.name;
            }

            bool operator!= (const Stop& other) const {
                return !(*this == other);
            }
        };

        using BusesToStopNames = std::set<std::string_view>;
        using DistancesContainer = std::unordered_map<detail::PointerPair<Stop>, double, detail::PointerPairHasher<Stop>>;

        std::ostream& operator<< (std::ostream& out, const BusesToStopNames& buses);

        class Catalogue : public detail::CatalogueTemplate<Stop> {
        public:
            Catalogue() = default;

            const Stop* Push(std::string&& name, std::string&& string_coord);

            const Stop* Push(std::string&& name, Coordinates&& coord);

            const Stop* Push(size_t id, std::string&& name, Coordinates&& coord);

            const Stop* Push(size_t id, Stop&& stop_value);

            void PushBusToStop(const Stop* stop, const std::string_view& bus_name);

            void AddDistance(const Stop* stop_1, const Stop* stop_2, double distance);

            const BusesToStopNames& GetBuses(const Stop* stop) const {
                return stop_buses_.at(stop);
            }

            const DistancesContainer& GetDistances() const {
                return distances_between_stops_;
            }

            bool IsEmpty(const Stop* stop) const {
                return stop_buses_.count(stop) == 0 || stop_buses_.at(stop).empty();
            }

        private:
            std::unordered_map<const Stop*, BusesToStopNames> stop_buses_ = {};
            DistancesContainer distances_between_stops_ = {};
        };
    } // namespace stop_catalogue

    namespace bus_catalogue {

        struct Bus {
            std::string name = {};
            std::deque<const stop_catalogue::Stop*> route = {};
            RouteType route_type = RouteType::Direct;
            double route_geo_length = 0.0;
            double route_true_length = 0.0;
            size_t stops_on_route = 0;
            size_t unique_stops = 0;
            RouteSettings route_settings = {};

            Bus() = default;
        };

        class BusHelper {
        public:
            BusHelper& SetName(std::string&& name) {
                name_ = std::move(name);
                return *this;
            }

            BusHelper& SetRouteType(RouteType route_type) {
                route_type_ = route_type;
                return *this;
            }

            BusHelper& SetStopNames(std::vector<std::string_view>&& stop_names) {
                stop_names_ = std::move(stop_names);
                return *this;
            }

            BusHelper& SetRouteSettings(RouteSettings settings) {
                settings_ = std::move(settings);
                return *this;
            }

            Bus Build(const stop_catalogue::Catalogue& stops_catalogue);

        private:
            double CalcRouteGeoLength(const std::deque<const stop_catalogue::Stop*>& route, RouteType route_type) const;
            double CalcRouteTrueLength(const std::deque<const stop_catalogue::Stop*>& route, const stop_catalogue::DistancesContainer& stops_distances, RouteType route_type) const;

        private:
            std::string name_;
            RouteType route_type_;
            std::vector<std::string_view> stop_names_;
            RouteSettings settings_;
        };

        std::ostream& operator<< (std::ostream& out, const Bus& bus);

        class Catalogue : public detail::CatalogueTemplate<Bus> {
        public:
            Catalogue() = default;

            void SetRouteSettings(RouteSettings&& settings) {
                settings_ = std::move(settings);
            }

            const RouteSettings& GetRouteSettings() const {
                return settings_;
            }

        private:
            RouteSettings settings_ = {};
        };

    } // namespace bus_catalogue

} // namespace transport_catalogue
