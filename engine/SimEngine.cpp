#include "SimEngine.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "Expr.h"

using nlohmann::json;

namespace {

json readJson(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("파일을 열 수 없다: " + path);
    try {
        return json::parse(in);
    } catch (const std::exception& e) {
        throw std::runtime_error(path + "\n" + e.what());
    }
}

// 수식 자리: 숫자도 받고 문자열도 받는다
std::string exprOf(const json& v) {
    return v.is_string() ? v.get<std::string>() : v.dump();
}

// 정수로 반올림하고 천 단위 쉼표 (예: -12,345)
std::string formatNumber(double v) {
    long long n = std::llround(v);
    bool neg = n < 0;
    std::string digits = std::to_string(neg ? -n : n);
    std::string out;
    for (size_t i = 0; i < digits.size(); ++i) {
        if (i > 0 && (digits.size() - i) % 3 == 0) out += ',';
        out += digits[i];
    }
    return neg ? "-" + out : out;
}

}  // namespace

SimEngine::SimEngine(std::string storyPath, std::string scenarioPath, std::string worldKey,
                     unsigned seed)
    : storyPath_(std::move(storyPath)),
      scenarioPath_(std::move(scenarioPath)),
      worldKey_(std::move(worldKey)),
      seed_(seed) {
    reload();
}

void SimEngine::reload() {
    error_.clear();
    chronicle_.clear();
    records_.clear();
    try {
        load();
    } catch (const std::exception& e) {
        error_ = e.what();
    }
}

void SimEngine::load() {
    // --- 시나리오: 수치와 매년 규칙 ---
    const json sc = readJson(scenarioPath_);
    // 주의: value()는 임시 객체를 돌려주므로 지역 변수에 담은 뒤 items()를 돈다
    const json defaults = sc.value("defaults", json::object());
    json worldParams;
    for (const json& w : sc.at("worlds"))
        if (w.at("key").get<std::string>() == worldKey_) worldParams = w.value("params", json::object());
    if (worldParams.is_null()) throw std::runtime_error("없는 세계 키: " + worldKey_);
    const json yearly = sc.value("yearly", json::object());
    params_.clear();
    for (const auto& p : defaults.items()) params_[p.key()] = p.value().get<double>();
    for (const auto& p : worldParams.items()) params_[p.key()] = p.value().get<double>();
    yearly_.clear();
    for (const auto& r : yearly.items()) yearly_.push_back({r.key(), exprOf(r.value())});

    // --- 스토리: 인물 초기값, 장면 ---
    const json st = readJson(storyPath_);
    agent_ = Agent{};
    for (const auto& a : st.at("agent").items()) agent_.attrs[a.key()] = a.value().get<double>();
    if (!agent_.has("age")) throw std::runtime_error("agent에 age가 없다.");

    const json& arr = st.at("scenes");
    if (arr.empty()) throw std::runtime_error("scenes가 비어 있다.");
    std::unordered_map<std::string, int> indexOf;
    for (size_t i = 0; i < arr.size(); ++i)
        indexOf[arr[i].at("id").get<std::string>()] = static_cast<int>(i);
    auto find = [&](const std::string& id) {
        auto it = indexOf.find(id);
        if (it == indexOf.end()) throw std::runtime_error("없는 장면 id: " + id);
        return it->second;
    };
    auto parseSet = [](const json& obj) {
        Assignments set;
        for (const auto& s : obj.items()) set.push_back({s.key(), exprOf(s.value())});
        return set;
    };

    std::vector<Scene> scenes;
    for (const json& s : arr) {
        Scene scene;
        scene.age = s.at("age").get<int>();

        const json& t = s.at("text");
        if (t.is_string()) {
            scene.texts.push_back({"", t.get<std::string>()});
        } else {
            for (const json& v : t) scene.texts.push_back({v.value("if", ""), v.at("text").get<std::string>()});
        }

        for (const json& c : s.at("choices")) {
            Choice choice;
            choice.text = c.at("text").get<std::string>();
            choice.restart = c.value("restart", false);
            if (!choice.restart) choice.next = find(c.at("next").get<std::string>());

            if (c.contains("outcomes")) {
                for (const json& o : c.at("outcomes"))
                    choice.outcomes.push_back({exprOf(o.value("weight", json(1))),
                                               parseSet(o.value("set", json::object())),
                                               o.value("log", "")});
            } else {
                // outcomes가 없으면 선택지 자체의 set/log가 확정 결과
                choice.outcomes.push_back({"1", parseSet(c.value("set", json::object())), c.value("log", "")});
            }
            scene.choices.push_back(std::move(choice));
        }
        scenes.push_back(std::move(scene));
    }

    scenes_ = std::move(scenes);
    current_ = find(st.value("start", arr[0].at("id").get<std::string>()));
    rng_.seed(seed_);
    record();
    advanceTo(scenes_[current_].age);
}

double SimEngine::eval(const std::string& expr) const {
    return evalExpr(expr, [this](const std::string& name) {
        auto a = agent_.attrs.find(name);
        if (a != agent_.attrs.end()) return a->second;
        auto p = params_.find(name);
        if (p != params_.end()) return p->second;
        throw std::runtime_error("모르는 이름: " + name);
    });
}

std::string SimEngine::interpolate(const std::string& tmpl) const {
    std::string out;
    size_t pos = 0;
    while (pos < tmpl.size()) {
        size_t open = tmpl.find('{', pos);
        if (open == std::string::npos) break;
        size_t close = tmpl.find('}', open);
        if (close == std::string::npos) break;
        out += tmpl.substr(pos, open - pos);
        out += formatNumber(eval(tmpl.substr(open + 1, close - open - 1)));
        pos = close + 1;
    }
    out += tmpl.substr(pos);
    return out;
}

void SimEngine::apply(const Assignments& set) {
    // 모두 옛 값으로 계산한 뒤 한꺼번에 대입 (JSON 키 순서에 결과가 좌우되지 않게)
    std::vector<double> values;
    for (const auto& [name, expr] : set) values.push_back(eval(expr));
    for (size_t i = 0; i < set.size(); ++i) agent_.attrs[set[i].first] = values[i];
}

void SimEngine::advanceTo(int age) {
    while (agent_.get("age") < age) {
        apply(yearly_);
        agent_.attrs["age"] += 1;
        record();
    }
}

void SimEngine::record() {
    YearRecord r{static_cast<int>(agent_.get("age")), agent_.attrs};
    if (!records_.empty() && records_.back().age == r.age)
        records_.back() = std::move(r);
    else
        records_.push_back(std::move(r));
}

std::vector<YearRecord> SimEngine::yearlyRecords() {
    return records_;
}

double SimEngine::nextRandom() {
    // 표준 분포 클래스는 구현마다 결과가 다를 수 있어서 직접 변환 (상위 24비트)
    return static_cast<double>(rng_() >> 8) / 16777216.0;
}

EventView SimEngine::currentEvent() {
    EventView view;
    if (error_.empty()) {
        try {
            const Scene& s = scenes_[current_];
            for (const CondText& t : s.texts) {
                if (t.cond.empty() || eval(t.cond) != 0) {
                    view.text = interpolate(t.text);
                    break;
                }
            }
            for (const Choice& c : s.choices) view.choices.push_back(c.text);
            return view;
        } catch (const std::exception& e) {
            error_ = e.what();
        }
    }
    view.text = "엔진 오류\n\n" + error_;
    view.choices = {"다시 불러오기"};
    return view;
}

void SimEngine::choose(int index) {
    if (!error_.empty()) {
        reload();
        return;
    }
    try {
        const Scene& s = scenes_[current_];
        if (index < 0 || index >= static_cast<int>(s.choices.size())) return;
        const Choice& c = s.choices[index];

        if (c.restart) {
            reload();  // JSON을 다시 읽고 같은 시드로 처음부터
            return;
        }

        // 결과가 몇 개든 난수는 꼭 하나만 뽑는다 (세계 간 운을 맞추기 위해)
        const double roll = nextRandom();

        std::vector<double> weights;
        double total = 0;
        for (const Outcome& o : c.outcomes) {
            double w = std::max(0.0, eval(o.weight));
            weights.push_back(w);
            total += w;
        }
        if (total > 0) {
            double target = roll * total;
            size_t picked = weights.size() - 1;
            for (size_t i = 0; i < weights.size(); ++i) {
                if (target < weights[i]) { picked = i; break; }
                target -= weights[i];
            }
            const Outcome& o = c.outcomes[picked];
            const int age = static_cast<int>(agent_.get("age"));
            apply(o.set);
            record();
            std::string log = interpolate(o.log);
            if (!log.empty()) chronicle_.push_back(std::to_string(age) + "세, " + log);
        }

        current_ = c.next;
        advanceTo(scenes_[current_].age);
    } catch (const std::exception& e) {
        error_ = e.what();
    }
}

std::vector<std::string> SimEngine::chronicle() {
    return chronicle_;
}
