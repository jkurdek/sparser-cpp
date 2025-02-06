#include <memory>
#include <span>

#include "json_facade.h"
#include "sparser.h"

int main(int argc, char* argv[]) {
    try {
        auto args = std::span(argv, argc);

        if (argc < 2) {
            std::cerr << "Usage: " << args[0] << " <filename>" << "\n";
            return 1;
        }

        const std::string filename = args[1];

        Predicate pred1{.key = "text", .value = "Lord of the Rings"};

        PredicateConjunction conj1{{pred1}};
        PredicateDisjunction disj{{conj1}};

        auto facade = std::make_unique<RapidJsonFacade>();
        auto json_query_driver = std::make_unique<JsonQueryDriver>(std::move(facade));
        auto sparser = Sparser(std::move(json_query_driver));
        sparser.Run(filename, JsonQuery(disj));

    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception caught" << "\n";
        return 1;
    }
    return 0;
}
