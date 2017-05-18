#include <iostream>
#include <experimental/optional>
#include <limits>
#include <cmath>
#include "dataflow.hpp"
using namespace std;


// nodes can hold a value of type either<optional<T>, std::exception>
// root arguments are given to us as optional<T>
//


const auto NaN = std::numeric_limits<double>::quiet_NaN();

int main()
{
    using Quote = std::pair<double, double>;

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

    auto fwidest = [](const std::vector<Quote>& quotesMap) {
                        Quote maxQuote = Quote{NaN, NaN};
                        for(const auto& quote : quotesMap) {
                            maxQuote.first = std::isnan(maxQuote.first) ? quote.first : std::min(maxQuote.first, quote.first);
                            maxQuote.second = std::isnan(maxQuote.second) ? quote.second : std::max(maxQuote.second, quote.second);
                        }
                        return maxQuote; };

    dataflow::graph g;
    auto mid = g.attach([]() {return 1.0;});
    auto spread = g.attach([]() {return 0.1;});
    auto shift = g.attach([]() {return 0.5;});


    std::map<std::string, dataflow::ValuePtr<Quote>> quotesMap;
    quotesMap["1"] = g.attach(fquote, mid, spread);
    quotesMap["2"] = g.attach(fwiden, quotesMap["1"], spread);
    quotesMap["3"] = g.attach(fwiden, quotesMap["2"], spread);
    quotesMap["4"] = g.attach(fshift, quotesMap["2"], shift);
    quotesMap["5"] = g.attach(fwiden, quotesMap["3"], spread);
    quotesMap["6"] = g.attach(fspan, quotesMap["5"], quotesMap["2"]);

    std::vector<dataflow::ValuePtr<Quote>> quotes;
    std::transform(quotesMap.begin(), quotesMap.end(), std::back_inserter(quotes), [](auto& kv) { return kv.second; });
    std::function<Quote(const std::vector<Quote>&)> f = fwidest;
    quotesMap["max"] = g.attach(f, quotes);

    g.calculate();

    quotesMap["2"]->clear();

    g.calculate();

    quotesMap["3"]->clear();

    g.calculate();

    for(const auto& quote: quotesMap) {
        cout << quote.second->count() << " - " << quote.first << ":" << quote.second->get().first << "," << quote.second->get().second << endl;
    }

    return 0;
}
