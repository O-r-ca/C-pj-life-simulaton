#pragma once
#include "Engine.h"

// 스토리 JSON을 읽어서 고정된 장면을 돌려주는 가짜 엔진 (GUI 개발용)
// 장면 구조(선택지)는 모든 세계가 같고, 상황 글과 연대기 문장만 worldKey에 따라 다르다.
// 그래야 같은 선택 번호를 여러 세계에 그대로 넣을 수 있다.
class FakeEngine : public Engine {
public:
    // worldKey: JSON의 text/log 객체에서 고를 키 (예: "default", "housing_support")
    FakeEngine(std::string storyPath, std::string worldKey);
    EventView currentEvent() override;
    void choose(int index) override;
    std::vector<std::string> chronicle() override;

private:
    struct Choice {
        std::string text;
        int next;        // 다음 장면 번호
        bool restart;    // true면 JSON을 다시 읽고 처음부터
        std::string log; // 연대기 문장 (비어 있으면 안 남김)
    };
    struct Scene {
        int age;
        std::string text;
        std::vector<Choice> choices;
    };

    void reload();  // 실패하면 오류 장면을 보여준다
    void load();    // 실패하면 예외

    std::string storyPath_;
    std::string worldKey_;
    std::vector<Scene> scenes_;
    int current_ = 0;
    std::vector<std::string> chronicle_;
};
