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

// TODO: consider how data is consumed, and marked as changed - can we pass in a vector of nodes into calculate() that have changed rather than maintain state
// TODO: Add unit test framework
// TODO: simplify attach() overloads
// TODO: prune nodes
// TODO: assert all args are in the nodes of this graph, not another graph
// TODO: can we import/export nodes from/to other graphs?
// TODO: how would you express the graph in a declarative config file, how to move from the realm of type-unsafe config to type-safe c++


template<typename T>
using ptr = std::shared_ptr<T>;

class Node {

    public:
        virtual ~Node() {};
        virtual bool has_value() const = 0;
        virtual int count() const = 0;
};

using NodePtr = ptr<Node>;
using NodePtrList = std::vector<NodePtr>;
using NodePtrSet = std::unordered_set<NodePtr>;


template<typename T> class Value : public Node {

    std::experimental::optional<T> _value;
    std::experimental::optional<std::exception> _e;
    int _count = 0;

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

        virtual bool has_value() const {
            return (bool)_value;
        }

        virtual int count() const {
            return _count;
        }
};

template<typename T>
using ValuePtr = ptr<Value<T>>;

template<typename T>
using ValuePtrList = std::vector<ValuePtr<T>>;

template<typename T>
using ValuePtrSet = std::unordered_set<ValuePtr<T>>;


class graph {

    using Calculator = std::function<void()>;

    std::map<NodePtr, Calculator> _functions;
    std::map<NodePtr, NodePtrList> _children;
    std::map<NodePtr, int> _nodeLevels;
    std::map<int, NodePtrList> _levels;

    private:
        template<typename F, typename R, typename... V>
        auto bind(const F& f, ValuePtr<R> r, ValuePtr<V> ... v) {
            return Calculator(
                [f, r, v...] () {
                    r->set(f(v->get()...));
                    return r;
                });
        }

        template<template<typename, typename> typename C1, typename A1, template<typename, typename> typename C2, typename A2, typename R, typename V>
        auto bind(std::function<R(const C1<V, A1>&)> f,
                  ValuePtr<R> r,
                  const C2<ValuePtr<V>, A2>& args) {
            return [f, r, args](){
                C1<V, A1> vs;
                std::transform(args.begin(), args.end(), std::back_inserter(vs), [](auto v) {return v->get();});
                r->set(f(vs));
                return r;
            };
        }

        void update(std::function<void()> f, NodePtr r, const NodePtrList& args) {
            _functions[r] = f;

            std::for_each(args.begin(), args.end(), [this, r] (auto arg) {_children[arg].push_back(r);});

            std::vector<int> levels;
            std::transform(args.begin(), args.end(), std::back_inserter(levels), [this](auto arg){ return _nodeLevels.find(arg)->second;});
            auto maxLevel = std::max_element(levels.cbegin(), levels.cend());
            auto level = maxLevel == levels.end() ? 0 : *maxLevel + 1;
            _nodeLevels[r] = level;
            _levels[level].push_back(r);
        }

        template<typename Iter>
        NodePtrSet descendents(Iter i1, Iter i2) {
            NodePtrList nodes(i1, i2);
            for(size_t i = 0; i < nodes.size(); i++) {
                const NodePtrList& children = _children[nodes[i]];
                nodes.insert(nodes.end(), children.begin(), children.end());
            }

            return NodePtrSet(nodes.begin(), nodes.end());
        }

    public:

        template<typename F, typename ...V> auto attach(const F& f, ValuePtr<V> ... args) {
            using R = decltype(f(args->get()...));
            auto r = std::make_shared<Value<R>>();
            auto g = bind(f, r, args...);
            update(g, r, NodePtrList{std::initializer_list<NodePtr>{args...}});
            return r;
        }

        template<template<typename, typename> typename C1, typename A1, template<typename, typename> typename C2, typename A2, typename R, typename V>
        auto attach(std::function<R(const C1<V, A1>&)> f, const C2<ValuePtr<V>, A2>& args) {
            auto r = std::make_shared<Value<R>>();
            auto g = bind(f, r, args);
            update(g, r, NodePtrList(args.begin(), args.end()));
            return r;
        }

        void calculate(const NodePtrSet& nodes) {
            auto dirtyNodes = descendents(nodes.begin(), nodes.end());
            for(const auto& level : _levels) {
                const auto& nodes = level.second;

                for(const auto node : nodes) {
                    bool isDirty = dirtyNodes.find(node) != dirtyNodes.end();
                    if(isDirty) {
                        _functions[node]();
                    };
                }
            }
        }

        void calculate_multithreaded(const NodePtrSet& nodes) {
            auto dirtyNodes = descendents(nodes.begin(), nodes.end());
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

    };

}

#endif // _\DATAFLOW_HPP_INCLUDED
