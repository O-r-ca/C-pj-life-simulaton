#pragma once
#include <map>
#include <string>
#include <vector>

// 화면이 엔진에서 받아가는 "지금 상황" 데이터.
// 엔진은 화면을 모른다: cout, cin, 그리기 코드 금지.
struct EventView {
    std::string text;
    std::vector<std::string> choices;
};

// 한 해의 기록: 그 나이일 때 인물 속성들 (예: "assets" -> 1200)
struct YearRecord {
    int age;
    std::map<std::string, double> values;
};

// 화면 코드는 이 인터페이스만 본다.
// 가짜 엔진 -> 진짜 엔진 교체 시 화면 코드는 그대로.
class Engine {
public:
    virtual ~Engine() = default;
    virtual EventView currentEvent() = 0;
    virtual void choose(int index) = 0;
    // 지금까지의 인생 기록 ("19세, 서울의 대학에 진학했다." 같은 문장들, 오래된 순)
    virtual std::vector<std::string> chronicle() = 0;
    // 해마다의 기록 (나이 순). 그래프용. 기록이 없는 엔진은 빈 목록
    virtual std::vector<YearRecord> yearlyRecords() { return {}; }
};
