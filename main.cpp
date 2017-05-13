#include <iostream>
#include <functional>
#include <map>
#include <vector>
#include <exception>
#include <algorithm>

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

    private:
        template<typename F, typename R, typename... V> auto bind(const F& f, Value<R>* r, const Value<V>* ... v) {
            return Calculator([f, r, v...] () {r->set(f(v->get()...));});
        }

    public:
        template<typename F, typename ...V> auto add(const F& f, const Value<V>* ... v) {
            typedef Value<decltype(f(v->get()...))> R;
            auto* r = new R{};
            _functions[r] = bind(f, r, v...);
            _values[r] = NodeList(std::initializer_list<const Node*>{v...});
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

        void multithreaded() {

        }
};

int main()
{
    typedef std::pair<double, double> Quote;

    auto fquote = [](double mid, double spread) {
        return Quote{};
    };

    auto fwiden = [](const Quote& quote, double spread) { return Quote(quote.first-spread, quote.second+spread);};

    Value<double> mid = {1};
    Value<double> spread {0.1};

    Graph g;
    auto quote1 = g.add(fquote, &mid, &spread);
    auto quote2 = g.add(fwiden, quote1, &spread);

    g.recursive_bottomup();

    cout << quote1->get().first << "," << quote1->get().second << endl;
    cout << quote2->get().first << "," << quote2->get().second << endl;

    return 0;
}
