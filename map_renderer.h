#pragma once

#include <algorithm>
#include <deque>
#include <iostream>
#include <map>
#include <set>
#include <string_view>

#include "domain.h"
#include "geo.h"
#include "svg.h"

namespace map_renderer {

    struct MapRendererSettings {
        MapRendererSettings() = default;

        // width задаёт ширину области карты в пикселях
        // вещественное число в диапазоне от 0 до 100000
        double width = 0.0;

        // height задаёт высоту области карты в пикселях
        // вещественное число в диапазоне от 0 до 100000.
        double height = 0.0;

        // отступ краёв карты от границ SVG - документа
        // вещественное число не меньше 0 и меньше min(width, height) / 2
        double padding = 0.0;

        // толщина линий, которыми рисуются автобусные маршруты
        // вещественное число в диапазоне от 0 до 100000
        double line_width = 0.0;

        // радиус окружностей, которыми обозначаются остановки
        // вещественное число в диапазоне от 0 до 100000
        double stop_radius = 0.0;

        // целое число, задающее размер шрифта текста, которым отображаются названия автобусных маршрутов
        // целое число в диапазоне от 0 до 100000
        int bus_label_font_size = 0;

        // смещение надписи с названием автобусного маршрута относительно координат конечной остановки на карте
        // задаёт значения свойств dx и dy SVG - элемента <text>
        // массив из двух элементов типа double, задаются в диапазоне от –100000 до 100000
        svg::Point bus_label_offset = {};

        // размер шрифта текста, которым отображаются названия остановок
        // целое число в диапазоне от 0 до 100000.
        int stop_label_font_size = 0;

        // смещение надписи с названием остановки относительно координат остановки на карте
        // задаёт значения свойств dx и dy SVG - элемента <text>
        // массив из двух элементов типа double, задаются в диапазоне от –100000 до 100000
        svg::Point stop_label_offset = {};

        // цвет подложки, отображаемой под названиями остановок и автобусных маршрутов
        svg::Color underlayer_color = {};

        // толщина подложки под названиями остановок и автобусных маршрутов
        // задаёт значение атрибута stroke - width элемента <text>
        // вещественное число в диапазоне от 0 до 100000
        double underlayer_width = 0.0;

        // цветовая палитра, используемая для визуализации маршрутов
        std::vector<svg::Color> color_palette = {};
    };



    class MapRendererCreator {
    public:
        MapRendererCreator(MapRendererSettings&& render_settings);

        svg::Polyline CreateLine(int color_index);

        svg::Text CreateBusText();
        svg::Text CreateUnderlayerBusText();
        svg::Text CreateDataBusText(int color_index);

        svg::Text CreateStopText();
        svg::Text CreateUnderlayerStopText();
        svg::Text CreateDataStopText();

        svg::Circle CreateCircle(const svg::Point& center);

    protected:
        const MapRendererSettings render_settings_;
    };

    class MapRenderer : public svg::Document, private MapRendererCreator {
    public:
        MapRenderer(
            MapRendererSettings render_settings,
            const transport_catalogue::stop_catalogue::Catalogue& stops,
            const transport_catalogue::bus_catalogue::Catalogue& buses);

    private:
        // инициализации массива остановок, через которые проходит хотя бы один маршрут
        void InitNotEmptyStops(
            const transport_catalogue::stop_catalogue::Catalogue& stops);

        // инициализации массива автобусов, которые имеют хотя бы одну остановку
        void InitNotEmptyBuses(
            const transport_catalogue::bus_catalogue::Catalogue& buses);

        // вычисление значения коэффициета масштабирования
        void CalculateZoomCoef();

        // вычисление координат остановок с учётом коэффициента масштабирования
        void CalculateStopZoomedCoords();

        // отрисовка линий маршрутов
        void DrawLines();

        // отрисовка названий маршрутов
        void DrawBusText();

        // отрисовка кругов остановок
        void DrawStopCircles();

        // отрисовка названий автобусов
        void DrawStopText();

    private:
        double zoom_coef_ = 0.0;
        double min_longitude_ = 0.0;
        double max_latitude_ = 0.0;

        std::map<const transport_catalogue::stop_catalogue::Stop*, svg::Point> stop_point_;
        std::map<std::string_view, const transport_catalogue::stop_catalogue::Stop*> stops_;
        std::map<std::string_view, const transport_catalogue::bus_catalogue::Bus*> buses_;
    };

} // namespace map_renderer
