#include <algorithm>
#include <cmath>
#include <string>

#include "geo.h"

inline bool InTheVicinity(const double d1, const double d2, const double delta = 1e-6) {
    return std::abs(d1 - d2) < delta;
}

Coordinates Coordinates::ParseFromStringView(const std::string_view& string_coord) {
    size_t start_pos = string_coord.find_first_not_of(' ');
    size_t comma_pos = string_coord.find_first_of(',');
    size_t start_pos_next = string_coord.find_first_not_of(' ', comma_pos + 1);
    Coordinates coord;
    coord.lat = std::stod(std::string(string_coord.substr(start_pos, comma_pos)));
    coord.lng = std::stod(std::string(string_coord.substr(start_pos_next)));
    return coord;
}

bool operator==(const Coordinates& lhs, const Coordinates& rhs) {
    return InTheVicinity(lhs.lat, rhs.lat) && InTheVicinity(lhs.lng, rhs.lng);
}

bool operator!=(const Coordinates& lhs, const Coordinates& rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& out, const Coordinates& coord) {
    using namespace std::string_literals;
    out << "<|"s << coord.lat << ", "s << coord.lng << "|>"s;
    return out;
}

double ComputeDistance(Coordinates from, Coordinates to) {
    static const double dr = 3.14159265358979323846 / 180.0;
    static const double rz = 6371000.0;
    return std::acos(
        std::sin(from.lat * dr) * std::sin(to.lat * dr) +
        std::cos(from.lat * dr) * std::cos(to.lat * dr) * cos(std::abs(from.lng - to.lng) * dr)) * rz;
}
