#include "json_builder.h"

namespace json {

    namespace detail {

        Node NodeGetter::operator()(std::nullptr_t) const {
            return Node();
        }

        Node NodeGetter::operator()(std::string&& value) const {
            return Node(std::move(value));
        }

        Node NodeGetter::operator()(bool&& value) const {
            return Node(value);
        }

        Node NodeGetter::operator()(int&& value) const {
            return Node(value);
        }

        Node NodeGetter::operator()(double&& value) const {
            return Node(value);
        }

        Node NodeGetter::operator()(Array&& value) const {
            return Node(std::move(value));
        }

        Node NodeGetter::operator()(Dict&& value) const {
            return Node(std::move(value));
        }

    } // namespace detail

    KeyItemContext Builder::Key(std::string&& value) {
        if (nodes_stack_.empty() || !nodes_stack_.back()->IsDict()) {
            throw std::logic_error("Key method expects a Dict(map) as the last Node");
        }
        nodes_stack_.emplace_back(std::make_unique<Node>(std::move(value)));
        return KeyItemContext{ *this };
    }

    Builder& Builder::Value(Node::Value&& value) {
        Node node = std::visit(detail::NodeGetter{}, std::move(value));
        if (nodes_stack_.empty()) {
            nodes_stack_.emplace_back(std::make_unique<Node>(std::move(node)));
        }
        else if (nodes_stack_.back()->IsArray() && array_counter_ > 0) {
            Array& value = nodes_stack_.back()->AsArray();
            value.push_back(std::move(node));
        }
        else if (nodes_stack_.back()->IsString() && dict_counter_ > 0) {
            Node key_node = *nodes_stack_.back();
            nodes_stack_.pop_back();
            Dict& value = nodes_stack_.back()->AsDict();
            value.insert({ key_node.AsString(), std::move(node) });
        }
        else {
            throw std::logic_error("All objects have been done");
        }
        return *this;
    }

    DictItemContext Builder::StartDict() {
        if (!nodes_stack_.empty() && !(nodes_stack_.back()->IsString() || nodes_stack_.back()->IsArray())) {
            throw std::logic_error("All objects have been done");
        }
        dict_counter_++;
        nodes_stack_.emplace_back(new Node(Dict{}));
        return DictItemContext{ *this };
    }

    ArrayItemContext Builder::StartArray() {
        if (!nodes_stack_.empty() && !(nodes_stack_.back()->IsString() || nodes_stack_.back()->IsArray())) {
            throw std::logic_error("All objects have been done");
        }
        array_counter_++;
        nodes_stack_.emplace_back(new Node(Array{}));
        return ArrayItemContext{ *this };
    }

    Builder& Builder::EndDict() {
        if (nodes_stack_.empty() || !nodes_stack_.back()->IsDict()) {
            throw std::logic_error("EndDict method could only \"end\" the Dict");
        }
        dict_counter_--;
        Dict value = nodes_stack_.back()->AsDict();
        nodes_stack_.pop_back();
        return Value(std::move(value));
    }

    Builder& Builder::EndArray() {
        if (nodes_stack_.empty() || !nodes_stack_.back()->IsArray()) {
            throw std::logic_error("EndArray method could only \"end\" the Array");
        }
        array_counter_--;
        Array value = nodes_stack_.back()->AsArray();
        nodes_stack_.pop_back();
        return Value(std::move(value));
    }

    Node Builder::Build() {
        if (nodes_stack_.size() != 1) {
            throw std::logic_error("It is expected to be the exactly one Node in the final stack");
        }
        if (array_counter_ < 0) {
            throw std::logic_error("Some Array are closed more than ones");
        }
        if (dict_counter_ < 0) {
            throw std::logic_error("Some Dict(map) are closed more than ones");
        }
        if (array_counter_ > 0) {
            throw std::logic_error("Some Array are not closed");
        }
        if (dict_counter_ > 0) {
            throw std::logic_error("Some Dict(map) are not closed");
        }
        root_ = *nodes_stack_.back();
        nodes_stack_.pop_back();
        return root_;
    }

    DictItemContext KeyItemContext::Value(Node::Value&& value) {
        Get().Value(std::move(value));
        return DictItemContext{ Get() };
    }

    DictItemContext KeyItemContext::StartDict() {
        return Get().StartDict();
    }

    ArrayItemContext KeyItemContext::StartArray() {
        return Get().StartArray();
    }

    KeyItemContext DictItemContext::Key(std::string&& value) {
        return Get().Key(std::move(value));
    }

    Builder& DictItemContext::EndDict() {
        return Get().EndDict();
    }

    ArrayItemContext ArrayItemContext::Value(Node::Value&& value)
    {
        Get().Value(std::move(value));
        return ArrayItemContext{ Get() };
    }

    DictItemContext ArrayItemContext::StartDict() {
        return Get().StartDict();
    }

    ArrayItemContext ArrayItemContext::StartArray() {
        return Get().StartArray();
    }

    Builder& ArrayItemContext::EndArray() {
        return Get().EndArray();
    }

} // namespace json
