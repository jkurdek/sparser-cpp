#include "json_facade.h"

#include <optional>
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

void RapidJsonFacade::Parse(std::string_view jsonStr) {
    rapidjson::ParseResult ok = doc_.Parse(jsonStr.data(), jsonStr.size());
    if (!ok || !doc_.IsObject()) {
        throw std::runtime_error("Failed to parse JSON string");
    }
    key_value_map_.clear();

    for (auto it = doc_.MemberBegin(); it != doc_.MemberEnd(); ++it) {
        const char* key = it->name.GetString();
        if (it->value.IsString()) {
            key_value_map_[key] = it->value.GetString();
        }
    }
}

std::optional<std::string_view> RapidJsonFacade::GetString(std::string_view key) const {
    auto it = key_value_map_.find(key);
    if (it == key_value_map_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool JsonQueryDriver::RunQuery(std::string_view buffer, const JsonQuery& query) {
    json_facade_->Parse(buffer);

    for (const auto& conjunction : query.GetDisjunction().conjunctions) {
        bool all_predicates_satisfied = true;

        for (const auto& predicate : conjunction.predicates) {
            auto value = json_facade_->GetString(predicate.key);
            if (!value.has_value() || !value.value().contains(predicate.value)) {
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
