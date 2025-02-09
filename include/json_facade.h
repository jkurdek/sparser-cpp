#ifndef JSON_FACADE_H_
#define JSON_FACADE_H_

#include <memory>
#include <string>
#include <string_view>
#include <vector>

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
    [[nodiscard]] virtual bool EvaluateQuery(std::string_view jsonStr, const JsonQuery& query) = 0;
};

class RapidJsonFacade : public JsonFacade {
   public:
    [[nodiscard]] virtual bool EvaluateQuery(std::string_view jsonStr, const JsonQuery& query) override;
};

class SimdJsonFacade : public JsonFacade {
   public:
    [[nodiscard]] virtual bool EvaluateQuery(std::string_view jsonStr, const JsonQuery& query) override;
};

class JsonQueryDriver {
   public:
    explicit JsonQueryDriver(std::unique_ptr<JsonFacade>&& json_facade = {}) : json_facade_(std::move(json_facade)) {}
    [[nodiscard]] bool RunQuery(std::string_view buffer, const JsonQuery& query);

   private:
    std::unique_ptr<JsonFacade> json_facade_;
};

#endif  // JSON_FACADE_H_
