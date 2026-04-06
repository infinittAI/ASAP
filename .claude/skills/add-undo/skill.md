---
name: add-undo
description: ASAP Annotation에 Undo/Redo 기능을 추가하는 오케스트레이터. Qt Command Pattern 기반으로 annotation 생성, 수정, 삭제 연산에 undo/redo를 구현. "undo 추가", "되돌리기 기능", "Ctrl+Z", "annotation undo", "실행 취소" 등 undo/redo 관련 요청 시 반드시 이 스킬을 사용할 것. annotationplugin 모듈의 QUndoCommand 서브클래스 생성과 기존 annotation 도구에 command pattern을 통합하는 작업에 트리거.
---

# add-undo — ASAP Annotation Undo/Redo 오케스트레이터

Annotation 생성 및 수정 중 실수를 복구하는 undo/redo 기능을 ASAP에 추가하는 파이프라인을 조율한다.

## 실행 모드: 에이전트 팀

기존 3개 에이전트가 파이프라인으로 협업한다.

## 에이전트 구성

| 팀원 | 에이전트 타입 | 역할 | 스킬 | 출력 |
|------|-------------|------|------|------|
| feature-architect | 커스텀 (Plan) | Undo 아키텍처 설계, 구현 계획 수립 | add-undo references | `_workspace/01_architect_plan.md` |
| asap-developer | 커스텀 (general-purpose) | QUndoCommand 클래스 구현, 기존 코드 통합 | implement-plugin | 코드 변경 + `_workspace/02_developer_changelog.md` |
| qa-inspector | 커스텀 (general-purpose) | 빌드 검증, 경계면 교차 검사 | build-verify | `_workspace/03_qa_report.md` |

## 아키텍처 패턴

**파이프라인 + 생성-검증 피드백**

```
[사용자 요청]
    │
    ▼
[Phase 1: 설계]  ← feature-architect (서브 에이전트)
    │ _workspace/01_architect_plan.md
    │ 상세 스펙: references/undo-spec.md
    ▼
[Phase 2: 팀 구성]  ← 오케스트레이터
    │ TaskCreate (의존성 포함)
    ▼
[Phase 3: 구현]  ← asap-developer
    │ QUndoCommand 6개 클래스 + 기존 코드 수정
    │ _workspace/02_developer_changelog.md
    ▼
[Phase 4: 검증]  ← qa-inspector
    │ _workspace/03_qa_report.md
    ├─ PASS → 종료
    └─ FAIL → Developer에게 피드백 → Phase 3 재실행 (최대 2회)
```

## Phase 1: 설계 (서브 에이전트)

feature-architect를 서브 에이전트로 호출하여 구현 계획을 수립한다. 이때 `references/undo-spec.md`의 스펙을 기준으로 삼는다.

```
Agent(
  name="feature-architect",
  model="opus",
  prompt="ASAP annotation에 undo/redo 기능을 추가하기 위한 구현 계획서를 _workspace/01_architect_plan.md에 작성하라.
         [상세 스펙]: add-undo 스킬의 references/undo-spec.md를 읽고, 그 내용을 기준으로 계획을 수립하라.
         [요구사항]:
         - Qt Command Pattern(QUndoStack, QUndoCommand)을 사용할 것
         - 6개 연산에 대한 command 클래스를 설계할 것
         - annotationplugin 모듈 내에 commands/ 하위 디렉토리에 배치할 것
         - MoveCoordinateCommand는 mergeWith()로 drag 병합을 지원할 것
         - Ctrl+Z / Ctrl+Shift+Z 단축키를 지원할 것
         계획서에는 기능 요약, 아키텍처 결정, 파일 변경 목록, 구현 단계(의존성 순서), 테스트 계획, 리스크를 포함하라.",
  subagent_type="Plan"
)
```

**산출물**: `_workspace/01_architect_plan.md`

**주의:** Architect가 undo-spec.md의 내용을 재발견하지 않도록, 스펙 파일을 명시적으로 읽도록 지시한다.

## Phase 2: 팀 구성

### 태스크 생성

| 태스크 | 담당자 | 의존성 | 설명 |
|--------|--------|--------|------|
| T1: Command 클래스 구현 | asap-developer | 없음 | 6개 QUndoCommand 서브클래스 .h/.cpp 작성 |
| T2: CMakeLists 업데이트 | asap-developer | T1 | 새 소스 파일을 annotationplugin CMakeLists.txt에 추가 |
| T3: Plugin 통합 | asap-developer | T1, T2 | QUndoStack을 AnnotationWorkstationExtensionPlugin에 추가, undo/redo 액션 연결 |
| T4: Tool 통합 | asap-developer | T1 | AnnotationTool, PolyAnnotationTool에서 command push |
| T5: 빌드 및 경계면 검증 | qa-inspector | T1-T4 | 컴파일/링크/경계면 검증 |
| T6: 수정 (조건부) | asap-developer | T5 | QA 피드백 반영 |
| T7: 재검증 (조건부) | qa-inspector | T6 | 수정 후 재검증 |

### 데이터 전달

- **태스크 기반**: TaskCreate/TaskUpdate로 진행 상황 추적
- **파일 기반**: `_workspace/` 디렉토리에 산출물 저장
  - `01_architect_plan.md` — 설계 계획서 (undo-spec.md 기반)
  - `02_developer_changelog.md` — 변경 이력
  - `03_qa_report.md` — QA 검증 보고서

## Phase 3: 구현

asap-developer가 Architect의 계획서를 읽고 코드를 구현한다.

### 구현 순서 (undo-spec.md의 구현 순서 참조)

1. **Command 클래스 파일 생성** — `annotationplugin/commands/` 디렉토리에 6개 command 클래스
2. **CMakeLists.txt 업데이트** — 새 .cpp 파일 추가
3. **Plugin 수정** — QUndoStack, undo/redo 액션, finishAnnotation/deleteAnnotation 래핑
4. **Tool 수정** — AnnotationTool, PolyAnnotationTool에서 command push

### 구현 시 주의사항

Developer는 다음 사항을 반드시 준수한다:

- **헤더 가드**: `#pragma once`가 아닌 `#ifndef` 사용 (기존 코드와 일관성)
- **네이밍**: 클래스명 PascalCase, 멤버 `_` 접두사, 메서드 camelCase
- **메모리**: shared_ptr로 annotation 수명 관리, raw 포인터는 Qt 부모-자식 관계로만 사용
- **Signal**: coordinate 변경 시 `annotationChanged` signal이 발생해야 UI가 갱신됨. command의 undo()/redo()에서도 signal이 발생하는지 확인
- **Macro/Merge**: MoveCoordinateCommand의 `id()`는 1, `mergeWith()`에서 같은 index의 이동만 병합

## Phase 4: 검증 및 피드백

qa-inspector가 코드 변경을 검증한다.

### 검증 항목 (undo-spec.md의 경계면 검증 기준 참조)

1. **컴파일**: 모든 command 클래스가 QUndoCommand를 올바르게 상속
2. **링크**: CMakeLists.txt에 모든 새 .cpp가 포함, Qt6::Widgets 의존성 확인
3. **통합**: QUndoStack이 plugin→command로 올바르게 전달
4. **기능**: undo/redo 후 annotation 상태가 정확히 복원
5. **메모리**: shared_ptr이 command에서 유지되어 undo 시 접근 가능

### 피드백 사이클

- QA 판정이 FAIL이면 Developer에게 구체적인 수정 요청 전송 (파일:줄번호 + 수정 방법)
- Developer는 수정 후 TaskUpdate로 상태 갱신
- QA가 재검증 수행
- 최대 2회 재시도 후에도 미해결이면 오케스트레이터가 사용자에게 보고

## 에러 핸들링

| 상황 | 대응 |
|------|------|
| Architect가 스펙과 충돌하는 설계 제안 | undo-spec.md 기준으로 Architect에게 수정 요청 |
| 단일 빌드 에러 | Developer에게 수정 요청, 1회 재시도 |
| 경계면 불일치 (signal 미발생 등) | Developer에게 해당 command의 undo()/redo() 수정 지시 |
| mergeWith() 오동작 | 같은 index가 아닌 이동을 병합하는지 확인 후 수정 |
| QA 2회 재시도 후 FAIL | 사용자에게 현재 상황 보고, 방향성 결정 요청 |

## 테스트 시나리오

### 정상 흐름
1. 사용자: "annotation에 undo 기능을 추가해줘"
2. Architect가 undo-spec.md를 읽고 구현 계획서 작성
3. Developer가 6개 command 클래스 + plugin/tool 통합 구현
4. QA가 빌드 및 경계면 검증 → PASS
5. 결과: Ctrl+Z로 annotation 조작을 되돌릴 수 있음

### 에러 흐름
1. Developer가 MoveCoordinateCommand 구현 중 annotationChanged signal 미발생 발견
2. signal 발생을 위해 QtAnnotation 메서드 직접 호출로 변경
3. QA에서 재검증 → signal 발생 확인 → PASS

## 팀 해체

모든 작업 완료 후:
- `_workspace/` 디렉토리는 보존 (사후 검증 및 감사 추적용)
- 최종 결과를 사용자에게 요약 보고:
  - 추가된 command 클래스 수
  - 수정된 기존 파일
  - 단축키 안내 (Ctrl+Z / Ctrl+Shift+Z)
