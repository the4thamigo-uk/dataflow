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

    auto mid = dataflow::createValue([]() {return 1.0;});
    auto spread = dataflow::createValue([]() {return 0.1;});
    auto shift = dataflow::createValue([]() {return 0.5;});

    std::map<std::string, dataflow::ValuePtr<Quote>> quotesMap;
    quotesMap["1"] = dataflow::createValue(fquote, mid, spread);
    quotesMap["2"] = dataflow::createValue(fwiden, quotesMap["1"], spread);
    quotesMap["3"] = dataflow::createValue(fwiden, quotesMap["2"], spread);
    quotesMap["4"] = dataflow::createValue(fshift, quotesMap["2"], shift);
    quotesMap["5"] = dataflow::createValue(fwiden, quotesMap["3"], spread);
    quotesMap["6"] = dataflow::createValue(fspan, quotesMap["5"], quotesMap["2"]);

    std::vector<dataflow::ValuePtr<Quote>> quotes;
    std::transform(quotesMap.begin(), quotesMap.end(), std::back_inserter(quotes), [](auto& kv) { return kv.second; });
    std::function<Quote(const std::vector<Quote>&)> f = fwidest;
    quotesMap["max"] = dataflow::createValue(f, quotes);

    dataflow::calculate({mid, spread, shift});
    dataflow::calculate({quotesMap["2"]});
    dataflow::calculate({quotesMap["3"]});
    dataflow::calculate({quotesMap["5"]});
    dataflow::calculate({quotesMap["6"]});


    std::cout << std::endl;
    for(const auto& quote: quotesMap) {
        cout << quote.second->count() << " - " << quote.first << ":" <<
            (quote.second->has_value() ? quote.second->get().first : -1.0) <<
            "," <<
            (quote.second->has_value() ? quote.second->get().second : -1.0) << endl;
    }

    return 0;
}
