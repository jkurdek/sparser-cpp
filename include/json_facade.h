#ifndef JSON_FACADE_H_
#define JSON_FACADE_H_

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "rapidjson/document.h"

struct Predicate {
    std::string key;
    std::string value;
};

struct PredicateConjunction {
    std::vector<Predicate> predicates;
};

struct PredicateDisjunction {
    std::vector<PredicateConjunction> conjunctions;
};

class JsonQuery {
   private:
    PredicateDisjunction disjunction_;

   public:
    explicit JsonQuery(const PredicateDisjunction& disjunction) : disjunction_(disjunction) {}

    [[nodiscard]] const inline PredicateDisjunction& GetDisjunction() const { return disjunction_; }
    [[nodiscard]] std::string ToString() const;

    friend std::ostream& operator<<(std::ostream& os, const JsonQuery& query);
};

class JsonFacade {
   public:
    virtual ~JsonFacade() = default;
    virtual void Parse(std::string_view jsonStr) = 0;
    virtual std::optional<std::string_view> GetString(std::string_view key) const = 0;
};

class RapidJsonFacade : public JsonFacade {
   public:
    void Parse(std::string_view jsonStr) override;
    std::optional<std::string_view> GetString(std::string_view key) const override;

   private:
    rapidjson::Document doc_;
    std::unordered_map<std::string_view, std::string_view> key_value_map_;
};

class JsonQueryDriver {
   public:
    explicit JsonQueryDriver(std::unique_ptr<JsonFacade>&& json_facade = {}) : json_facade_(std::move(json_facade)) {}
    bool RunQuery(std::string_view buffer, const JsonQuery& query);

   private:
    std::unique_ptr<JsonFacade> json_facade_;
};

#endif  // JSON_FACADE_H_
