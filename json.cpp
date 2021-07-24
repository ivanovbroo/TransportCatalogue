#include <utility>

#include "json.h"

namespace json {

    // ---------- NodePrinter -----------------------------------------------------

    void NodePrinter::operator() (std::nullptr_t) const {
        using namespace std::literals;
        helper.PrintIndent();
        out << "null"sv;
    }

    void NodePrinter::operator() (const std::string& value) const {
        using namespace std::literals;

        static const std::map<char, std::string> escapes = {
            {'\"', "\\\""s},
            {'\\', "\\\\"s},
            {'\t', "\\t"s},
            {'\r', "\\r"s},
            {'\n', "\\n"s}
        };

        helper.StartString();
        for (char c : value) {
            if (escapes.count(c)) {
                out << escapes.at(c);
            }
            else {
                out << c;
            }
        }
        helper.FinishString();
    }

    void NodePrinter::operator() (bool value) const {
        helper.PrintIndent();
        out << std::boolalpha << value;
    }

    void NodePrinter::operator() (int value) const {
        helper.PrintIndent();
        out << value;
    }

    void NodePrinter::operator() (double value) const {
        helper.PrintIndent();
        out << value;
    }

    void NodePrinter::operator() (const Array& value) const {
        using namespace std::literals;
        bool first = true;
        helper.StartArray();
        for (const Node& node : value) {
            if (!first) {
                helper.NextArrayValue();
            }
            std::visit(*this, node.Data());
            first = false;
        }
        helper.FinishArray();
    }

    void NodePrinter::operator() (const Dict& value) const {
        using namespace std::literals;
        bool first = true;
        helper.StartMap();
        for (const auto& [name, node] : value) {
            if (!first) {
                helper.NextMapPair();
            }
            this->operator()(name);
            helper.NextMapValue();
            std::visit(*this, node.Data());
            first = false;
        }
        helper.FinishMap();
    }

    // ---------- Node ------------------------------------------------------------

    const std::string& Node::AsString() const {
        if (!IsString()) {
            throw std::logic_error("Node data is not string format");
        }
        return std::get<std::string>(Data());
    }

    bool Node::AsBool() const {
        if (!IsBool()) {
            throw std::logic_error("Node data is not bool format");
        }
        return std::get<bool>(Data());
    }

    int Node::AsInt() const {
        if (!IsInt()) {
            throw std::logic_error("Node data is not int format");
        }
        return std::get<int>(Data());
    }

    double Node::AsDouble() const {
        if (!IsDouble()) {
            throw std::logic_error("Node data is not double format");
        }
        return (IsPureDouble()) ? std::get<double>(Data()) : static_cast<double>(AsInt());
    }

    const Array& Node::AsArray() const {
        if (!IsArray()) {
            throw std::logic_error("Node data is not Array format");
        }
        return std::get<Array>(Data());
    }

    const Dict& Node::AsMap() const {
        if (!IsMap()) {
            throw std::logic_error("Node data is not Dict format");
        }
        return std::get<Dict>(Data());
    }

    const Dict& Node::AsDict() const {
        return AsMap();
    }

    std::string& Node::AsString() {
        if (!IsString()) {
            throw std::logic_error("Node data is not string format");
        }
        return std::get<std::string>(Data());
    }

    Array& Node::AsArray() {
        if (!IsArray()) {
            throw std::logic_error("Node data is not Array format");
        }
        return std::get<Array>(Data());
    }

    Dict& Node::AsMap() {
        if (!IsMap()) {
            throw std::logic_error("Node data is not Dict format");
        }
        return std::get<Dict>(Data());
    }

    Dict& Node::AsDict() {
        return AsMap();
    }

    bool Node::IsNull() const {
        return std::holds_alternative<std::nullptr_t>(Data());
    }

    bool Node::IsString() const {
        return std::holds_alternative<std::string>(Data());
    }

    bool Node::IsBool() const {
        return std::holds_alternative<bool>(Data());
    }

    bool Node::IsInt() const {
        return std::holds_alternative<int>(Data());
    }

    bool Node::IsDouble() const {
        return std::holds_alternative<double>(Data()) || IsInt();
    }

    bool Node::IsPureDouble() const {
        return std::holds_alternative<double>(Data());
    }

    bool Node::IsArray() const {
        return std::holds_alternative<Array>(Data());
    }

    bool Node::IsMap() const {
        return std::holds_alternative<Dict>(Data());
    }

    bool Node::IsDict() const {
        return IsMap();
    }

    const Node::NodeData& Node::Data() const {
        return *this;
    }

    Node::NodeData& Node::Data() {
        return *this;
    }

    bool operator== (const Node& lhs, const Node& rhs) {
        return lhs.Data() == rhs.Data();
    }

    bool operator!= (const Node& lhs, const Node& rhs) {
        return !(lhs == rhs);
    }

} // namespace json
