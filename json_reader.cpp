#include <map>
#include <set>
#include <string>

#include "json_reader.h"

namespace json {

    // ---------- Document --------------------------------------------------------

    Document::Document(Node root)
        : root_(std::move(root)) {
    }

    const Node& Document::GetRoot() const {
        return root_;
    }

    bool operator== (const Document& lhs, const Document& rhs) {
        return lhs.GetRoot() == rhs.GetRoot();
    }

    bool operator!= (const Document& lhs, const Document& rhs) {
        return !(lhs.GetRoot() == rhs.GetRoot());
    }

    namespace detail {

#define TO_STR(str) (std::string(str))

        static const std::set<char> skip_this = { ' ', '\t', '\r', '\n', ':' };

        Node LoadNode(std::istream& input);

        std::string LoadStringCount(std::istream& input, size_t count) {
            std::string line;
            for (char c; count > 0 && input >> c;) {
                line += c;
                count--;
            }
            return line;
        }

        bool LoadTypeCompare(std::istream& input, const std::string& check_word) {
            return check_word == LoadStringCount(input, check_word.size());
        }

        void LoadTypeCheck(std::istream& input, const std::string& check_word, const std::string& error_msg) {
            if (!LoadTypeCompare(input, check_word)) {
                throw ParsingError(error_msg);
            }
        }

#define LOAD_NULL_CHECK(input) (LoadTypeCheck(input, TO_STR("null"), TO_STR("Json LoadNull error")))
#define LOAD_TRUE_CHECK(input) (LoadTypeCheck(input, TO_STR("true"), TO_STR("Json LoadTrue error")))
#define LOAD_FALSE_CHECK(input) (LoadTypeCheck(input, TO_STR("false"), TO_STR("Json LoadFalse error")))

        Node LoadNull(std::istream& input) {
            LOAD_NULL_CHECK(input);
            return Node();
        }

        Node LoadString(std::istream& input) {
            static const std::map<char, char> escape{
                {'r', '\r'},
                {'n', '\n'},
                {'t', '\t'}
            };

            std::string line;

            input >> std::noskipws;

            if (input.peek() == '\"') {
                input.get();
            }

            bool is_closed = false;

            for (char c; input >> c;) {
                is_closed = (c == '\"');
                if (is_closed) {
                    break;
                }
                if (c == '\\') {
                    input >> c;
                    if (escape.count(c)) {
                        line += escape.at(c);
                    }
                    else {
                        line += c;
                    }
                }
                else {
                    line += c;
                }
            }

            if (!is_closed) {
                throw ParsingError(TO_STR("Quote must be closed in string"));
            }

            return Node(move(line));
        }

        Node LoadTrue(std::istream& input) {
            LOAD_TRUE_CHECK(input);
            return Node(true);
        }

        Node LoadFalse(std::istream& input) {
            LOAD_FALSE_CHECK(input);
            return Node(false);
        }

        Node LoadNumber(std::istream& input) {
            using namespace std::literals;

            std::string parsed_num;

            // Считывает в parsed_num очередной символ из input
            auto read_char = [&parsed_num, &input] {
                parsed_num += static_cast<char>(input.get());
                if (!input) {
                    throw ParsingError("Failed to read number from stream"s);
                }
            };

            // Считывает одну или более цифр в parsed_num из input
            auto read_digits = [&input, read_char] {
                if (!std::isdigit(input.peek())) {
                    throw ParsingError("A digit is expected"s);
                }
                while (std::isdigit(input.peek())) {
                    read_char();
                }
            };

            if (input.peek() == '-') {
                read_char();
            }
            // Парсим целую часть числа
            if (input.peek() == '0') {
                read_char();
                // После 0 в JSON не могут идти другие цифры
            }
            else {
                read_digits();
            }

            bool is_int = true;
            // Парсим дробную часть числа
            if (input.peek() == '.') {
                read_char();
                read_digits();
                is_int = false;
            }

            // Парсим экспоненциальную часть числа
            if (int ch = input.peek(); ch == 'e' || ch == 'E') {
                read_char();
                if (ch = input.peek(); ch == '+' || ch == '-') {
                    read_char();
                }
                read_digits();
                is_int = false;
            }

            try {
                if (is_int) {
                    // Сначала пробуем преобразовать строку в int
                    try {
                        return Node(std::stoi(parsed_num));
                    }
                    catch (...) {
                        // В случае неудачи, например, при переполнении
                        // код ниже попробует преобразовать строку в double
                    }
                }
                return Node(std::stod(parsed_num));
            }
            catch (...) {
                throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
            }
        }

        Node LoadArray(std::istream& input) {
            Array result;

            bool is_closed = false;
            for (char c; input >> c;) {
                if (is_closed = (c == ']'); is_closed) {
                    break;
                }
                if (skip_this.count(c) > 0 || c == ',') {
                    continue;
                }
                input.putback(c);
                result.push_back(LoadNode(input));
            }

            if (!is_closed) {
                throw ParsingError(TO_STR("Brackets must be closed in Array"));
            }

            return Node(move(result));
        }

        Node LoadDict(std::istream& input) {
            Dict result;

            bool is_closed = false;
            for (char c; input >> c;) {
                if (is_closed = (c == '}'); is_closed) {
                    break;
                }
                if (skip_this.count(c) > 0 || c == ',') {
                    continue;
                }

                std::string key = LoadString(input).AsString();
                input >> c;
                result.insert({ std::move(key), LoadNode(input) });
            }

            if (!is_closed) {
                throw ParsingError(TO_STR("Brackets must be closed in Dict"));
            }

            return Node(std::move(result));
        }

        Node LoadNode(std::istream& input) {
            char c;
            input >> c;

            if (skip_this.count(c) > 0) {
                return LoadNode(input);
            }
            else if (c == '[') {
                return LoadArray(input);
            }
            else if (c == '{') {
                return LoadDict(input);
            }
            else if (c == '"') {
                return LoadString(input);
            }
            else if (c == 'n') {
                input.putback(c);
                return LoadNull(input);
            }
            else if (c == 't') {
                input.putback(c);
                return LoadTrue(input);
            }
            else if (c == 'f') {
                input.putback(c);
                return LoadFalse(input);
            }
            else {
                input.putback(c);
                return LoadNumber(input);
            }
        }

    } // namespace detail

    Document Load(std::istream& input) {
        return Document{ detail::LoadNode(input) };
    }

    void Print(const Document& doc, std::ostream& output) {
        std::visit(NodePrinter{ output }, doc.GetRoot().Data());
    }

    // ---------- Reader ----------------------------------------------------------

    Reader::Reader(std::istream& input)
        : doc_(std::move(json::Load(input))) {
        using namespace std::string_literals;

        const json::Dict& input_requests = doc_.GetRoot().AsMap();

        if (input_requests.count("base_requests"s) > 0) {
            InitBaseRequests(input_requests.at("base_requests"s).AsArray());
        }
        if (input_requests.count("stat_requests"s) > 0) {
            InitStatRequests(input_requests.at("stat_requests"s).AsArray());
        }
        InitRenderSettings(input_requests);
        InitRoutingSettings(input_requests);
        InitSerializationSettings(input_requests);
    }

    void Reader::InitBaseRequests(const json::Array& base_requests) {
        using namespace std::literals;

        for (const json::Node& node_request : base_requests) {
            const json::Dict& request = node_request.AsMap();
            std::string_view type = request.at("type"s).AsString();
            if (type == "Stop"sv) {
                stop_requests_.push_back(&node_request);
                road_distances_.insert({
                    request.at("name"s).AsString(),
                    &request.at("road_distances"s).AsMap()
                    });
            }
            else if (type == "Bus"sv) {
                bus_requests_.push_back(&node_request);
            }
            else {
                throw json::ParsingError("Unknown type \""s + std::string(type) + "\""s);
            }
        }
    }

    void Reader::InitStatRequests(const json::Array& stat_requests) {
        for (const json::Node& node_request : stat_requests) {
            stat_requests_.push_back(&node_request);
        }
    }

    void Reader::InitRenderSettings(const json::Dict& input_requests) {
        using namespace std::literals;
        static const json::Dict empty_dict{};

        bool is_render_settings = (input_requests.count("render_settings"s) > 0);

        for (const auto& [name, node] : (is_render_settings) ? input_requests.at("render_settings"s).AsMap() : empty_dict) {
            render_settings_.emplace(name, &node);
        }
    }

    void Reader::InitRoutingSettings(const json::Dict& routing_settings) {
        using namespace std::literals;
        static const json::Dict empty_dict{};

        bool is_routing_settings = (routing_settings.count("routing_settings"s) > 0);

        for (const auto& [name, node] : (is_routing_settings) ? routing_settings.at("routing_settings"s).AsMap() : empty_dict) {
            routing_settings_.emplace(name, &node);
        }
    }

    void Reader::InitSerializationSettings(const json::Dict& serialization_settings) {
        using namespace std::literals;
        static const json::Dict empty_dict{};

        bool is_serialization_settings = (serialization_settings.count("serialization_settings"s) > 0);

        for (const auto& [name, node] : (is_serialization_settings) ? serialization_settings.at("serialization_settings"s).AsMap() : empty_dict) {
            serialization_settings_.emplace(name, &node);
        }
    }

} // namespace json
