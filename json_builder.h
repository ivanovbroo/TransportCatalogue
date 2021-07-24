#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "json.h"

namespace json {

    namespace detail {

        struct NodeGetter {
            Node operator() (std::nullptr_t) const;
            Node operator() (std::string&& value) const;
            Node operator() (bool&& value) const;
            Node operator() (int&& value) const;
            Node operator() (double&& value) const;
            Node operator() (Array&& value) const;
            Node operator() (Dict&& value) const;
        };

    } // namespace detail

    class DictItemContext;
    class KeyItemContext;
    class ArrayItemContext;

    class Builder {
    public:
        Builder() = default;

        Builder(size_t reserve) {
            nodes_stack_.reserve(reserve);
        }

        // Задаёт строковое значение ключа для очередной пары ключ-значение
        KeyItemContext Key(std::string&& value);

        // Задаёт значение, соответствующее ключу при определении словаря, очередной элемент массива всё содержимое конструируемого JSON-объекта
        Builder& Value(Node::Value&& value);

        // Начинает определение сложного значения-словаря
        DictItemContext StartDict();

        // Начинает определение сложного значения-массива
        ArrayItemContext StartArray();

        // Завершает определение сложного значения-словаря
        Builder& EndDict();

        // Завершает определение сложного значения-массива
        Builder& EndArray();

        // Возвращает объект json::Node, содержащий JSON, описанный предыдущими вызовами методов
        Node Build();

    private:
        Node root_;
        std::vector<std::unique_ptr<Node>> nodes_stack_;
        int array_counter_ = 0;
        int dict_counter_ = 0;
    };

    class ItemContext {
    public:
        ItemContext(Builder& builder)
            : builder_(builder) {
        }

    protected:
        Builder& Get() {
            return builder_;
        }

    private:
        Builder& builder_;
    };

    class KeyItemContext : public ItemContext {
    public:
        KeyItemContext(Builder& builder)
            : ItemContext(builder) {
        }

        DictItemContext Value(Node::Value&& value);

        DictItemContext StartDict();

        ArrayItemContext StartArray();
    };

    class DictItemContext : public ItemContext {
    public:
        DictItemContext(Builder& builder)
            : ItemContext(builder) {
        }

        KeyItemContext Key(std::string&& value);

        Builder& EndDict();
    };

    class ArrayItemContext : public ItemContext {
    public:
        ArrayItemContext(Builder& builder)
            : ItemContext(builder) {
        }

        ArrayItemContext Value(Node::Value&& value);

        DictItemContext StartDict();

        ArrayItemContext StartArray();

        Builder& EndArray();
    };

} // namespace json
