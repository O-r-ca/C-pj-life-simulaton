#pragma once
#include <map>
#include <string>

// 에이전트(인물). 속성은 이름 -> 숫자로 들고 있어서,
// 코드를 고치지 않고 스토리 JSON에서 새 속성(예: health)을 바로 추가할 수 있다.
// 참/거짓 속성(married 등)은 1/0으로 쓴다.
struct Agent {
    std::map<std::string, double> attrs;

    double get(const std::string& name) const { return attrs.at(name); }
    bool has(const std::string& name) const { return attrs.count(name) > 0; }
};
