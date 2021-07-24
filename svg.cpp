#include "svg.h"

namespace svg {

    using namespace std::literals;
    using namespace std::string_literals;

    // ---------- Point -----------------------------------------------------------

    std::string Point::Str() const {
        std::ostringstream oss;
        static const std::string comma = std::string(",");
        oss << x << comma << y;
        return oss.str();
    }

    // ---------- Object ----------------------------------------------------------

    void Object::Render(const RenderContext& context) const {
        context.RenderIndent();

        // Делегируем вывод тега своим подклассам
        RenderObject(context);

        context.out << std::endl;
    }

    // ---------- Circle ----------------------------------------------------------

    Circle& Circle::SetCenter(Point center) {
        center_ = std::move(center);
        return *this;
    }

    Circle& Circle::SetRadius(double radius) {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << radius_ << "\""sv;
        RenderAttrs(out);
        out << "/>"sv;
    }

    // ---------- Polyline --------------------------------------------------------

    Polyline& Polyline::AddPoint(Point point) {
        if (!points_.empty()) {
            points_ += " "s;
        }
        points_ += point.Str();
        return *this;
    }

    void Polyline::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<polyline points=\""sv << points_ << "\""sv;
        RenderAttrs(out);
        out << "/>"sv;
    }

    // ---------- Text ------------------------------------------------------------

    Text& Text::SetPosition(Point pos) {
        pos_ = std::move(pos);
        return *this;
    }

    Text& Text::SetOffset(Point offset) {
        offset_ = std::move(offset);
        return *this;
    }

    Text& Text::SetFontSize(uint32_t size) {
        size_ = size;
        return *this;
    }

    Text& Text::SetFontFamily(std::string font_family) {
        font_family_ = std::move(font_family);
        return *this;
    }

    Text& Text::SetFontWeight(std::string font_weight) {
        font_weight_ = std::move(font_weight);
        return *this;
    }

    Text& Text::SetData(std::string data) {
        static const std::unordered_map<char, std::string> svg_style = {
            {'"', "&quot;"s},
            {'\'', "&apos;"s},
            {'<', "&lt;"s},
            {'>', "&gt;"s},
            {'&', "&amp;"s}
        };

        data_.clear();
        data_.reserve(data.size());
        for (char c : data) {
            if (svg_style.count(c)) {
                data_.append(svg_style.at(c));
            }
            else {
                data_.append(1, c);
            }
        }

        return *this;
    }

    void Text::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<text"sv;
        RenderAttrs(out);
        out << " x=\""sv << pos_.x << "\" y=\""sv << pos_.y << "\""sv;
        out << " dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\""sv;
        out << " font-size=\""sv << size_ << "\""sv;
        if (font_family_) {
            out << " font-family=\""sv << *font_family_ << "\""sv;
        }
        if (font_weight_) {
            out << " font-weight=\""sv << *font_weight_ << "\""sv;
        }
        out << ">"sv << data_ << "</text>"sv;
    }

    // ---------- Document --------------------------------------------------------

    void Document::AddPtr(std::unique_ptr<Object>&& obj) {
        objects_.emplace_back(std::move(obj));
    }

    void Document::Render(std::ostream& out) const {
        RenderContext render_context(out, 4, 2);

        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
        for (const std::unique_ptr<Object>& object : objects_) {
            object->Render(render_context);
        }
        out << "</svg>"sv;
    }

    const Document::Objects& Document::GetObjects() const {
        return objects_;
    }

}  // namespace svg
