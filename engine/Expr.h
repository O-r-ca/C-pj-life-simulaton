#pragma once
#include <functional>
#include <string>

// JSON에 적는 작은 수식 계산기.
// 지원: 숫자, 이름(에이전트 속성·시나리오 수치), + - * / ( ),
//       < > <= >= == != && || !, 함수 min(a,b) max(a,b) clamp(x,lo,hi)
// 비교·논리 연산 결과는 1 또는 0.
using VarLookup = std::function<double(const std::string&)>;

// 문법 오류, 모르는 이름, 0으로 나누기 등은 std::runtime_error
double evalExpr(const std::string& src, const VarLookup& lookup);
