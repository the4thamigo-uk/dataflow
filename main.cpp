#include <iostream>
#include <experimental/optional>
#include "dataflow.hpp"
using namespace std;


// nodes can hold a value of type either<optional<T>, std::exception>
// root arguments are given to us as optional<T>
//



/*

    try {
        return Value(f());
    } catch(const std::exception& e) {
        return Value(std::optional<T>(), e);
    }


*/

struct X {
    X() = delete;
};

typedef std::experimental::optional<X> x;


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

    dataflow::graph g;
    auto mid = g.attach([]() {return 1.0;});
    auto spread = g.attach([]() {return 0.1;});
    auto shift = g.attach([]() {return 0.5;});

    std::map<std::string, std::shared_ptr<dataflow::Value<Quote>>> quotes;
    quotes["1"] = g.attach(fquote, mid, spread);
    quotes["2"] = g.attach(fwiden, quotes["1"], spread);
    quotes["3"] = g.attach(fwiden, quotes["2"], spread);
    quotes["4"] = g.attach(fshift, quotes["2"], shift);
    quotes["5"] = g.attach(fspan, quotes["4"], quotes["1"]);

    g.calculate_multithreaded();

    for(const auto& quote: quotes) {
        cout << quote.first << ":" << quote.second->get().first << "," << quote.second->get().second << endl;
    }

    return 0;
}
