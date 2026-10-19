#pragma once

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <charconv>
#include <variant>
#include <memory>
#include <stdexcept>

namespace Visualizer::Json {

enum class Type {
    Null,
    Boolean,
    Number,
    String,
    Array,
    Object
};

class Value {
public:
    using Array = std::vector<Value>;
    using Object = std::map<std::string, Value>;

    Value() : m_type(Type::Null), m_data(nullptr) {}
    Value(std::nullptr_t) : m_type(Type::Null), m_data(nullptr) {}
    Value(bool b) : m_type(Type::Boolean), m_data(b) {}
    Value(int i) : m_type(Type::Number), m_data(static_cast<double>(i)) {}
    Value(float f) : m_type(Type::Number), m_data(static_cast<double>(f)) {}
    Value(double d) : m_type(Type::Number), m_data(d) {}
    Value(const char* s) : m_type(Type::String), m_data(std::string(s)) {}
    Value(std::string s) : m_type(Type::String), m_data(std::move(s)) {}
    Value(Array arr) : m_type(Type::Array), m_data(std::move(arr)) {}
    Value(Object obj) : m_type(Type::Object), m_data(std::move(obj)) {}

    Type type() const { return m_type; }
    bool is_null() const { return m_type == Type::Null; }
    bool is_boolean() const { return m_type == Type::Boolean; }
    bool is_number() const { return m_type == Type::Number; }
    bool is_string() const { return m_type == Type::String; }
    bool is_array() const { return m_type == Type::Array; }
    bool is_object() const { return m_type == Type::Object; }

    bool as_bool(bool def = false) const {
        if (auto* val = std::get_if<bool>(&m_data)) return *val;
        return def;
    }

    double as_double(double def = 0.0) const {
        if (auto* val = std::get_if<double>(&m_data)) return *val;
        return def;
    }

    float as_float(float def = 0.0f) const {
        return static_cast<float>(as_double(static_cast<double>(def)));
    }

    int as_int(int def = 0) const {
        return static_cast<int>(as_double(static_cast<double>(def)));
    }

    const std::string& as_string(const std::string& def = "") const {
        if (auto* val = std::get_if<std::string>(&m_data)) return *val;
        static std::string s_empty;
        s_empty = def;
        return s_empty;
    }

    const Array& as_array() const {
        if (auto* val = std::get_if<Array>(&m_data)) return *val;
        static Array s_empty;
        return s_empty;
    }

    Array& as_array_mut() {
        if (!std::holds_alternative<Array>(m_data)) {
            m_type = Type::Array;
            m_data = Array();
        }
        return std::get<Array>(m_data);
    }

    const Object& as_object() const {
        if (auto* val = std::get_if<Object>(&m_data)) return *val;
        static Object s_empty;
        return s_empty;
    }

    Object& as_object_mut() {
        if (!std::holds_alternative<Object>(m_data)) {
            m_type = Type::Object;
            m_data = Object();
        }
        return std::get<Object>(m_data);
    }

    bool contains(const std::string& key) const {
        if (auto* obj = std::get_if<Object>(&m_data)) {
            return obj->find(key) != obj->end();
        }
        return false;
    }

    Value& operator[](const std::string& key) {
        return as_object_mut()[key];
    }

    const Value& operator[](const std::string& key) const {
        if (auto* obj = std::get_if<Object>(&m_data)) {
            auto it = obj->find(key);
            if (it != obj->end()) return it->second;
        }
        static Value s_null;
        return s_null;
    }

    std::string dump(int indent = 2, int currentIndent = 0) const {
        std::string ind(currentIndent, ' ');
        std::string nextInd(currentIndent + indent, ' ');

        switch (m_type) {
        case Type::Null: return "null";
        case Type::Boolean: return as_bool() ? "true" : "false";
        case Type::Number: {
            std::ostringstream ss;
            ss << std::setprecision(6) << as_double();
            return ss.str();
        }
        case Type::String: {
            std::string res = "\"";
            for (char c : as_string()) {
                if (c == '"') res += "\\\"";
                else if (c == '\\') res += "\\\\";
                else if (c == '\n') res += "\\n";
                else if (c == '\r') res += "\\r";
                else if (c == '\t') res += "\\t";
                else res += c;
            }
            res += "\"";
            return res;
        }
        case Type::Array: {
            const auto& arr = as_array();
            if (arr.empty()) return "[]";
            std::string res = "[\n";
            for (size_t i = 0; i < arr.size(); ++i) {
                res += nextInd + arr[i].dump(indent, currentIndent + indent);
                if (i + 1 < arr.size()) res += ",";
                res += "\n";
            }
            res += ind + "]";
            return res;
        }
        case Type::Object: {
            const auto& obj = as_object();
            if (obj.empty()) return "{}";
            std::string res = "{\n";
            size_t count = 0;
            for (const auto& [k, v] : obj) {
                res += nextInd + "\"" + k + "\": " + v.dump(indent, currentIndent + indent);
                if (++count < obj.size()) res += ",";
                res += "\n";
            }
            res += ind + "}";
            return res;
        }
        }
        return "null";
    }

    static Value parse(const std::string& src) {
        size_t idx = 0;
        return parseValue(src, idx);
    }

private:
    Type m_type;
    std::variant<std::nullptr_t, bool, double, std::string, Array, Object> m_data;

    static void skipWhitespace(const std::string& src, size_t& idx) {
        while (idx < src.size() && (src[idx] == ' ' || src[idx] == '\t' || src[idx] == '\n' || src[idx] == '\r')) {
            idx++;
        }
    }

    static Value parseValue(const std::string& src, size_t& idx) {
        skipWhitespace(src, idx);
        if (idx >= src.size()) return Value();

        char c = src[idx];
        if (c == 'n') { idx += 4; return Value(); }
        if (c == 't') { idx += 4; return Value(true); }
        if (c == 'f') { idx += 5; return Value(false); }
        if (c == '"') return parseString(src, idx);
        if (c == '[') return parseArray(src, idx);
        if (c == '{') return parseObject(src, idx);
        if (c == '-' || (c >= '0' && c <= '9')) return parseNumber(src, idx);
        return Value();
    }

    static Value parseString(const std::string& src, size_t& idx) {
        idx++; // skip open quote
        std::string res;
        while (idx < src.size()) {
            char c = src[idx++];
            if (c == '"') break;
            if (c == '\\' && idx < src.size()) {
                char esc = src[idx++];
                if (esc == 'n') res += '\n';
                else if (esc == 'r') res += '\r';
                else if (esc == 't') res += '\t';
                else if (esc == '"') res += '"';
                else if (esc == '\\') res += '\\';
                else res += esc;
            } else {
                res += c;
            }
        }
        return Value(res);
    }

    static Value parseNumber(const std::string& src, size_t& idx) {
        size_t start = idx;
        if (src[idx] == '-') idx++;
        while (idx < src.size() && ((src[idx] >= '0' && src[idx] <= '9') || src[idx] == '.' || src[idx] == 'e' || src[idx] == 'E' || src[idx] == '+' || src[idx] == '-')) {
            idx++;
        }
        std::string numStr = src.substr(start, idx - start);
        return Value(std::stod(numStr));
    }

    static Value parseArray(const std::string& src, size_t& idx) {
        idx++; // skip '['
        Array arr;
        while (idx < src.size()) {
            skipWhitespace(src, idx);
            if (idx < src.size() && src[idx] == ']') { idx++; break; }
            arr.push_back(parseValue(src, idx));
            skipWhitespace(src, idx);
            if (idx < src.size() && src[idx] == ',') idx++;
        }
        return Value(arr);
    }

    static Value parseObject(const std::string& src, size_t& idx) {
        idx++; // skip '{'
        Object obj;
        while (idx < src.size()) {
            skipWhitespace(src, idx);
            if (idx < src.size() && src[idx] == '}') { idx++; break; }
            if (src[idx] == '"') {
                std::string key = parseString(src, idx).as_string();
                skipWhitespace(src, idx);
                if (idx < src.size() && src[idx] == ':') idx++;
                obj[key] = parseValue(src, idx);
            }
            skipWhitespace(src, idx);
            if (idx < src.size() && src[idx] == ',') idx++;
        }
        return Value(obj);
    }
};

} // namespace Visualizer::Json
