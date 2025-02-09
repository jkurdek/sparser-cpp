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
    static thread_local rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(jsonStr.data(), jsonStr.size());

    if (!ok || !doc.IsObject()) {
#ifndef NDEBUG
        throw std::runtime_error("Failed to parse JSON string");
#endif
        return false;
    }

    for (const auto& conjunction : query.GetDisjunction().conjunctions) {
        bool all_predicates_satisfied = true;
        for (const auto& predicate : conjunction.predicates) {
            if (!doc.HasMember(predicate.key.c_str())) {
#ifndef NDEBUG
                throw std::runtime_error("Key not found: " + predicate.key);
#endif
                all_predicates_satisfied = false;
                break;
            }
            const rapidjson::Value& value = doc[predicate.key.c_str()];
            if (!value.IsString() ||
                std::string_view(value.GetString(), value.GetStringLength()).find(predicate.value) ==
                    std::string_view::npos) {
                all_predicates_satisfied = false;
                break;
            }
        }
        if (all_predicates_satisfied) {
            return true;
        }
    }
    return false;
}

bool SimdJsonFacade::EvaluateQuery(std::string_view jsonStr, const JsonQuery& query) {
    static thread_local simdjson::dom::parser parser;
    
    try {
        simdjson::dom::element doc = parser.parse(jsonStr.data(), jsonStr.size());
        
        for (const auto& conjunction : query.GetDisjunction().conjunctions) {
            bool all_predicates_satisfied = true;
            
            for (const auto& predicate : conjunction.predicates) {
                simdjson::dom::element field;
                auto error = doc[predicate.key].get(field);
                
                if (error || !field.is_string()) {
                    all_predicates_satisfied = false;
                    break;
                }
                
                std::string_view field_str = field.get_string().value();
                if (field_str.find(predicate.value) == std::string_view::npos) {
                    all_predicates_satisfied = false;
                    break;
                }
            }
            
            if (all_predicates_satisfied) {
                return true;
            }
        }
        return false;
    } catch (const simdjson::simdjson_error&) {
        return false;
    }
}

bool JsonQueryDriver::RunQuery(std::string_view buffer, const JsonQuery& query) {
    return json_facade_->EvaluateQuery(buffer, query);
}
