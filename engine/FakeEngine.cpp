#include "FakeEngine.h"

#include <fstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include <nlohmann/json.hpp>

using nlohmann::json;

namespace {
// text/log 값 고르기: 문자열이면 공통, 객체면 worldKey -> 없으면 "default"
std::string pickForWorld(const json& v, const std::string& worldKey) {
    if (v.is_null()) return "";
    if (v.is_string()) return v.get<std::string>();
    if (v.contains(worldKey)) return v.at(worldKey).get<std::string>();
    return v.value("default", "");
}
}  // namespace

FakeEngine::FakeEngine(std::string storyPath, std::string worldKey)
    : storyPath_(std::move(storyPath)), worldKey_(std::move(worldKey)) {
    reload();
}

void FakeEngine::reload() {
    chronicle_.clear();
    try {
        load();
    } catch (const std::exception& e) {
        scenes_ = {{0,
                    "스토리 파일을 불러오지 못했다.\n" + storyPath_ + "\n\n" + e.what(),
                    {{"다시 불러오기", 0, true, ""}}}};
        current_ = 0;
    }
}

void FakeEngine::load() {
    std::ifstream in(storyPath_);
    if (!in) throw std::runtime_error("파일을 열 수 없다.");
    const json root = json::parse(in);
    const json& arr = root.at("scenes");
    if (arr.empty()) throw std::runtime_error("scenes가 비어 있다.");

    std::unordered_map<std::string, int> indexOf;
    for (size_t i = 0; i < arr.size(); ++i)
        indexOf[arr[i].at("id").get<std::string>()] = static_cast<int>(i);
    auto find = [&](const std::string& id) {
        auto it = indexOf.find(id);
        if (it == indexOf.end()) throw std::runtime_error("없는 장면 id: " + id);
        return it->second;
    };

    std::vector<Scene> scenes;
    for (const json& s : arr) {
        Scene scene{s.value("age", 0), pickForWorld(s.value("text", json()), worldKey_), {}};
        for (const json& c : s.at("choices")) {
            Choice choice;
            choice.text = c.at("text").get<std::string>();
            choice.restart = c.value("restart", false);
            choice.next = choice.restart ? 0 : find(c.at("next").get<std::string>());
            choice.log = pickForWorld(c.value("log", json()), worldKey_);
            scene.choices.push_back(std::move(choice));
        }
        scenes.push_back(std::move(scene));
    }

    current_ = find(root.value("start", arr[0].at("id").get<std::string>()));
    scenes_ = std::move(scenes);
}

EventView FakeEngine::currentEvent() {
    const Scene& s = scenes_[current_];
    EventView view;
    view.text = s.text;
    for (const Choice& c : s.choices) view.choices.push_back(c.text);
    return view;
}

void FakeEngine::choose(int index) {
    const Scene& s = scenes_[current_];
    if (index < 0 || index >= static_cast<int>(s.choices.size())) return;
    const Choice& c = s.choices[index];

    if (c.restart) {
        reload();  // JSON을 다시 읽으므로, 파일을 고친 내용이 바로 반영된다
        return;
    }
    if (!c.log.empty()) chronicle_.push_back(std::to_string(s.age) + "세, " + c.log);
    current_ = c.next;
}

std::vector<std::string> FakeEngine::chronicle() {
    return chronicle_;
}
