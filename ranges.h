#pragma once

#include <iterator>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "domain.h"

namespace ranges {

    template <typename It, typename Ptr = void>
    class Range {
    public:
        using ValueType = typename std::iterator_traits<It>::value_type;

        Range(It begin, It end, const Ptr* ptr = nullptr)
            : begin_(begin)
            , end_(end)
            , ptr_(ptr) {
        }

        const It& begin() const {
            return begin_;
        }

        const It& end() const {
            return end_;
        }

        const Ptr* GetPtr() const {
            return ptr_;
        }

    private:
        It begin_;
        It end_;
        const Ptr* ptr_ = nullptr;
    };

    template <typename C, typename Ptr = void>
    inline auto AsRange(const C& container, const Ptr* ptr = nullptr) {
        return Range{ container.begin(), container.end(), ptr };
    }

    template <typename It>
    class BusRange : public Range<It, transport_catalogue::bus_catalogue::Bus> {
    public:
        explicit BusRange(It begin, It end, const transport_catalogue::bus_catalogue::Bus* ptr)
            : Range<It, transport_catalogue::bus_catalogue::Bus>(begin, end, ptr) {
        }
    };

    inline auto AsBusRangeDirect(const transport_catalogue::bus_catalogue::Bus* bus) {
        return BusRange{ bus->route.begin(), bus->route.end(), bus };
    }

    inline auto AsBusRangeReversed(const transport_catalogue::bus_catalogue::Bus* bus) {
        return BusRange{ bus->route.rbegin(), bus->route.rend(), bus };
    }

}  // namespace ranges
