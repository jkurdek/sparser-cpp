#include "json_facade.h"

#include <simdjson.h>

#include <cstring>
#include <sstream>
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

bool SimdJsonFacade::EvaluateQuery(std::string_view jsonStr, const JsonQuery& query) {
    simdjson::ondemand::parser parser;
    auto json = simdjson::padded_string(jsonStr);

    try {
        auto doc = parser.iterate(json);
        auto obj = doc.get_object();

        bool ans = false;
        for (const auto& conjunction : query.GetDisjunction().conjunctions) {
            bool all_predicates_satisfied = true;

            for (const auto& predicate : conjunction.predicates) {
                simdjson::ondemand::value field;
                auto error = obj[predicate.key].get(field);

                if (error || !field.is_string()) {
                    all_predicates_satisfied = false;
                    break;
                }

                std::string_view field_str;
                error = field.get_string().get(field_str);
                if (error || field_str.find(predicate.value) == std::string_view::npos) {
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

    } catch (const simdjson::simdjson_error& e) {
#ifndef NDEBUG
        throw std::runtime_error(e.what());
#endif
        return false;
    }
}

bool JsonQueryDriver::RunQuery(std::string_view buffer, const JsonQuery& query) {
    return json_facade_->EvaluateQuery(buffer, query);
}
