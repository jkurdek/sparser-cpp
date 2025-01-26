#include "json_facade.h"

#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "rapidjson/document.h"

std::string JsonQuery::ToString() const {
    std::ostringstream oss;
    for (const auto& conjunction : disjunction_.conjunctions) {
        if (!conjunction.predicates.empty()) {
            oss << "(";
        }
        for (const auto& predicate : conjunction.predicates) {
            oss << predicate.key << ": " << predicate.value;
            if (&predicate != &conjunction.predicates.back()) {
                oss << " ∧ ";
            }
        }
        if (!conjunction.predicates.empty()) {
            oss << ")";
        }
        if (&conjunction != &disjunction_.conjunctions.back()) {
            oss << " ∨ ";
        }
    }
    oss << "\n";
    return oss.str();
}

std::ostream& operator<<(std::ostream& os, const JsonQuery& query) {
    os << query.ToString();
    return os;
}

bool RapidJsonFacade::EvaluateQuery(std::string_view jsonStr, const JsonQuery& query) {
    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(jsonStr.data(), jsonStr.size());

    if (!ok || !doc.IsObject()) {
#ifndef NDEBUG
        throw std::runtime_error("Failed to parse JSON string");
#endif
        return false;
    }

    bool ans = false;

    for (const auto& conjunction : query.GetDisjunction().conjunctions) {
        bool all_predicates_satisfied = true;

        for (const auto& predicate : conjunction.predicates) {
            auto itr = doc.FindMember(predicate.key.c_str());

            if (itr == doc.MemberEnd()) {
#ifndef NDEBUG
                throw std::runtime_error("Key not found: " + predicate.key);
#endif
                return false;
            }

            if (!itr->value.IsString() || !strstr(itr->value.GetString(), predicate.value.c_str())) {
                all_predicates_satisfied = false;
                break;
            }
        }

        if (all_predicates_satisfied) {
            ans = true;
            break;
        }
    }

    return ans;
}

bool JsonQueryDriver::RunQuery(std::string_view buffer, const JsonQuery& query) {
    return json_facade_->EvaluateQuery(buffer, query);
}
