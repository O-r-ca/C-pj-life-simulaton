#pragma once
#include <string>
#include <vector>

// 시나리오 파일에 정의된 세계 목록 (화면의 세계 고르기용)
struct WorldDesc {
    std::string key;   // SimEngine에 넘기는 키
    std::string name;  // 화면 표시용 이름
    std::string desc;  // 한 줄 설명
};

// 실패하면 빈 목록
std::vector<WorldDesc> listWorlds(const std::string& scenarioPath);
