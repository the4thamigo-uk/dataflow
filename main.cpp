#include <iostream>
#include <functional>
#include <map>
#include <vector>
#include <exception>
#include <algorithm>

using namespace std;

class BaseValue {

    public:
        virtual ~BaseValue() {};

        virtual void clear() = 0;
        virtual bool isSet() const = 0;
};

template<typename T> class Value :
    public BaseValue {

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

    std::map<BaseValue*, std::function<void()>> _functions;
    std::map<BaseValue*, std::vector<const BaseValue*>> _values;

    public:
        template<typename F, typename R, typename... V> auto bind(const F& f, Value<R>* r, const Value<V>* ... v) {
            return std::function<void()>([f, r, v...] () {r->set(f(v->get()...));});
        }
        template<typename F, typename R, typename ...V> void add(const F& f, Value<R>* r, const Value<V>* ... v) {
            _functions[r] = bind(f, r, v...);
            _values[r] = std::vector<const BaseValue*>(std::initializer_list<const BaseValue*>{v...});
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
        void calculate() {
            while(pass() > 0) {
                // repeat
            }
        }
};





// functions are always pure
// the same function can appear many times in the same graph
// each value can appear only once in the same graph
// a value is bound to a function return
// a function is bound to a set of argument values

// user defines a set of functions and calls dataflow.bind(f, v...) => w
// bind asserts the values are valid, and computes the maximum depth of these values
// bind attaches a new value w to the the f and adds it to the list of values, sets the depth + 1
// bind stores the set of values against the function
// w is placed in an 'unevaluated' queue U ordered by their depth in the tree
// we look through U to see if the bound function can be evaluated (i.e. its argument values are evaluated)
// if so it is evaluated and value is cached, and removed from the list

int main()
{
    // one ay using explicit value objects.
    auto fquote = [](double mid, double spread) {
        return std::make_pair(mid-spread, mid+spread);
        };
    auto fwiden = [](const std::pair<double, double>& quote, double spread) { return std::make_pair(quote.first-spread, quote.second+spread);};

    Value<double> spread {0.1};
    Value<double> mid = {1};
    Value<std::pair<double, double>> quote1;
    Value<std::pair<double, double>> quote2;

    Graph g;
    g.add(fquote, &quote1, &mid, &spread);
    g.add(fwiden, &quote2, &quote1, &spread);

    g.calculate();

    cout << quote1.get().first << "," << quote1.get().second << endl;
    cout << quote2.get().first << "," << quote2.get().second << endl;

    return 0;
}
