#ifndef DATAFLOW_HPP_INCLUDED
#define DATAFLOW_HPP_INCLUDED

#include <functional>
#include <map>
#include <vector>
#include <exception>
#include <algorithm>
#include <utility>
#include <future>
#include <unordered_set>
#include <experimental/optional>
#include <memory>
#include <initializer_list>

namespace dataflow {

// TODO: Add unit test framework
// TODO: prune nodes
// TODO: assert all args are in the nodes of this graph, not another graph
// TODO: can we import/export nodes from/to other graphs?
// TODO: how would you express the graph in a declarative config file, how to move from the realm of type-unsafe config to type-safe c++
// TODO: calculate() method should be a selectable strategy
// TODO: simplfy creation of root values which are currently functions
// TODO: clang-tidy warnings
// TODO: we dont need a graph object as such, just nodes and calculate functions.

template<typename T>
using ptr = std::shared_ptr<T>;
template<typename T>
using wptr = std::weak_ptr<T>;

using NodePtr = ptr<class Node>;
using WeakNodePtr = wptr<class Node>;
using NodePtrList = std::vector<NodePtr>;
using NodePtrSet = std::unordered_set<NodePtr>;
using WeakNodePtrList = std::vector<WeakNodePtr>;

template<typename T> class Value;

template<typename T>
using ValuePtr = ptr<Value<T>>;

template<typename T>
using ValuePtrList = std::vector<ValuePtr<T>>;

template<typename T>
using ValuePtrSet = std::unordered_set<ValuePtr<T>>;


class Node {
    public:
        virtual ~Node() {};
        virtual bool has_value() const = 0;
        virtual int count() const = 0;
        virtual void calculate() = 0;
        virtual int level() const = 0;
        virtual void addChild(NodePtr p) = 0;
        virtual NodePtrList children() const = 0;
        virtual NodePtrList parents() const = 0;
};


template<typename T> class Value :
    public Node {

    private:
        std::function<void()> f_;
        int level_;
        WeakNodePtrList children_;
        WeakNodePtrList parents_;

        std::experimental::optional<T> _value;
        std::experimental::optional<std::exception> _e;
        int _count = 0;

        void init(const NodePtrList& args) {
            std::copy(args.begin(), args.end(), std::back_inserter(parents_));

            auto deepestNode = std::max_element(args.begin(), args.end(), [] (auto p1, auto p2) { return p1->level() < p2->level();} );
            level_ = deepestNode == args.end() ? 0 : (*deepestNode)->level() + 1;
        }


    public:

        template<typename F,  typename ...V>
        Value(const F& f, ValuePtr<V> ... args) {
            this->f_ = [this, f, args...]
                            () {
                                this->set(f(args->get()...));
                            };

            init(NodePtrList{std::initializer_list<NodePtr>{args...}});

        }

        template<template<typename, typename> typename C1, typename A1, template<typename, typename> typename C2, typename A2, typename R, typename V>
        Value(std::function<R(const C1<V, A1>&)> f, const C2<ValuePtr<V>, A2>& args) {
            this->f_ = [this, f, args]
                () {
                    C1<V, A1> vs;
                    std::transform(args.begin(), args.end(), std::back_inserter(vs), [](auto v) {return v->get();});
                    this->set(f(vs));
                };

            init(NodePtrList(args.begin(), args.end()));
        }

    public:

        void set(const std::exception& e) {
            _e = e;
            _value = {};
        }

        void set(const T& value) {
            _e = {};
            _value = value;
            _count++;
        }

        T get() const {
            if(_e) {
                throw _e;
            }

            return _value.value();
        }

        virtual bool has_value() const override {
            return (bool)_value;
        }

        virtual int count() const override {
            return _count;
        }

        virtual void calculate() override {
            f_();
        }

        virtual int level() const override {
            return level_;
        }

        virtual void addChild(NodePtr p) override {
            children_.push_back(p);
        }

        virtual NodePtrList children() const override {
            NodePtrList nodes;
            std::transform(children_.begin(), children_.end(), std::back_inserter(nodes), [](auto p) { return p.lock();} );
            return nodes;
        }

        virtual NodePtrList parents() const override {
            NodePtrList nodes;
            std::transform(parents_.begin(), parents_.end(), std::back_inserter(nodes), [](auto p) { return p.lock();} );
            return nodes;
        }
};

template<typename F, typename ...V>
auto createValue(const F& f, ValuePtr<V> ... args) {
    using R = decltype(f(args->get()...));
    auto p = std::make_shared<Value<R>>(f, args...);
    for(auto parent: p->parents()) {
        parent->addChild(p);
    }
    return p;
}

template<template<typename, typename> typename C1, typename A1, template<typename, typename> typename C2, typename A2, typename R, typename V>
auto createValue(std::function<R(const C1<V, A1>&)> f, const C2<ValuePtr<V>, A2>& args) {
    auto p = std::make_shared<Value<R>>(f, args);
    for(auto parent: p->parents()) {
        parent->addChild(p);
    }
    return p;
}

NodePtrList descendents(NodePtrList nodes) {
    for(size_t i = 0; i < nodes.size(); i++) {
        const auto children = nodes[i]->children();
        nodes.insert(nodes.end(), children.begin(), children.end());
    }

    std::sort(nodes.begin(), nodes.end());
    auto last = std::unique(nodes.begin(), nodes.end());
    nodes.erase(last, nodes.end());
    std::sort(nodes.begin(), nodes.end(), [] (auto lhs, auto rhs) { return lhs->level() < rhs->level(); });


    return nodes;
}

std::vector<NodePtrList> levels(const NodePtrList& nodes) {
    std::vector<NodePtrList> levels;
    for(auto it1 = nodes.begin(), it = it1; it != nodes.end(); it++) {
        auto it2 = (it+1);
        if( it2 == nodes.end() || (*it2)->level() != (*it1)->level()) {
            levels.push_back(NodePtrList(it1, it2));
            it1 = it2;
        }
    }
    return levels;
}


void calculate(const NodePtrList& nodes) {
    for(const auto level : levels(descendents(nodes))) {
        for(const auto node : level) {
            node->calculate();
        }
    }
}

/*
void calculate_multithreaded(const NodePtrSet& nodes) {
    auto dirtyNodes = Node::descendents(nodes.begin(), nodes.end());
    for(const auto& level : _levels) {
        const auto& nodes = level.second;
        std::vector<std::future<void>> tasks;
        for(auto node : nodes) {
            bool isDirty = dirtyNodes.find(node) != dirtyNodes.end();
            if(isDirty) {
                tasks.push_back(std::async(_functions[node]));
            }
        }
        std::for_each(tasks.cbegin(), tasks.cend(),
            [](const auto& task) {
                task.wait();
            });
    }
}

template<typename V>
void calculate(const ValuePtrSet<V>& dirtyNodes) {
    calculate(NodePtrSet(dirtyNodes.begin(), dirtyNodes.end()));
}

void calculate(std::initializer_list<NodePtr> dirtyNodes) {
    calculate(NodePtrSet(dirtyNodes));
}

template<typename V>
void calculate(std::initializer_list<ValuePtr<V>> dirtyNodes) {
    calculate(NodePtrSet(dirtyNodes.begin(), dirtyNodes.end()));
}

*/
}
#endif // _\DATAFLOW_HPP_INCLUDED
