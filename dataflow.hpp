#ifndef DATAFLOW_HPP_INCLUDED
#define DATAFLOW_HPP_INCLUDED

#include <functional>
#include <map>
#include <vector>
#include <exception>
#include <algorithm>
#include <utility>
#include <future>
#include <experimental/optional>
#include <memory>

namespace dataflow {

// TODO: consider how data is consumed, and marked as changed
// TODO: consider how data is collated e.g. aggregate all prices into a single map
// TODO: Add unit test framework

class Node {

    public:
        virtual ~Node() {};

        virtual void clear() = 0;
        virtual bool has_value() const = 0;
};

template<typename T> class Value : public Node {

    std::experimental::optional<T> _value;
    std::experimental::optional<std::exception> _e;

    public:

        void set(const std::exception& e) {
            _e = e;
            _value = {};
        }

        void set1(const T& value) {
            _e = {};
            _value = value;
        }

        T get() const {
            if(_e) {
                throw _e;
            }

            return _value.value();
        }

        virtual void clear() {
            _e = {};
            _value = {};
        }

        virtual bool has_value() const {
            return (bool)_value;
        }
};

class graph {

    typedef std::function<void()> Calculator;

    std::map<std::shared_ptr<Node>, Calculator> _functions;

    typedef std::vector<std::shared_ptr<Node>> NodeList;
    std::map<std::shared_ptr<Node>, NodeList> _values;

    std::map<std::shared_ptr<Node>, int> _nodeLevels;
    std::map<int, NodeList> _levels;

    private:
        template<typename F, typename R, typename... V> auto bind(const F& f, std::shared_ptr<Value<R>> r, std::shared_ptr<Value<V>> ... v) {
            return Calculator([f, r, v...] () {r->set1(f(v->get()...));});
        }

    public:
        template<typename F, typename ...V> auto attach(const F& f, std::shared_ptr<Value<V>> ... v) {
            typedef decltype(f(v->get()...)) R;
            auto r = std::make_shared<Value<R>>();
            _functions[r] = bind(f, r, v...);
            auto rn = std::static_pointer_cast<Node>(r);
            const auto& args = _values[rn] = std::initializer_list<std::shared_ptr<Node>>{std::static_pointer_cast<Node>(v)...};
            // TODO assert all args are in the nodes of this graph

            std::vector<int> levels;
            std::transform(args.cbegin(), args.cend(), std::back_inserter(levels), [this](auto arg){ return _nodeLevels.find(arg)->second;});
            auto maxLevel = std::max_element(levels.cbegin(), levels.cend());
            auto level = maxLevel == levels.end() ? 0 : *maxLevel + 1;
            _nodeLevels[r] = level;
            _levels[level].push_back(r);
            return r;
        }

        void calculate() {
            for(const auto& level : _levels) {
                const auto& nodes = level.second;
                std::for_each(nodes.cbegin(), nodes.cend(),
                    [this](const auto& node) {
                        _functions[node]();
                    });
            }
        }

        void calculate_multithreaded() {
            for(const auto& level : _levels) {
                const auto& nodes = level.second;
                std::vector<std::future<void>> tasks;
                std::transform(nodes.cbegin(), nodes.cend(), std::back_inserter(tasks),
                    [this](const auto& node) {
                        return std::async(_functions[node]);
                    });
                std::for_each(tasks.cbegin(), tasks.cend(),
                    [](const auto& task) {
                        task.wait();
                    });
            }
        }
    };

}

#endif // _\DATAFLOW_HPP_INCLUDED
