#pragma once

#include <algorithm>
#include <cstdint>
#include <deque>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>

namespace svg {

    // ---------- Point -----------------------------------------------------------

    struct Point {
        Point() = default;
        Point(double x, double y)
            : x(x)
            , y(y) {
        }
        std::string Str() const;
        double x = 0.0;
        double y = 0.0;
    };

    inline bool operator== (const Point& lhs, const Point& rhs) {
        return (std::abs(lhs.x - rhs.x) < 1e-6) && (std::abs(lhs.y - rhs.y) < 1e-6);
    }

    inline bool operator!= (const Point& lhs, const Point& rhs) {
        return !(lhs == rhs);
    }

    // ---------- RenderContext ---------------------------------------------------

    /*
    * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
    * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
    */
    struct RenderContext {
        RenderContext(std::ostream& out)
            : out(out) {
        }

        RenderContext(std::ostream& out, int indent_step, int indent = 0)
            : out(out)
            , indent_step(indent_step)
            , indent(indent) {
        }

        RenderContext Indented() const {
            return { out, indent_step, indent + indent_step };
        }

        void RenderIndent() const {
            for (int i = 0; i < indent; ++i) {
                out.put(' ');
            }
        }

        std::ostream& out;
        int indent_step = 0;
        int indent = 0;
    };

    // ---------- Object ----------------------------------------------------------

    /*
    * Абстрактный базовый класс Object служит для унифицированного хранения
    * конкретных тегов SVG-документа
    * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
    */
    class Object {
    public:
        void Render(const RenderContext& context) const;

        virtual ~Object() = default;

    private:
        virtual void RenderObject(const RenderContext& context) const = 0;
    };

    // ---------- Rgb -------------------------------------------------------------

#define TO_INT(c) (static_cast<int>(c))

    struct Rgb {
        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;

        Rgb() = default;

        Rgb(uint8_t red, uint8_t green, uint8_t blue)
            : red(red)
            , green(green)
            , blue(blue) {
        }

        static Rgb FromStringView(std::string_view str) {
            size_t from = str.find_first_of('(') + 1;
            size_t to = str.find_first_of(',', from);
            uint8_t r = std::stoi(std::string(str.substr(from, to - from)));
            from = to + 1;
            to = str.find_first_of(',', from);
            uint8_t g = std::stoi(std::string(str.substr(from, to - from)));
            from = to + 1;
            to = str.find_first_of(')', from);
            uint8_t b = std::stoi(std::string(str.substr(from, to - from)));
            return { r, g, b };
        }
    };

    inline std::ostream& operator<< (std::ostream& out, const Rgb& rgb) {
        using namespace std::literals;
        out << "rgb("sv << TO_INT(rgb.red) << ","sv << TO_INT(rgb.green) << ","sv << TO_INT(rgb.blue) << ")"sv;
        return out;
    }

    // ---------- Rgba ------------------------------------------------------------

    struct Rgba : public Rgb {
        double opacity = 1.0;

        Rgba() = default;

        Rgba(uint8_t red, uint8_t green, uint8_t blue, double opacity)
            : Rgb(red, green, blue)
            , opacity(opacity) {
        }

        static Rgba FromStringView(std::string_view str) {
            size_t from = str.find_first_of('(') + 1;
            size_t to = str.find_first_of(',', from);
            uint8_t r = std::stoi(std::string(str.substr(from, to - from)));
            from = to + 1;
            to = str.find_first_of(',', from);
            uint8_t g = std::stoi(std::string(str.substr(from, to - from)));
            from = to + 1;
            to = str.find_first_of(',', from);
            uint8_t b = std::stoi(std::string(str.substr(from, to - from)));
            from = to + 1;
            to = str.find_first_of(')', from);
            double o = std::stod(std::string(str.substr(from, to - from)));
            return { r, g, b, o };
        }
    };

    inline std::ostream& operator<< (std::ostream& out, const Rgba& rgba) {
        using namespace std::literals;
        out << "rgba("sv << TO_INT(rgba.red) << ","sv << TO_INT(rgba.green) << ","sv << TO_INT(rgba.blue) << ","sv << rgba.opacity << ")"sv;
        return out;
    }

    // ---------- Color -----------------------------------------------------------

    enum class ColorType {
        MONOSTATE = 1,
        STRING = 2,
        RGB = 3,
        RGBA = 4,
        UNKNOWN = 5
    };

    inline uint32_t ColorTypeToInt(ColorType type) {
        return static_cast<uint32_t>(type);
    }

    inline ColorType ColorTypeFromInt(uint32_t type) {
        if (type == 1) {
            return ColorType::MONOSTATE;
        }
        else if (type == 2) {
            return ColorType::STRING;
        }
        else if (type == 3) {
            return ColorType::RGB;
        }
        else if (type == 4) {
            return ColorType::RGBA;
        }
        return ColorType::UNKNOWN;
    }

    using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;

    inline const std::string NoneColor = std::string("none");
    inline const std::string EmptyStringColor = std::string("");

    struct ColorPrinter {
        std::string_view operator()(std::monostate) const {
            out << NoneColor;
            return EmptyStringColor;
        }

        std::string_view operator()(const std::string& color) const {
            out << color;
            return EmptyStringColor;
        }

        std::string_view operator()(const Rgb& rgb) const {
            out << rgb;
            return EmptyStringColor;
        }

        std::string_view operator()(const Rgba& rgba) const {
            out << rgba;
            return EmptyStringColor;
        }

        std::ostream& out;
    };

#define PRINT_COLOR(out, color) (std::visit(ColorPrinter{out}, color))

    inline std::string GetColorStringName(const Color& color) {
        std::stringstream ss;
        PRINT_COLOR(ss, color);
        return ss.str();
    }

    struct ColorTypeGetter {
        ColorType operator()(std::monostate) const {
            return ColorType::MONOSTATE;
        }

        ColorType operator()(const std::string&) const {
            return ColorType::STRING;
        }

        ColorType operator()(const Rgb&) const {
            return ColorType::RGB;
        }

        ColorType operator()(const Rgba&) const {
            return ColorType::RGBA;
        }
    };

    inline ColorType GetColorType(const Color& color) {
        return std::visit(ColorTypeGetter{}, color);
    }

    // ---------- PathProps -------------------------------------------------------

    /*
    * Класс содержит свойства, управляющие параметрами заливки и контура
    * Реализует паттерн "Curiously Recurring Template Pattern" (CRTP)
    */

    enum class StrokeLineCap {
        BUTT,
        ROUND,
        SQUARE,
    };

    inline const std::unordered_map<StrokeLineCap, std::string> GetStrokeLineCapName = {
        {StrokeLineCap::BUTT, std::string("butt")},
        {StrokeLineCap::ROUND, std::string("round")},
        {StrokeLineCap::SQUARE, std::string("square")}
    };

    inline std::ostream& operator<< (std::ostream& out, StrokeLineCap cap) {
        out << GetStrokeLineCapName.at(cap);
        return out;
    }

    enum class StrokeLineJoin {
        ARCS,
        BEVEL,
        MITER,
        MITER_CLIP,
        ROUND,
    };

    inline const std::unordered_map<StrokeLineJoin, std::string> GetStrokeLineJoinName = {
        {StrokeLineJoin::ARCS, std::string("arcs")},
        {StrokeLineJoin::BEVEL, std::string("bevel")},
        {StrokeLineJoin::MITER, std::string("miter")},
        {StrokeLineJoin::MITER_CLIP, std::string("miter-clip")},
        {StrokeLineJoin::ROUND, std::string("round")}
    };

    inline std::ostream& operator<< (std::ostream& out, StrokeLineJoin join) {
        out << GetStrokeLineJoinName.at(join);
        return out;
    }

    template <typename Owner>
    class PathProps {
    public:
        // Задаёт цвет заливки
        Owner& SetFillColor(Color color);

        // Задаёт цвет контура
        Owner& SetStrokeColor(Color color);

        // Задаёт толщину линии
        Owner& SetStrokeWidth(double width);

        // Задаёт тип формы конца линии
        Owner& SetStrokeLineCap(StrokeLineCap line_cap);

        // Задаёт тип формы соединения линий
        Owner& SetStrokeLineJoin(StrokeLineJoin line_join);

    protected:
        ~PathProps() = default;

        void RenderAttrs(std::ostream& out) const;

    private:
        Owner& AsOwner();

        std::optional<Color> fill_color_;
        std::optional<Color> stroke_color_;
        std::optional<double> stroke_width_;
        std::optional<StrokeLineCap> stroke_line_cap_;
        std::optional<StrokeLineJoin> stroke_line_join_;
    };

    template<typename Owner>
    inline Owner& PathProps<Owner>::SetFillColor(Color color) {
        fill_color_ = std::move(color);
        return AsOwner();
    }

    template<typename Owner>
    inline Owner& PathProps<Owner>::SetStrokeColor(Color color) {
        stroke_color_ = std::move(color);
        return AsOwner();
    }

    template<typename Owner>
    inline Owner& PathProps<Owner>::SetStrokeWidth(double width) {
        stroke_width_ = width;
        return AsOwner();
    }

    template<typename Owner>
    inline Owner& PathProps<Owner>::SetStrokeLineCap(StrokeLineCap line_cap) {
        stroke_line_cap_ = line_cap;
        return AsOwner();
    }

    template<typename Owner>
    inline Owner& PathProps<Owner>::SetStrokeLineJoin(StrokeLineJoin line_join) {
        stroke_line_join_ = line_join;
        return AsOwner();
    }

    template<typename Owner>
    void PathProps<Owner>::RenderAttrs(std::ostream& out) const {
        using namespace std::literals;

        if (fill_color_) {
            out << " fill=\""sv << PRINT_COLOR(out, *fill_color_) << "\""sv;
        }
        if (stroke_color_) {
            out << " stroke=\""sv << PRINT_COLOR(out, *stroke_color_) << "\""sv;
        }
        if (stroke_width_) {
            out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
        }
        if (stroke_line_cap_) {
            out << " stroke-linecap=\""sv << *stroke_line_cap_ << "\""sv;
        }
        if (stroke_line_join_) {
            out << " stroke-linejoin=\""sv << *stroke_line_join_ << "\""sv;
        }
    }

    template<typename Owner>
    Owner& PathProps<Owner>::AsOwner() {
        // static_cast безопасно преобразует *this к Owner&,
        // если класс Owner - наследник PathProps
        return static_cast<Owner&>(*this);
    }

    // ---------- Circle ----------------------------------------------------------

    /*
    * Класс Circle моделирует элемент <circle> для отображения круга
    * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
    */
    class Circle final : public Object, public PathProps<Circle> {
    public:
        // Задаёт координаты центра окружности
        Circle& SetCenter(Point center);

        // Задаёт радиус окружности
        Circle& SetRadius(double radius);

    private:
        void RenderObject(const RenderContext& context) const override;

    private:
        Point center_;
        double radius_ = 1.0;
    };

    // ---------- Polyline --------------------------------------------------------

    /*
    * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
    * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
    */
    class Polyline : public Object, public PathProps<Polyline> {
    public:
        // Добавляет очередную вершину к ломаной линии
        Polyline& AddPoint(Point point);

    private:
        void RenderObject(const RenderContext& context) const override;

    private:
        std::string points_;
    };

    // ---------- Text ------------------------------------------------------------

    /*
    * Класс Text моделирует элемент <text> для отображения текста
    * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
    */
    class Text : public Object, public PathProps<Text> {
    public:
        // Задаёт координаты опорной точки (атрибуты x и y)
        Text& SetPosition(Point pos);

        // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
        Text& SetOffset(Point offset);

        // Задаёт размеры шрифта (атрибут font-size)
        Text& SetFontSize(uint32_t size);

        // Задаёт название шрифта (атрибут font-family)
        Text& SetFontFamily(std::string font_family);

        // Задаёт толщину шрифта (атрибут font-weight)
        Text& SetFontWeight(std::string font_weight);

        // Задаёт текстовое содержимое объекта (отображается внутри тега text)
        Text& SetData(std::string data);

    private:
        void RenderObject(const RenderContext& context) const override;

    private:
        Point pos_;
        Point offset_;
        uint32_t size_ = 1;
        std::optional<std::string> font_family_;
        std::optional<std::string> font_weight_;
        std::string data_;
    };

    // ---------- ObjectContainer -------------------------------------------------

    class ObjectContainer {
    public:
        // Метод Add добавляет в svg-контейнер любой объект-наследник svg::Object
        // Паттерн проектирования "Шаблонный метод"
        template <typename Obj>
        void Add(Obj obj) {
            AddPtr(std::make_unique<Obj>(std::move(obj)));
        }

        // Добавляет в svg-контейнер объект-наследник svg::Object
        virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;

    protected:
        ~ObjectContainer() = default;
    };

    // ---------- Document --------------------------------------------------------

    class Document : public ObjectContainer {
        using Objects = std::deque<std::unique_ptr<Object>>;
    public:
        void AddPtr(std::unique_ptr<Object>&& obj) override;

        // Выводит в ostream svg-представление документа
        void Render(std::ostream& out) const;

        const Objects& GetObjects() const;

    private:
        Objects objects_;
    };

    // ---------- Drawable --------------------------------------------------------

    class Drawable {
    public:
        virtual void Draw(ObjectContainer& container) const = 0;

        virtual ~Drawable() = default;
    };

}  // namespace svg
