---
name: add-feature
description: ASAP 프로젝트에 새로운 기능을 추가하는 전체 워크플로우를 오케스트레이션한다. 새 플러그인 개발, 파일 포맷 지원, GUI 도구 추가, 이미지 처리 필터, 명령줄 도구, Python 바인딩 등 ASAP의 모든 기능 확장 요청에 반드시 이 스킬을 사용할 것. "기능 추가해줘", "새 플러그인 만들어줘", "필터 추가", "도구 추가", "포맷 지원" 등의 요청 시 트리거.
---

# add-feature — ASAP 기능 추가 오케스트레이터

ASAP 프로젝트에 새 기능을 추가하는 전체 파이프라인을 조율한다. 에이전트 팀 모드로 3명의 전문가가 협업한다.

## 실행 모드

**에이전트 팀** — Feature Architect, ASAP Developer, QA Inspector 세 에이전트가 협업한다. Architect가 설계를 완료하면 Developer가 구현하고, QA가 검증한다. 검증 실패 시 Developer에게 피드백이 전달되어 수정-재검증 사이클이 돌아간다.

## 아키텍처 패턴

**파이프라인 + 생성-검증 피드백**

```
[사용자 요청]
    │
    ▼
[Phase 1: 분석]  ← feature-architect (서브 에이전트)
    │ _workspace/01_architect_plan.md
    ▼
[Phase 2: 팀 구성]  ← 오케스트레이터
    │ TeamCreate + TaskCreate
    ▼
[Phase 3: 구현]  ← asap-developer
    │ 코드 변경 + _workspace/02_developer_changelog.md
    ▼
[Phase 4: 검증]  ← qa-inspector
    │ _workspace/03_qa_report.md
    ├─ PASS → 종료
    └─ FAIL → Developer에게 피드백 → Phase 3 재실행 (최대 2회)
```

## Phase 1: 분석 및 계획 (서브 에이전트)

오케스트레이터가 feature-architect를 서브 에이전트로 호출하여 요구사항을 분석하고 구현 계획을 수립한다.

```
Agent(
  name="feature-architect",
  model="opus",
  prompt="다음 기능 요구사항을 분석하여 _workspace/01_architect_plan.md에 구현 계획서를 작성하라.
         [사용자 요구사항]: {user_request}
         [컨텍스트]: {context}
         계획서에는 기능 요약, 아키텍처 결정, 파일 변경 목록, 구현 단계, 테스트 계획, 리스크를 포함하라.",
  subagent_type="Plan"
)
```

**산출물**: `_workspace/01_architect_plan.md`

## Phase 2: 팀 구성

계획서가 완성되면 오케스트레이터가 에이전트 팀을 구성한다.

### 태스크 생성

| 태스크 | 담당자 | 의존성 | 설명 |
|--------|--------|--------|------|
| T1: 구현 | asap-developer | 없음 | Architect의 계획에 따라 코드 구현 |
| T2: 검증 | qa-inspector | T1 | 빌드 및 통합 정합성 검증 |
| T3: 수정 (조건부) | asap-developer | T2 | QA 피드백 반영 (필요 시) |
| T4: 재검증 (조건부) | qa-inspector | T3 | 수정 후 재검증 |

### 데이터 전달

- **태스크 기반**: TaskCreate/TaskUpdate로 진행 상황 추적
- **파일 기반**: `_workspace/` 디렉토리에 산출물 저장
  - `01_architect_plan.md` — 설계 계획서
  - `02_developer_changelog.md` — 변경 이력
  - `03_qa_report.md` — QA 검증 보고서

## Phase 3: 구현

asap-developer가 Architect의 계획서를 읽고 코드를 구현한다.

- Developer는 계획서의 구현 단계를 순서대로 수행한다.
- 각 단계 완료 후 TaskUpdate로 진행 상황을 보고한다.
- 모든 구현 완료 후 `_workspace/02_developer_changelog.md`를 작성한다.
- 플러그인 구현 시 `implement-plugin` 스킬을 참고할 수 있다.

## Phase 4: 검증 및 피드백

qa-inspector가 코드 변경을 검증한다.

- QA는 Developer의 변경 이력과 Architect의 계획서를 모두 읽고 검증을 수행한다.
- 빌드 검증: `build-verify` 스킬의 지침에 따라 CMake 구성 및 빌드를 실행한다.
- 경계면 검증: 새 코드와 기존 인터페이스 간 연결을 교차 확인한다.
- 결과를 `_workspace/03_qa_report.md`에 작성한다.

**피드백 사이클**:
- QA 판정이 FAIL이면 Developer에게 구체적인 수정 요청을 SendMessage로 전송한다.
- Developer는 수정 후 TaskUpdate로 상태를 갱신한다.
- QA가 재검증을 수행한다.
- 최대 2회 재시도 후에도 미해결이면 오케스트레이터가 사용자에게 보고한다.

## 에러 핸들링

| 상황 | 대응 |
|------|------|
| Architect 분석 실패 | 사용자에게 요구사항 명확화 요청 |
| 단일 빌드 에러 | Developer에게 수정 요청, 1회 재시도 |
| 반복 빌드 실패 | 사용자에게 환경 점검 요청 |
| 경계면 불일치 | 양쪽 파일 모두 분석 후 Developer에게 수정 지시 |
| QA 2회 재시도 후에도 FAIL | 사용자에게 현재 상황 보고, 방향성 결정 요청 |
| 서드파티 의존성 문제 | cmakemodules/의 찾기 모듈 확인, 필요 시 사용자에게 설치 지침 제공 |

## 테스트 시나리오

### 정상 흐름
1. 사용자: "색상 정규화 필터 플러그인을 추가해줘"
2. Architect가 요구사항을 분석하여 계획서 작성 (ImageFilterPluginInterface 기반)
3. Developer가 계획에 따라 필터 클래스, 설정 패널, CMakeLists.txt 구현
4. QA가 빌드 및 플러그인 로드 검증 → PASS
5. 결과를 사용자에게 보고

### 에러 흐름
1. 사용자: "DICOM 프레임 읽기 기능을 추가해줘"
2. Architect가 계획서 작성 중 DCMTK 의존성 충돌 발견
3. 사용자에게 DCMTK 버전 호환성 문제 보고
4. 사용자가 DCMTK 버전 업데이트 승인
5. Architect가 수정된 계획서 작성 후 계속 진행

## 팀 해체

모든 작업 완료 후:
- `_workspace/` 디렉토리는 보존하여 사후 검증 및 감사 추적에 활용한다.
- 최종 결과를 사용자에게 요약하여 보고한다.
