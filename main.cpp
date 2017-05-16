#include <iostream>
#include <functional>
#include <map>
#include <vector>
#include <exception>
#include <algorithm>
#include <utility>
#include <future>

using namespace std;

class Node {

    public:
        virtual ~Node() {};

        virtual void clear() = 0;
        virtual bool isSet() const = 0;
};

template<typename T> class Value :
    public Node {

    T _value = {};
    bool _isSet = false;

    public:
        Value() {

        }

        Value ( Value&& ) = delete;
        Value ( const Value& ) = delete;

        Value(const T& value) {
            _value = value;
            _isSet = true;
        }

        T get() const {
            if(!_isSet) {
                throw std::logic_error("not set");
            }

            return _value;
        }

        void set(const T& value) {
            _value = value;
            _isSet = true;
        }

        virtual void clear() {
            _isSet = false;
        }

        virtual bool isSet() const {
            return _isSet;
        }
};

class Graph {

    typedef std::function<void()> Calculator;
    std::map<const Node*, Calculator> _functions;

    typedef std::vector<const Node*> NodeList;
    std::map<const Node*, NodeList> _values;

    std::map<const Node*, int> _nodeLevels;
    std::map<int, NodeList> _levels;

    private:
        template<typename F, typename R, typename... V> auto bind(const F& f, Value<R>* r, const Value<V>* ... v) {
            return Calculator([f, r, v...] () {r->set(f(v->get()...));});
        }

    public:
        template<typename F, typename ...V> auto add(const F& f, const Value<V>* ... v) {
            typedef Value<decltype(f(v->get()...))> R;
            auto* r = new R{};
            _functions[r] = bind(f, r, v...);
            const auto& args = _values[r] = std::initializer_list<const Node*>{v...};

            // TODO assert all args are in the nodes of this graph

            std::vector<int> levels;
            std::transform(args.cbegin(), args.cend(), std::back_inserter(levels), [this](auto arg){ return _nodeLevels.find(arg)->second;});
            auto maxLevel = std::max_element(levels.cbegin(), levels.cend());
            auto level = maxLevel == levels.end() ? 0 : *maxLevel + 1;
            _nodeLevels[r] = level;
            _levels[level].push_back(r);
            return r;
        }

        size_t pass() {
            size_t remaining = 0;
            for(auto& kv: _functions) {
                const auto& result = kv.first;
                const auto& f = kv.second;
                const auto& args = _values[result];

                if(!result->isSet()) {
                    if(std::all_of(args.cbegin(), args.cend(), [](auto arg) {return arg->isSet();})){
                        f();
                    } else {
                        remaining++;
                    }
                }
            }

            // all values are calculated
            return remaining;
        }

        void repeated() {
            while(pass() > 0) {
                // repeat
            }
        }

        void recursive_bottomup() {

            std::function<void(const Node*)> calculate;
            calculate = [this, &calculate] (const Node* value) {
                if(value->isSet())
                    return;

                const auto& f = _functions[value];
                const auto& args = _values[value];
                std::for_each(args.cbegin(), args.cend(), calculate);
                f();
            };

            for(const auto& kv: _functions) {
                auto value = kv.first;
                calculate(value);
            }
        }

        void recursive_topdown() {

        }

        void stack_bottomup() {
        }

        void stack_topdown() {
        }

        void by_level() {
            for(const auto& level : _levels) {
                const auto& nodes = level.second;
                std::for_each(nodes.cbegin(), nodes.cend(),
                    [this](const auto& node) {
                        _functions[node]();
                    });
            }
        }

        void multithreaded_level_synchronous() {
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

int main()
{
    typedef std::pair<double, double> Quote;

    auto fquote = [](double mid, double spread) {
        return Quote{mid - spread, mid+spread};
    };

    auto fwiden = [](const Quote& quote, double spread) {
        return Quote{quote.first-spread, quote.second+spread};
    };

    auto fspan = [](const Quote& quote1, const Quote& quote2) {
        return Quote{std::min(quote1.first, quote2.first), std::max(quote1.second, quote2.second)};
    };

    auto fshift = [](const Quote& quote, double shift) {
        return Quote{quote.first+shift, quote.second+shift};
    };

    Graph g;
    auto mid = g.add([]() {return 1.0;});
    auto spread = g.add([]() {return 0.1;});
    auto shift = g.add([]() {return 0.5;});

    std::map<std::string, Value<Quote>*> quotes;
    quotes["1"] = g.add(fquote, mid, spread);
    quotes["2"] = g.add(fwiden, quotes["1"], spread);
    quotes["3"] = g.add(fwiden, quotes["2"], spread);
    quotes["4"] = g.add(fshift, quotes["2"], shift);
    quotes["5"] = g.add(fspan, quotes["4"], quotes["1"]);

    g.multithreaded_level_synchronous();

    for(const auto& quote: quotes) {
        cout << quote.first << ":" << quote.second->get().first << "," << quote.second->get().second << endl;
    }

    return 0;
}
