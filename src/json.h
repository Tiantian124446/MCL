// ---------------------------------------------------------------------------
//  json.h  -  极简 JSON 解析器 (header-only, 无第三方依赖)
//  支持: object / array / string / number / bool / null, UTF-8 与 \uXXXX 转义
// ---------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>
#include <utility>
#include <cstdlib>
#include <cstring>

namespace mjson {

class Value {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };

    Type type = Type::Null;
    bool bval = false;
    double nval = 0.0;
    std::string sval;
    std::vector<Value> arr;
    std::vector<std::pair<std::string, Value>> obj;

    bool isNull()   const { return type == Type::Null; }
    bool isBool()   const { return type == Type::Bool; }
    bool isNumber() const { return type == Type::Number; }
    bool isString() const { return type == Type::String; }
    bool isArray()  const { return type == Type::Array; }
    bool isObject() const { return type == Type::Object; }

    size_t size() const {
        if (type == Type::Array)  return arr.size();
        if (type == Type::Object) return obj.size();
        return 0;
    }

    bool has(const std::string& k) const {
        if (type != Type::Object) return false;
        for (size_t i = 0; i < obj.size(); ++i)
            if (obj[i].first == k) return true;
        return false;
    }

    const Value& get(const std::string& k) const {
        static const Value nullV;
        if (type == Type::Object) {
            for (size_t i = 0; i < obj.size(); ++i)
                if (obj[i].first == k) return obj[i].second;
        }
        return nullV;
    }
    const Value& operator[](const std::string& k) const { return get(k); }

    const Value& at(size_t i) const {
        static const Value nullV;
        if (type == Type::Array && i < arr.size()) return arr[i];
        return nullV;
    }
    const Value& operator[](size_t i) const { return at(i); }

    std::string asString(const std::string& def = "") const {
        return (type == Type::String) ? sval : def;
    }
    double asNumber(double def = 0.0) const {
        return (type == Type::Number) ? nval : def;
    }
    int asInt(int def = 0) const { return (int)asNumber((double)def); }
    bool asBool(bool def = false) const {
        return (type == Type::Bool) ? bval : def;
    }
};

namespace detail {

class Parser {
public:
    const char* p;
    const char* end;
    bool ok;

    Parser() : p(nullptr), end(nullptr), ok(true) {}

    void skipWs() {
        while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) ++p;
    }

    static void utf8Encode(unsigned int cp, std::string& out) {
        if (cp <= 0x7F) {
            out += (char)cp;
        } else if (cp <= 0x7FF) {
            out += (char)(0xC0 | (cp >> 6));
            out += (char)(0x80 | (cp & 0x3F));
        } else if (cp <= 0xFFFF) {
            out += (char)(0xE0 | (cp >> 12));
            out += (char)(0x80 | ((cp >> 6) & 0x3F));
            out += (char)(0x80 | (cp & 0x3F));
        } else {
            out += (char)(0xF0 | (cp >> 18));
            out += (char)(0x80 | ((cp >> 12) & 0x3F));
            out += (char)(0x80 | ((cp >> 6) & 0x3F));
            out += (char)(0x80 | (cp & 0x3F));
        }
    }

    unsigned int hex4() {
        unsigned int v = 0;
        for (int i = 0; i < 4; ++i) {
            if (p >= end) { ok = false; return v; }
            char c = *p++;
            v <<= 4;
            if (c >= '0' && c <= '9')      v |= (unsigned int)(c - '0');
            else if (c >= 'a' && c <= 'f') v |= (unsigned int)(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') v |= (unsigned int)(c - 'A' + 10);
            else { ok = false; return v; }
        }
        return v;
    }

    std::string parseString() {
        std::string out;
        if (p >= end || *p != '"') { ok = false; return out; }
        ++p;
        while (p < end) {
            char c = *p++;
            if (c == '"') return out;
            if (c == '\\') {
                if (p >= end) break;
                char e = *p++;
                switch (e) {
                    case '"':  out += '"';  break;
                    case '\\': out += '\\'; break;
                    case '/':  out += '/';  break;
                    case 'b':  out += '\b'; break;
                    case 'f':  out += '\f'; break;
                    case 'n':  out += '\n'; break;
                    case 'r':  out += '\r'; break;
                    case 't':  out += '\t'; break;
                    case 'u': {
                        unsigned int cp = hex4();
                        if (cp >= 0xD800 && cp <= 0xDBFF && (end - p) >= 6 &&
                            p[0] == '\\' && p[1] == 'u') {
                            p += 2;
                            unsigned int lo = hex4();
                            if (lo >= 0xDC00 && lo <= 0xDFFF)
                                cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                        }
                        utf8Encode(cp, out);
                        break;
                    }
                    default: out += e; break;
                }
            } else {
                out += c;
            }
        }
        ok = false;
        return out;
    }

    Value parseValue() {
        skipWs();
        Value v;
        if (p >= end) { ok = false; return v; }
        char c = *p;

        if (c == '{') {
            ++p;
            v.type = Value::Type::Object;
            skipWs();
            if (p < end && *p == '}') { ++p; return v; }
            while (p < end) {
                skipWs();
                if (p >= end || *p != '"') { ok = false; return v; }
                std::string key = parseString();
                if (!ok) return v;
                skipWs();
                if (p >= end || *p != ':') { ok = false; return v; }
                ++p;
                Value val = parseValue();
                if (!ok) return v;
                v.obj.push_back(std::make_pair(key, val));
                skipWs();
                if (p < end && *p == ',') { ++p; continue; }
                if (p < end && *p == '}') { ++p; return v; }
                ok = false; return v;
            }
            ok = false; return v;
        }

        if (c == '[') {
            ++p;
            v.type = Value::Type::Array;
            skipWs();
            if (p < end && *p == ']') { ++p; return v; }
            while (p < end) {
                Value val = parseValue();
                if (!ok) return v;
                v.arr.push_back(val);
                skipWs();
                if (p < end && *p == ',') { ++p; continue; }
                if (p < end && *p == ']') { ++p; return v; }
                ok = false; return v;
            }
            ok = false; return v;
        }

        if (c == '"') {
            v.type = Value::Type::String;
            v.sval = parseString();
            return v;
        }

        if (c == 't') {
            if ((end - p) >= 4 && std::strncmp(p, "true", 4) == 0) {
                p += 4; v.type = Value::Type::Bool; v.bval = true; return v;
            }
            ok = false; return v;
        }
        if (c == 'f') {
            if ((end - p) >= 5 && std::strncmp(p, "false", 5) == 0) {
                p += 5; v.type = Value::Type::Bool; v.bval = false; return v;
            }
            ok = false; return v;
        }
        if (c == 'n') {
            if ((end - p) >= 4 && std::strncmp(p, "null", 4) == 0) {
                p += 4; v.type = Value::Type::Null; return v;
            }
            ok = false; return v;
        }

        // number
        const char* start = p;
        if (p < end && (*p == '-' || *p == '+')) ++p;
        while (p < end && ((*p >= '0' && *p <= '9') || *p == '.' ||
                           *p == 'e' || *p == 'E' || *p == '+' || *p == '-')) ++p;
        if (p == start) { ok = false; return v; }
        std::string num(start, (size_t)(p - start));
        v.type = Value::Type::Number;
        v.nval = std::strtod(num.c_str(), nullptr);
        return v;
    }
};

} // namespace detail

inline bool parse(const std::string& text, Value& out) {
    detail::Parser ps;
    ps.p   = text.c_str();
    ps.end = ps.p + text.size();
    out = ps.parseValue();
    return ps.ok;
}

// 从 JSON 文本中解析出根对象
inline Value parseOrEmpty(const std::string& text) {
    Value v;
    parse(text, v);
    return v;
}

} // namespace mjson
