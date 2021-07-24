#pragma once

#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "domain.h"

namespace transport_catalogue {

    class TransportCatalogue {
    public:
        TransportCatalogue() = default;

        void AddBus(bus_catalogue::Bus&& add_bus);

        void AddBus(size_t id, bus_catalogue::Bus&& add_bus);

        void AddStop(std::string&& name, std::string&& string_coord);

        void AddStop(std::string&& name, Coordinates&& coord);

        void AddStop(size_t id, std::string&& name, Coordinates&& coord);

        void AddStop(size_t id, stop_catalogue::Stop&& stop);

        void AddDistanceBetweenStops(const std::string_view& stop_from_name, const std::string_view& stop_to_name, double distance);

        const stop_catalogue::BusesToStopNames& GetBusesForStop(const std::string_view& name) const;

        const stop_catalogue::Catalogue& GetStops() const;

        const bus_catalogue::Catalogue& GetBuses() const;

        void SetBusRouteCommonSettings(RouteSettings&& settings);

        template <typename Type>
        size_t GetId(const Type* data) const {
            if constexpr (std::is_same_v<Type, stop_catalogue::Stop>) {
                return stops_.GetId(data);
            }
            else {
                return buses_.GetId(data);
            }
        }

    private:
        bus_catalogue::Catalogue buses_;
        stop_catalogue::Catalogue stops_;
    };

} // namespace transport_catalogue
