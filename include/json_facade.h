#ifndef JSON_FACADE_H_
#define JSON_FACADE_H_

#include <optional>
#include <string_view>
#include <unordered_map>

#include "rapidjson/document.h"
#include "sparser.h"

class JsonFacade {
   public:
    virtual ~JsonFacade() = default;
    virtual bool Parse(std::string_view jsonStr) = 0;
    virtual bool HasKey(std::string_view key) const = 0;
    virtual std::optional<std::string_view> GetString(std::string_view key) const = 0;
};

class RapidJsonFacade : public JsonFacade {
   public:
    bool Parse(std::string_view jsonStr) override;
    bool HasKey(std::string_view key) const override;
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
