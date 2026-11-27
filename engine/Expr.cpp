#include "Expr.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <stdexcept>
#include <vector>

namespace {

class Parser {
public:
    Parser(const std::string& src, const VarLookup& lookup) : src_(src), lookup_(lookup) {}

    double parse() {
        double v = parseOr();
        skipSpace();
        if (pos_ != src_.size()) fail("남는 글자");
        return v;
    }

private:
    const std::string& src_;
    const VarLookup& lookup_;
    size_t pos_ = 0;

    [[noreturn]] void fail(const std::string& why) const {
        throw std::runtime_error("수식 오류(" + why + "): " + src_);
    }

    void skipSpace() {
        while (pos_ < src_.size() && std::isspace(static_cast<unsigned char>(src_[pos_]))) ++pos_;
    }

    bool accept(const char* op) {
        skipSpace();
        size_t n = std::char_traits<char>::length(op);
        if (src_.compare(pos_, n, op) == 0) {
            pos_ += n;
            return true;
        }
        return false;
    }

    double parseOr() {
        double v = parseAnd();
        while (accept("||")) {
            double r = parseAnd();
            v = (v != 0 || r != 0) ? 1 : 0;
        }
        return v;
    }

    double parseAnd() {
        double v = parseCmp();
        while (accept("&&")) {
            double r = parseCmp();
            v = (v != 0 && r != 0) ? 1 : 0;
        }
        return v;
    }

    double parseCmp() {
        double v = parseAdd();
        // 두 글자 연산자를 먼저 본다
        if (accept("<=")) return v <= parseAdd() ? 1 : 0;
        if (accept(">=")) return v >= parseAdd() ? 1 : 0;
        if (accept("==")) return v == parseAdd() ? 1 : 0;
        if (accept("!=")) return v != parseAdd() ? 1 : 0;
        if (accept("<")) return v < parseAdd() ? 1 : 0;
        if (accept(">")) return v > parseAdd() ? 1 : 0;
        return v;
    }

    double parseAdd() {
        double v = parseMul();
        for (;;) {
            if (accept("+")) v += parseMul();
            else if (accept("-")) v -= parseMul();
            else return v;
        }
    }

    double parseMul() {
        double v = parseUnary();
        for (;;) {
            if (accept("*")) {
                v *= parseUnary();
            } else if (accept("/")) {
                double d = parseUnary();
                if (d == 0) fail("0으로 나눔");
                v /= d;
            } else {
                return v;
            }
        }
    }

    double parseUnary() {
        if (accept("-")) return -parseUnary();
        if (accept("!=")) fail("잘못된 위치의 !=");
        if (accept("!")) return parseUnary() == 0 ? 1 : 0;
        return parsePrimary();
    }

    double parsePrimary() {
        skipSpace();
        if (pos_ >= src_.size()) fail("식이 끝남");

        if (accept("(")) {
            double v = parseOr();
            if (!accept(")")) fail(") 없음");
            return v;
        }

        char c = src_[pos_];
        if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
            const char* begin = src_.c_str() + pos_;
            char* end = nullptr;
            double v = std::strtod(begin, &end);
            if (end == begin) fail("숫자");
            pos_ += static_cast<size_t>(end - begin);
            return v;
        }

        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            size_t start = pos_;
            while (pos_ < src_.size() &&
                   (std::isalnum(static_cast<unsigned char>(src_[pos_])) || src_[pos_] == '_'))
                ++pos_;
            std::string name = src_.substr(start, pos_ - start);

            if (accept("(")) {
                std::vector<double> args;
                if (!accept(")")) {
                    do args.push_back(parseOr()); while (accept(","));
                    if (!accept(")")) fail(") 없음");
                }
                return callFunc(name, args);
            }
            return lookup_(name);
        }

        fail(std::string("알 수 없는 글자 '") + c + "'");
    }

    double callFunc(const std::string& name, const std::vector<double>& a) const {
        if (name == "min" && a.size() == 2) return std::min(a[0], a[1]);
        if (name == "max" && a.size() == 2) return std::max(a[0], a[1]);
        if (name == "clamp" && a.size() == 3) return std::clamp(a[0], a[1], a[2]);
        fail("모르는 함수 또는 인자 개수: " + name);
    }
};

}  // namespace

double evalExpr(const std::string& src, const VarLookup& lookup) {
    return Parser(src, lookup).parse();
}
