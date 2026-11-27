#pragma once
#include <map>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "Agent.h"
#include "Engine.h"

// 진짜 엔진 첫 버전.
// - 시나리오 JSON: 세계 수치(params)와 매년 적용되는 규칙(yearly)
// - 스토리 JSON: 장면, 선택지, 확률적 결과(outcomes)
// - 시드 고정 난수: 선택 한 번에 난수를 정확히 하나만 뽑는다.
//   장면 구조가 같은 두 세계는 같은 시드면 같은 "운"을 받으므로, 결과 차이는 환경 때문이다.
class SimEngine : public Engine {
public:
    SimEngine(std::string storyPath, std::string scenarioPath, std::string worldKey, unsigned seed);
    EventView currentEvent() override;
    void choose(int index) override;
    std::vector<std::string> chronicle() override;
    std::vector<YearRecord> yearlyRecords() override;

private:
    using Assignments = std::vector<std::pair<std::string, std::string>>;  // 속성 -> 수식

    struct Outcome {
        std::string weight;  // 수식. 결과들 중 하나를 가중치 비율로 뽑는다
        Assignments set;     // 동시에 적용 (모두 옛 값으로 계산한 뒤 대입)
        std::string log;     // 연대기 문장. {수식} 자리에 값이 들어간다
    };
    struct Choice {
        std::string text;
        int next = 0;
        bool restart = false;
        std::vector<Outcome> outcomes;
    };
    struct CondText {
        std::string cond;  // 비어 있으면 항상 참
        std::string text;
    };
    struct Scene {
        int age = 0;
        std::vector<CondText> texts;  // 조건이 맞는 첫 글을 쓴다
        std::vector<Choice> choices;
    };

    void reload();  // 실패하면 error_에 기록
    void load();    // 실패하면 예외

    double eval(const std::string& expr) const;
    std::string interpolate(const std::string& tmpl) const;
    void apply(const Assignments& set);
    void advanceTo(int age);  // 한 해씩 yearly 규칙 적용 (틱 루프)
    double nextRandom();      // [0, 1)
    void record();            // 지금 속성을 올해 기록으로 저장 (같은 나이면 덮어씀)

    std::string storyPath_, scenarioPath_, worldKey_;
    unsigned seed_;
    std::mt19937 rng_;

    std::map<std::string, double> params_;
    Assignments yearly_;
    Agent agent_;
    std::vector<Scene> scenes_;
    int current_ = 0;
    std::vector<std::string> chronicle_;
    std::vector<YearRecord> records_;
    std::string error_;  // 비어 있지 않으면 오류 화면
};
