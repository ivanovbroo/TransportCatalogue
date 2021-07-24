#include "transport_catalogue.h"

namespace transport_catalogue {

    void TransportCatalogue::AddBus(bus_catalogue::Bus&& add_bus) {
        const bus_catalogue::Bus* bus = buses_.PushData(std::move(add_bus));

        for (const stop_catalogue::Stop* stop : bus->route) {
            stops_.PushBusToStop(stop, bus->name);
        }
    }

    void TransportCatalogue::AddBus(size_t id, bus_catalogue::Bus&& add_bus) {
        const bus_catalogue::Bus* bus = buses_.PushData(id, std::move(add_bus));

        for (const stop_catalogue::Stop* stop : bus->route) {
            stops_.PushBusToStop(stop, bus->name);
        }
    }

    void TransportCatalogue::AddStop(std::string&& name, std::string&& string_coord) {
        stops_.Push(std::move(name), std::move(string_coord));
    }

    void TransportCatalogue::AddStop(std::string&& name, Coordinates&& coord) {
        stops_.Push(std::move(name), std::move(coord));
    }

    void TransportCatalogue::AddStop(size_t id, std::string&& name, Coordinates&& coord) {
        stops_.Push(id, std::move(name), std::move(coord));
    }

    void TransportCatalogue::AddStop(size_t id, stop_catalogue::Stop&& stop) {
        stops_.Push(id, std::move(stop));
    }

    void TransportCatalogue::AddDistanceBetweenStops(const std::string_view& stop_from_name, const std::string_view& stop_to_name, double distance) {
        auto stop_from = stops_.At(stop_from_name);
        auto stop_to = stops_.At(stop_to_name);
        stops_.AddDistance(*stop_from, *stop_to, distance);
    }

    const stop_catalogue::BusesToStopNames& TransportCatalogue::GetBusesForStop(const std::string_view& name) const {
        static const std::set<std::string_view> empty_set = {};
        auto stop = stops_.At(name);
        return (stop)
            ? stops_.GetBuses(*stop)
            : empty_set;
    }

    const stop_catalogue::Catalogue& TransportCatalogue::GetStops() const {
        return stops_;
    }

    const bus_catalogue::Catalogue& TransportCatalogue::GetBuses() const {
        return buses_;
    }

    void TransportCatalogue::SetBusRouteCommonSettings(RouteSettings&& settings) {
        buses_.SetRouteSettings(std::move(settings));
    }

}
