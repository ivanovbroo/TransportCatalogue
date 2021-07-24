#include <algorithm>
#include <utility>

#include "map_renderer.h"

namespace map_renderer {

    MapRendererCreator::MapRendererCreator(MapRendererSettings&& render_settings)
        : render_settings_(std::move(render_settings)) {
    }

    svg::Polyline MapRendererCreator::CreateLine(int color_index) {
        svg::Polyline polyline;
        polyline.SetFillColor(svg::Color());
        polyline.SetStrokeWidth(render_settings_.line_width);
        polyline.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
        polyline.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        polyline.SetStrokeColor(render_settings_.color_palette.at(color_index % render_settings_.color_palette.size()));
        return polyline;
    }

    svg::Text MapRendererCreator::CreateBusText() {
        using namespace std::string_literals;
        svg::Text text;
        text.SetOffset(render_settings_.bus_label_offset);
        text.SetFontSize(render_settings_.bus_label_font_size);
        text.SetFontFamily("Verdana"s);
        text.SetFontWeight("bold"s);
        return text;
    }

    svg::Text MapRendererCreator::CreateUnderlayerBusText() {
        svg::Text text = CreateBusText();
        text.SetFillColor(render_settings_.underlayer_color);
        text.SetStrokeColor(render_settings_.underlayer_color);
        text.SetStrokeWidth(render_settings_.underlayer_width);
        text.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
        text.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        return text;
    }

    svg::Text MapRendererCreator::CreateDataBusText(int color_index) {
        svg::Text text = CreateBusText();
        text.SetFillColor(render_settings_.color_palette.at(color_index % render_settings_.color_palette.size()));
        return text;
    }

    svg::Text MapRendererCreator::CreateStopText() {
        using namespace std::string_literals;
        svg::Text text;
        text.SetOffset(render_settings_.stop_label_offset);
        text.SetFontSize(render_settings_.stop_label_font_size);
        text.SetFontFamily("Verdana"s);
        return text;
    }

    svg::Text MapRendererCreator::CreateUnderlayerStopText() {
        svg::Text text = CreateStopText();
        text.SetFillColor(render_settings_.underlayer_color);
        text.SetStrokeColor(render_settings_.underlayer_color);
        text.SetStrokeWidth(render_settings_.underlayer_width);
        text.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
        text.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        return text;
    }

    svg::Text MapRendererCreator::CreateDataStopText() {
        using namespace std::string_literals;
        svg::Text text = CreateStopText();
        text.SetFillColor(svg::Color("black"s));
        return text;
    }

    svg::Circle MapRendererCreator::CreateCircle(const svg::Point& center) {
        using namespace std::string_literals;
        svg::Circle circle;
        circle.SetCenter(center);
        circle.SetRadius(render_settings_.stop_radius);
        circle.SetFillColor(svg::Color("white"s));
        return circle;
    } 

    MapRenderer::MapRenderer(
        MapRendererSettings render_settings,
        const transport_catalogue::stop_catalogue::Catalogue& stops,
        const transport_catalogue::bus_catalogue::Catalogue& buses)
        : MapRendererCreator(std::move(render_settings)) {
        InitNotEmptyStops(stops);
        InitNotEmptyBuses(buses);
        CalculateZoomCoef();
        CalculateStopZoomedCoords();
        DrawLines();
        DrawBusText();
        DrawStopCircles();
        DrawStopText();
    }

    void MapRenderer::InitNotEmptyStops(
        const transport_catalogue::stop_catalogue::Catalogue& stops) {
        for (const auto& [name, stop] : stops) {
            if (!stops.IsEmpty(stop)) {
                stop_point_.emplace(stop, svg::Point{});
                stops_.emplace(name, stop);
            }
        }
    }

    void MapRenderer::InitNotEmptyBuses(
        const transport_catalogue::bus_catalogue::Catalogue& buses) {
        for (const auto& [name, bus] : buses) {
            if (bus->stops_on_route > 0) {
                buses_.emplace(name, bus);
            }
        }
    }

    void MapRenderer::CalculateZoomCoef() {
        const auto [bottom_it, top_it] = std::minmax_element(stop_point_.begin(), stop_point_.end(),
            [](const auto& lhs, const auto& rhs) {
                return lhs.first->coord.lat < rhs.first->coord.lat;
            });

        const auto [left_it, right_it] = std::minmax_element(stop_point_.begin(), stop_point_.end(),
            [](const auto& lhs, const auto& rhs) {
                return lhs.first->coord.lng < rhs.first->coord.lng;
            });

        double min_lat = bottom_it->first->coord.lat;
        double max_lat = top_it->first->coord.lat;
        double min_lon = left_it->first->coord.lng;
        double max_lon = right_it->first->coord.lng;

        double delta_lat = max_lat - min_lat;
        double delta_lon = max_lon - min_lon;

        double height_coef = (std::abs(delta_lat) > 1e-6) ? ((render_settings_.height - 2.0 * render_settings_.padding) / delta_lat) : 0.0;
        double width_coef = (std::abs(delta_lon) > 1e-6) ? ((render_settings_.width - 2.0 * render_settings_.padding) / delta_lon) : 0.0;

        zoom_coef_ = std::min(height_coef, width_coef);
        min_longitude_ = min_lon;
        max_latitude_ = max_lat;
    }

    void MapRenderer::CalculateStopZoomedCoords() {
        for (auto& [stop, point] : stop_point_) {
            point.x = (stop->coord.lng - min_longitude_) * zoom_coef_ + render_settings_.padding;
            point.y = (max_latitude_ - stop->coord.lat) * zoom_coef_ + render_settings_.padding;
        }
    }

    void MapRenderer::DrawLines() {
        size_t color_index = 0;
        for (const auto& [name, bus] : buses_) {
            svg::Polyline polyline = CreateLine(color_index++);

            for (const auto& stop : bus->route) {
                polyline.AddPoint(stop_point_.at(stop));
            }

            if (bus->route_type == transport_catalogue::RouteType::BackAndForth && !bus->route.empty()) {
                for (auto it = bus->route.rbegin() + 1; it != bus->route.rend(); ++it) {
                    polyline.AddPoint(stop_point_.at(*it));
                }
            }

            Add(std::move(polyline));
        }
    }

    void MapRenderer::DrawBusText() {
        size_t color_index = 0;
        for (const auto& [name, bus] : buses_) {
            svg::Text underlayer_text = CreateUnderlayerBusText().SetData(std::string(name));
            svg::Text data_text = CreateDataBusText(color_index++).SetData(std::string(name));;

            underlayer_text.SetPosition(stop_point_.at(bus->route.front()));
            data_text.SetPosition(stop_point_.at(bus->route.front()));

            Add(underlayer_text);
            Add(data_text);

            if (bus->route_type == transport_catalogue::RouteType::BackAndForth && !bus->route.empty() && *bus->route.front() != *bus->route.back()) {
                underlayer_text.SetPosition(stop_point_.at(bus->route.back()));
                data_text.SetPosition(stop_point_.at(bus->route.back()));

                Add(std::move(underlayer_text));
                Add(std::move(data_text));
            }
        }
    }

    void MapRenderer::DrawStopCircles() {
        for (const auto& [name, stop] : stops_) {
            Add(std::move(CreateCircle(stop_point_.at(stop))));
        }
    }

    void MapRenderer::DrawStopText() {
        for (const auto& [name, stop] : stops_) {
            svg::Text underlayer_text = CreateUnderlayerStopText().SetData(std::string(name)).SetPosition(stop_point_.at(stop));
            svg::Text data_text = CreateDataStopText().SetData(std::string(name)).SetPosition(stop_point_.at(stop));;

            Add(std::move(underlayer_text));
            Add(std::move(data_text));
        }
    }

} // namespace map_renderer
