#pragma once

#include <ostream>
#include <string_view>

struct Coordinates {
    double lat = 0.0;
    double lng = 0.0;

    static Coordinates ParseFromStringView(const std::string_view& string_coord);
};

bool operator== (const Coordinates& lhs, const Coordinates& rhs);

bool operator!= (const Coordinates& lhs, const Coordinates& rhs);

std::ostream& operator<< (std::ostream& out, const Coordinates& coord);

double ComputeDistance(Coordinates from, Coordinates to);
