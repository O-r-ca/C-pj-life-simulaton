# 이벤트·시나리오 JSON 작성 규칙 (확정판)

스토리 담당은 `story/` 폴더의 이벤트 JSON을, 데이터 담당은 `scenarios/`의 세계 JSON과 `data/policies.json`을 작성한다.
규칙을 바꾸거나 새 필드가 필요하면 먼저 바댜에게 알린다. (엔진 코드와 맞춰야 하기 때문)
엔진이 이 데이터를 어떻게 처리하는지는 `ENGINE_SPEC.md`를 본다.

## 가장 중요한 규칙
**선택지에는 "기본 효과"와 "관련 정책 id"만 적는다. 정책이 실제로 뭘 해주는지는 시나리오(세계) 파일에서 정한다.**

같은 선택을 정책이 다른 세계에 돌려서 비교하는 게 핵심이라, 정책 효과를 선택지 안에 박아두면 세계가 달라도 결과가 같아져서 비교가 안 된다.

## 이벤트 파일 (story/*.json)

```json
{
  "events": [
    {
      "id": "age21_rent",
      "age": 21,
      "text": "21세. 본가에서 나와 혼자 살아볼까 고민된다.\n어떻게 할까?",
      "choices": [
        {
          "text": "월세방을 구한다",
          "log": "본가에서 나와 월세방을 구했다.",
          "effects": { "cash": -500, "deposit": 500 },
          "policies": ["youth_rent_support"],
          "next": "age24_jobhunt"
        },
        {
          "text": "본가에 계속 산다",
          "log": "본가에 남아 돈을 아꼈다.",
          "effects": { "health": -3 },
          "policies": [],
          "next": "age24_jobhunt"
        }
      ]
    }
  ]
}
```

### 필드 설명
- `id`: 이벤트 고유 이름. 형식은 `age{나이}_{주제}` (예: `age19_college`). 중복 금지.
- `age`: 이 이벤트가 일어나는 나이.
- `text`: 상황 설명. 줄바꿈은 `\n`. 특정 정치 성향을 드러내는 표현은 쓰지 않는다.
- `choices`: 선택지 목록 (2~4개 권장).
  - `text`: 선택지 문구.
  - `log`: (선택) 연대기에 남을 한 문장. **과거형**으로, 나이는 빼고 쓴다. 엔진이 앞에 `"21세, "`를 붙인다. 없으면 선택지 `text`가 그대로 들어간다.
  - `effects`: 선택의 **기본 효과**. 아래 허용 키만 사용. 없으면 `{}`.
  - `policies`: 이 선택과 관련된 정책 id 목록. 없으면 `[]`. 여러 개면 적은 순서대로 적용된다.
  - `next`: 다음 이벤트의 `id`, 또는 인생을 끝내는 `"end"`.
    다음 이벤트의 `age`는 현재 이벤트의 `age`보다 작으면 안 된다.

`log`는 연대기를 서사로 만드는 재료다. "결혼한다"보다 "집값이 부담돼 결혼을 미뤘다."처럼 **무엇을 했고 왜 그랬는지**가 드러나게 쓴다.
단, 세계마다 달라지는 결과(지원을 받았다 등)는 `log`에 쓰지 않는다. 정책 줄은 엔진이 따로 붙인다.

### effects 허용 키와 단위
| 키 | 의미 | 적용 방식 | 단위 |
|---|---|---|---|
| `cash` | 보유 현금 | **증감** (지출은 음수) | 만원 |
| `loan` | 대출 | **증감** (대출이 늘면 양수, 갚으면 음수) | 만원 |
| `deposit` | 전·월세 보증금 | **증감** (낼 때 양수, 돌려받을 때 음수) | 만원 |
| `house` | 소유한 집의 가치 | **증감** (살 때 양수, 팔 때 음수) | 만원 |
| `salary` | 연봉 | **설정** (그 값으로 바뀜, 0이면 소득 없음) | 만원/년 |
| `health` | 건강 점수 | **증감** | 점수 (0~100) |

`salary`만 설정이라는 점에 주의한다. (예: 은퇴 시 `"salary": 0`, 이직 시 `"salary": 5000`)
새로운 키(예: `happiness`, `family`)가 필요하면 바댜에게 먼저 요청한다. 임의로 만들지 않는다.

### 돈이 자산으로 바뀌는 선택은 짝을 맞춘다
보증금을 내거나 집을 사는 건 돈이 사라지는 게 아니라 다른 자산으로 바뀌는 것이다. **양쪽을 같이 적어야** 순자산(`cash + deposit + house - loan`)이 맞게 나온다.

| 상황 | effects 예시 |
|---|---|
| 보증금 3,000을 현금으로 냄 | `{ "cash": -3000, "deposit": 3000 }` |
| 전세 3억, 현금 1억 + 대출 2억 | `{ "cash": -10000, "loan": 20000, "deposit": 30000 }` |
| 전세 끝나고 6억 집을 삼 (보증금 3억 돌려받아 보태고, 대출 3억 추가) | `{ "deposit": -30000, "loan": 30000, "house": 60000 }` |

`cash`만 크게 빼면 그래프에서 순자산이 갑자기 떨어진 것처럼 보이니 주의한다.

### cash는 이벤트 사이에 자동으로 쌓인다
이벤트 안에서 월급이나 생활비를 `cash`로 직접 넣지 않는다. 엔진이 이벤트 사이 기간만큼 매년 `salary - 생활비 - 대출 이자 - 대출 원금 상환`을 cash에 정산한다. 대출 원금은 그만큼 줄어든다.
`effects`에는 그 선택으로 생기는 **일회성 지출·수입**만 적는다. (등록금, 보증금, 학원비 등)

### 엔딩
마지막 이벤트의 선택지는 `"next": "end"`로 끝낸다. 엔딩 문구는 따로 쓰지 않는다. 엔진이 최종 상태로 요약 줄을 만든다.

## 정책 사전 (data/policies.json)

정책마다 id(코드용), 한글 이름(화면용), 분류를 정한다. 이벤트와 시나리오는 항상 **id**만 쓴다.

```json
{
  "policies": [
    { "id": "youth_rent_support", "name": "청년월세지원", "category": "youth" }
  ]
}
```

현재 이벤트에서 쓰는 정책 id와 이름이다.

| id | 한글 이름 | category |
|---|---|---|
| `youth_rent_support` | 청년월세지원 | youth |
| `youth_job_leap_subsidy` | 청년일자리도약장려금 | youth |
| `youth_future_savings` | 청년미래적금 | youth |
| `job_seeker_allowance` | 구직급여 | employment |
| `crisis_employment_retention` | 고용유지지원 (가칭) | employment |
| `newborn_special_loan` | 신생아특례대출 | marriage |
| `newlywed_jeonse_loan` | 버팀목신혼부부전세대출 | marriage |
| `parent_benefit` | 부모급여 | birth |
| `first_meeting_voucher` | 첫만남이용권 | birth |
| `child_allowance` | 아동수당 | birth |
| `child_basic_allowance` | 아동기본수당 | birth |
| `parental_leave_pay` | 육아휴직급여 | childcare |
| `pension_reform` | 국민연금개혁 | elderly |
| `long_term_care` | 장기요양보험 | elderly |

확인된 것 (근거: `docs/policy_research.md`, 2026-11 조사):
- `구직급여`는 청년 전용이 아니므로 `employment`로 분류한다.
- `코로나고용유지특례`는 2045년 배경에 어색해서 "고용유지지원"(가칭)으로 일반화했다. 근거는 코로나19 고용유지지원금 특례(휴업수당 최대 90% 지원, 2020~2021)다.
- `아동수당`은 **현행 제도**다 (2026년 만 9세 미만, 2030년까지 13세 미만으로 확대). `아동기본수당`은 아동수당·부모급여·첫만남이용권을 하나로 합치는 **개편안**(2027년 예산 자료, 확정 아님)이다.
  → 둘 다 사전에 남기되, **한 세계에 같이 정의하지 않는다.** `child_basic_allowance`를 정의한 세계에는 `child_allowance`, `parent_benefit`, `first_meeting_voucher`를 정의하지 않는다. (중복 지급 방지)

정책 id는 영어 소문자와 밑줄만 쓴다. 새 정책을 선택지에 쓰면 데이터 담당에게 알려서 사전과 두 세계의 정의를 채운다.

## 시나리오 파일 (scenarios/*.json)

세계마다 시작값, 생활비·금리 같은 상수, 그리고 어떤 정책이 있고 그 정책이 효과를 어떻게 바꾸는지 정의한다.

```json
{
  "world": "지원 확대",
  "start": { "cash": 0, "loan": 0, "salary": 0, "health": 80 },
  "constants": {
    "living_cost_per_year": 1200,
    "loan_interest_rate": 0.04,
    "loan_repay_rate": 0.035,
    "house_price_growth": 0.02
  },
  "policies": {
    "youth_rent_support":    { "yearly": { "cash": 240 }, "years": 2 },
    "first_meeting_voucher": { "effects": { "cash": 200 } },
    "newlywed_jeonse_loan":  { "constants": { "loan_interest_rate": 0.025 } }
  }
}
```

```json
{
  "world": "현행 유지",
  "start": { "cash": 0, "loan": 0, "salary": 0, "health": 80 },
  "constants": {
    "living_cost_per_year": 1200,
    "loan_interest_rate": 0.04,
    "loan_repay_rate": 0.035,
    "house_price_growth": 0.02
  },
  "policies": {}
}
```

위 숫자는 형식을 보여주는 **예시값**이다. 실제 값은 데이터 담당이 통계로 정한다.

### constants
| 이름 | 의미 | 필수 |
|---|---|---|
| `living_cost_per_year` | 1년 생활비 (만원) | 필수 |
| `loan_interest_rate` | 대출 연 이자율 (0.04 = 4%) | 필수 |
| `loan_repay_rate` | 해마다 갚는 원금 비율 (0.035면 약 28년에 걸쳐 갚는 셈) | 필수 |
| `house_price_growth` | 집값 연 변동률 | 선택 (없으면 0) |
| `start`에 없는 상태 키 | 0으로 시작 | — |

### 정책 정의: 세 가지 효과
정책 하나에 아래 중 필요한 것만 쓴다. 섞어 써도 된다.

| 필드 | 언제 | 예 | 쓸 수 있는 키 |
|---|---|---|---|
| `effects` | 선택하는 순간 **한 번** | 첫만남이용권 200만원 | `effects` 허용 키 전부 (증감/설정 규칙 동일) |
| `yearly` + `years` | 선택 다음 해부터 **`years`년 동안 매년** | 청년월세 연 240만원 × 2년, 아동수당 | `cash`, `health` |
| `constants` | 그 사람에게 **이후 계속** 상수를 바꿈 | 정책 대출 금리 2.5% | 시나리오 `constants`에 있는 이름 |

정책이 "얼마를 주는지"뿐 아니라 "얼마나 오래 주는지", "조건을 어떻게 바꾸는지"까지 실제 제도에 가깝게 쓰기 위한 구분이다.

위 예시에서 `월세방을 구한다`(기본 `cash -500, deposit +500`)는
- "현행 유지" 세계: 정책 없음 → 보증금만 내고 끝. 연대기에 정책 줄 없음.
- "지원 확대" 세계: `youth_rent_support` 적용 → 22~23세 정산 때 매년 `cash +240`. 연대기에 `21세, 청년월세지원을 받았다. (2년간 매년 +240만원)`

이렇게 같은 선택에서 세계에 따라 결과가 달라진다.

## 작성 체크리스트
- 파일은 UTF-8, JSON 문법 검사 통과 (쉼표, 따옴표 주의).
- `id`가 겹치지 않는다.
- `next`가 존재하는 이벤트 id이거나 `"end"`이다.
- `next` 이벤트의 `age`가 현재 `age`보다 작지 않다.
- 선택지에 정책 효과가 직접 들어가 있지 않다. `log`에도 세계마다 달라지는 결과를 쓰지 않았다.
- `effects`에 월급·생활비 같은 매년 반복되는 값을 넣지 않았다.
- 보증금·집처럼 돈이 자산으로 바뀌는 선택은 `cash`와 `deposit`/`house`/`loan`의 짝을 맞췄다.
- 단위(만원)를 지켰다.
- 선택지에 쓴 정책 id가 정책 사전에 있다.
- 정책 정의에 `yearly`를 썼으면 `years`도 썼다.
- `child_basic_allowance`와 기존 출산 정책 셋을 한 세계에 같이 정의하지 않았다.

## 앞으로 추가될 수 있는 필드 (지금은 쓰지 않는다)
- `requires`: 이전 선택이나 속성에 따라 이벤트가 나오는 조건. (예: 서울 진학자에게만 나오는 이벤트)
- `chance`: 같은 선택에도 확률적으로 결과가 갈리는 경우의 확률 표현.
- 정책이 인생 도중에 랜덤으로 생기거나 바뀌는 방식.
- 결혼·아이 같은 가족 상태 키.

필요해지면 바댜가 형식을 정해서 공유한다.

## 변경 이력
- 2026-11-27
  - 선택지에 `log`(연대기 문장) 추가.
  - effects 키에 `deposit`(보증금), `house`(집) 추가. 자산 짝 맞추기 규칙 추가.
  - 시나리오 `constants`에 `loan_repay_rate`(필수), `house_price_growth`(선택) 추가.
  - 정책 정의를 `effects` / `yearly`+`years` / `constants` 세 가지로 확장.
  - 정책 사전 "확인할 것"에 조사 결과 반영 (아동수당 vs 아동기본수당).
