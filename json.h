#pragma once

#include <istream>
#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace json {

    // ---------- ParsingError ----------------------------------------------------

    // Эта ошибка должна выбрасываться при ошибках парсинга JSON
    class ParsingError : public std::runtime_error {
    public:
        using runtime_error::runtime_error;
    };

        class Node;
    using Dict = std::map<std::string, Node>;
    using Array = std::vector<Node>;

    // ---------- NodePrinter -----------------------------------------------------

    class NodePrinterHelper {
    public:
        explicit NodePrinterHelper(std::ostream& out, char c = ' ')
            : out_(out)
            , c_(c) {
        }

        void PrintIndent() const {
            if (!is_map_value_) {
                for (size_t i = 0; i < indent_; ++i) {
                    out_ << c_;
                }
            }
            else {
                out_ << c_;
                is_map_value_ = false;
            }
        }

        void StartArray() const {
            PrintIndent();
            out_ << '[' << '\n';
            indent_ += indent_step_;
        }

        void FinishArray() const {
            indent_ -= indent_step_;
            out_ << '\n';
            PrintIndent();
            out_ << ']';
        }

        void StartMap() const {
            PrintIndent();
            out_ << '{' << '\n';
            indent_ += indent_step_;
        }

        void FinishMap() const {
            indent_ -= indent_step_;
            out_ << '\n';
            PrintIndent();
            out_ << '}';
        }

        void StartString() const {
            PrintIndent();
            out_ << '\"';
        }

        void FinishString() const {
            out_ << '\"';
        }

        void NextMapValue() const {
            out_ << ':';
            is_map_value_ = true;
        }

        void NextMapPair() const {
            out_ << ',' << '\n';
        }

        void NextArrayValue() const {
            out_ << ',' << '\n';
        }

    private:
        std::ostream& out_;
        char c_;
        mutable size_t indent_ = 0;
        mutable bool is_map_value_ = false;
        size_t indent_step_ = 4;
    };

    struct NodePrinter {
        void operator() (std::nullptr_t) const;
        void operator() (const std::string& value) const;
        void operator() (bool value) const;
        void operator() (int value) const;
        void operator() (double value) const;
        void operator() (const Array& value) const;
        void operator() (const Dict& value) const;

        NodePrinter(std::ostream& out)
            : out(out)
            , helper(NodePrinterHelper(out)) {
        }

        std::ostream& out;
        NodePrinterHelper helper;
    };

    // ---------- Node ------------------------------------------------------------

    class Node : private std::variant<std::nullptr_t, std::string, bool, int, double, Array, Dict> {
    public:
        using variant::variant;
        using NodeData = variant;
        using Value = variant;

    public:
        const std::string& AsString() const;
        bool AsBool() const;
        int AsInt() const;
        double AsDouble() const;
        const Array& AsArray() const;
        const Dict& AsMap() const;
        const Dict& AsDict() const;

        std::string& AsString();
        Array& AsArray();
        Dict& AsMap();
        Dict& AsDict();

        bool IsNull() const;
        bool IsString() const;
        bool IsBool() const;
        bool IsInt() const;
        bool IsDouble() const;
        bool IsPureDouble() const;
        bool IsArray() const;
        bool IsMap() const;
        bool IsDict() const;

        const NodeData& Data() const;
        NodeData& Data();
    };

    bool operator== (const Node& lhs, const Node& rhs);

    bool operator!= (const Node& lhs, const Node& rhs);

} // namespace json
