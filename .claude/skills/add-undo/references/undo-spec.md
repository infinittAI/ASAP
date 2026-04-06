# ASAP Annotation Undo/Redo — 구현 스펙

Annotation 생성 및 수정 중 실수를 복구하는 undo/redo 기능의 상세 구현 스펙.

## 목차

1. [대상 연산](#1-대상-연산)
2. [Qt Command Pattern 구조](#2-qt-command-pattern-구조)
3. [Command 클래스 설계](#3-command-클래스-설계)
4. [QUndoStack 관리](#4-qundostack-관리)
5. [UI/단축키](#5-ui단축키)
6. [적용 범위](#6-적용-범위)
7. [파일 변경 목록](#7-파일-변경-목록)
8. [구현 순서](#8-구현-순서)
9. [경계면 검증 기준](#9-경계면-검증-기준)

---

## 1. 대상 연산

| # | 연산 | 발생 시점 | 설명 |
|---|------|----------|------|
| 1 | Point 추가 | 생성 중 | Polygon 생성 중 클릭으로 추가한 각 vertex |
| 2 | Polygon 생성 완료 | finishAnnotation() | Annotation이 목록에 추가된 것 |
| 3 | Vertex 이동 | 수정 중 | 기존 annotation의 vertex를 drag하여 이동 |
| 4 | Vertex 삽입 | 수정 중 | 기존 edge를 더블클릭하여 vertex 삽입 |
| 5 | Vertex 삭제 | 수정 중 | vertex 선택 후 Del 키로 삭제 |
| 6 | Annotation 삭제 | 선택 후 | E 키 또는 트리에서 annotation 삭제 |

## 2. Qt Command Pattern 구조

```
QUndoStack (AnnotationWorkstationExtensionPlugin 소유)
  └─ QUndoCommand 서브클래스들
       ├─ AddCoordinateCommand       (#1 생성 중 point 추가)
       ├─ CreateAnnotationCommand    (#2 annotation 생성 완료)
       ├─ MoveCoordinateCommand      (#3 vertex 이동)
       ├─ InsertCoordinateCommand    (#4 edge에 vertex 삽입)
       ├─ RemoveCoordinateCommand    (#5 vertex 삭제)
       └─ DeleteAnnotationCommand    (#6 annotation 전체 삭제)
```

각 Command는 `undo()` / `redo()` 메서드로 상태를 복원한다. `QUndoCommand::id()`와 `mergeWith()`를 사용하여 연속적인 vertex 이동(mouseMove)을 하나의 command로 병합한다.

## 3. Command 클래스 설계

### 3.1 AddCoordinateCommand

생성 중인 annotation에 point를 추가하는 명령.

```cpp
class AddCoordinateCommand : public QUndoCommand {
public:
  AddCoordinateCommand(QtAnnotation* annotation, float x, float y, QUndoCommand* parent = nullptr);
  void undo() override;
  void redo() override;
private:
  QtAnnotation* _annotation;
  float _x, _y;
};
```

- `redo()`: `annotation->addCoordinate(x, y)` 호출
- `undo()`: `annotation->removeCoordinate(lastIndex)` 호출
- 생성 중인 annotation이 아직 list에 추가되지 않았으므로, annotation 포인터가 유효한지 확인이 필요

### 3.2 CreateAnnotationCommand

Annotation 생성을 완료하고 목록에 추가하는 명령.

```cpp
class CreateAnnotationCommand : public QUndoCommand {
public:
  CreateAnnotationCommand(
    AnnotationWorkstationExtensionPlugin* plugin,
    std::shared_ptr<Annotation> annotation,
    QTreeWidgetItem* treeItem,
    QUndoCommand* parent = nullptr);
  void undo() override;
  void redo() override;
private:
  AnnotationWorkstationExtensionPlugin* _plugin;
  std::shared_ptr<Annotation> _annotation;
  QTreeWidgetItem* _treeItem;
};
```

- `redo()`: annotation을 AnnotationList, scene, tree widget에 추가
- `undo()`: annotation을 tree widget에서 제거, scene에서 숨김, AnnotationList에서 제거
- annotation 데이터는 shared_ptr로 보존하여 redo 가능하게 유지

### 3.3 MoveCoordinateCommand

Vertex를 이동하는 명령. drag 중 연속적인 이동을 하나로 병합.

```cpp
class MoveCoordinateCommand : public QUndoCommand {
public:
  MoveCoordinateCommand(QtAnnotation* annotation, int index,
                        float dx, float dy, QUndoCommand* parent = nullptr);
  int id() const override { return 1; }  // MoveCoordinateCommand 고유 ID
  bool mergeWith(const QUndoCommand* other) override;
  void undo() override;
  void redo() override;
private:
  QtAnnotation* _annotation;
  int _index;
  float _dx, _dy;
};
```

- `redo()`: `annotation->moveCoordinateBy(index, dx, dy)`
- `undo()`: `annotation->moveCoordinateBy(index, -dx, -dy)`
- `mergeWith()`: 같은 index에 대한 연속 MoveCoordinateCommand를 병합하여 delta를 누적
- `id()` 반환값 1은 MoveCoordinateCommand끼리만 병합됨을 보장

### 3.4 InsertCoordinateCommand

기존 edge에 vertex를 삽입하는 명령.

```cpp
class InsertCoordinateCommand : public QUndoCommand {
public:
  InsertCoordinateCommand(QtAnnotation* annotation, int index,
                          float x, float y, QUndoCommand* parent = nullptr);
  void undo() override;
  void redo() override;
private:
  QtAnnotation* _annotation;
  int _index;
  float _x, _y;
};
```

- `redo()`: `annotation->insertCoordinate(index, x, y)`
- `undo()`: `annotation->removeCoordinate(index)` (삽입한 바로 그 위치)

### 3.5 RemoveCoordinateCommand

Vertex를 삭제하는 명령. 삭제 전 좌표를 저장하여 복원 가능.

```cpp
class RemoveCoordinateCommand : public QUndoCommand {
public:
  RemoveCoordinateCommand(QtAnnotation* annotation, int index,
                          QUndoCommand* parent = nullptr);
  void undo() override;
  void redo() override;
private:
  QtAnnotation* _annotation;
  int _index;
  float _x, _y;  // 삭제 전 좌표 저장
};
```

- 생성자에서 `annotation->getCoordinate(index)`로 삭제 전 좌표를 저장
- `redo()`: `annotation->removeCoordinate(index)`
- `undo()`: `annotation->insertCoordinate(index, _x, _y)`

### 3.6 DeleteAnnotationCommand

Annotation 전체를 삭제하는 명령.

```cpp
class DeleteAnnotationCommand : public QUndoCommand {
public:
  DeleteAnnotationCommand(
    AnnotationWorkstationExtensionPlugin* plugin,
    std::shared_ptr<Annotation> annotation,
    QTreeWidgetItem* treeItem,
    QUndoCommand* parent = nullptr);
  void undo() override;
  void redo() override;
private:
  AnnotationWorkstationExtensionPlugin* _plugin;
  std::shared_ptr<Annotation> _annotation;
  QTreeWidgetItem* _treeItem;
  // 복원에 필요한 추가 상태: group 정보, tree 내 위치 등
};
```

- `redo()`: annotation을 tree/scene/list에서 제거
- `undo()`: annotation을 원래 위치에 복원 (삭제 전 상태 그대로)

## 4. QUndoStack 관리

### 소유권
`AnnotationWorkstationExtensionPlugin`이 `QUndoStack` 인스턴스를 소유한다.

```cpp
// AnnotationWorkstationExtensionPlugin.h
QUndoStack* _undoStack;
```

### Stack 비우기 시점
- 새 annotation 파일을 열었을 때: `_undoStack->clear()`
- `onNewImageLoaded()` 콜백에서 처리

### Macro 지원 (vertex drag)
vertex 이동은 여러 mouseMove 이벤트를 하나의 command로 묶어야 한다. 두 가지 방법:
1. **QUndoStack::beginMacro()/endMacro()**: 범위 내의 모든 command를 하나로 묶음
2. **QUndoCommand::mergeWith()**: 같은 ID의 연속 command를 병합 (권장)

`mergeWith()` 방식이 더 유연하므로 MoveCoordinateCommand에 적용한다.

### 최대 history
```cpp
_undoStack->setUndoLimit(100);
```

100단계면 일반적인 annotation 작업에 충분하다. 메모리 사용량이 문제되면 줄일 수 있다.

## 5. UI/단축키

| 동작 | 단축키 | 비고 |
|------|--------|------|
| Undo | `Ctrl+Z` | ShortcutManager에 등록 |
| Redo | `Ctrl+Shift+Z` | ShortcutManager에 등록 |

### QAction 연결
```cpp
QAction* undoAction = _undoStack->createUndoAction(this, tr("&Undo"));
QAction* redoAction = _undoStack->createRedoAction(this, tr("&Redo"));
undoAction->setShortcuts(QKeySequence::Undo);
redoAction->setShortcuts(QKeySequence::Redo);
```

`QUndoStack::createUndoAction()` / `createRedoAction()`은 Qt에서 제공하는 편의 메서드로, 단축키와 메뉴 텍스트를 자동으로 관리한다.

## 6. 적용 범위

### 포함
- 모든 annotation 타입: DOT, POLYGON, SPLINE, POINTSET, MEASUREMENT, RECTANGLE
- 생성, 수정(이동/삽입/삭제 vertex), annotation 삭제

### 제외 (1단계)
- Annotation group 생성/삭제 (annotation 자체에만 집중)
- Annotation 색상 변경 (속성 변경은 별도 스펙 필요)
- 파일 저장/로드 (undo stack에 영향 없음)

### 기타
- 새 파일 로드 시 stack 초기화
- 파일 저장은 undo stack에 영향 없음
- `canClose()`에서 `_undoStack->isClean()`으로 수정 여부 판단 가능

## 7. 파일 변경 목록

### 신규 파일

| 파일 경로 | 설명 |
|-----------|------|
| `annotationplugin/commands/AddCoordinateCommand.h` | 생성 중 point 추가 command |
| `annotationplugin/commands/AddCoordinateCommand.cpp` | 구현 |
| `annotationplugin/commands/CreateAnnotationCommand.h` | Annotation 생성 완료 command |
| `annotationplugin/commands/CreateAnnotationCommand.cpp` | 구현 |
| `annotationplugin/commands/MoveCoordinateCommand.h` | Vertex 이동 command |
| `annotationplugin/commands/MoveCoordinateCommand.cpp` | 구현 |
| `annotationplugin/commands/InsertCoordinateCommand.h` | Edge에 vertex 삽입 command |
| `annotationplugin/commands/InsertCoordinateCommand.cpp` | 구현 |
| `annotationplugin/commands/RemoveCoordinateCommand.h` | Vertex 삭제 command |
| `annotationplugin/commands/RemoveCoordinateCommand.cpp` | 구현 |
| `annotationplugin/commands/DeleteAnnotationCommand.h` | Annotation 전체 삭제 command |
| `annotationplugin/commands/DeleteAnnotationCommand.cpp` | 구현 |

### 수정 파일

| 파일 경로 | 변경 내용 |
|-----------|----------|
| `annotationplugin/CMakeLists.txt` | commands/*.cpp를 소스에 추가 |
| `annotationplugin/AnnotationWorkstationExtensionPlugin.h` | QUndoStack 멤버 추가, undo/redo QAction 추가 |
| `annotationplugin/AnnotationWorkstationExtensionPlugin.cpp` | QUndoStack 초기화, undo/redo 액션을 툴바/메뉴에 추가, finishAnnotation/deleteAnnotation 시 command push, onNewImageLoaded에서 stack clear |
| `annotationplugin/QtAnnotation.h` | coordinate 조작 메서드에 QUndoStack 파라미터 추가 (또는 plugin 참조) |
| `annotationplugin/QtAnnotation.cpp` | moveCoordinateBy 등에서 command push |
| `annotationplugin/AnnotationTool.cpp` | addCoordinate 시 AddCoordinateCommand push |
| `annotationplugin/PolyAnnotationTool.cpp` | edge 더블클릭 시 InsertCoordinateCommand push |

### 의존성
- Qt6::Widgets에 이미 QUndoStack, QUndoCommand가 포함되어 있으므로 추가 의존성 없음

## 8. 구현 순서

의존성을 고려한 순서:

1. **Command 기반 클래스 파일 생성** (6개 .h/.cpp 쌍)
   - AddCoordinateCommand, MoveCoordinateCommand 먼저 (가장 빈번한 연산)
   - InsertCoordinateCommand, RemoveCoordinateCommand 다음
   - CreateAnnotationCommand, DeleteAnnotationCommand 마지막 (plugin 참조 필요)

2. **CMakeLists.txt 업데이트**
   - 새 소스 파일을 add_library에 추가

3. **AnnotationWorkstationExtensionPlugin 수정**
   - QUndoStack 멤버 추가 및 초기화
   - undo/redo QAction 생성 및 툴바 추가
   - finishAnnotation()을 CreateAnnotationCommand로 래핑
   - deleteAnnotation()을 DeleteAnnotationCommand로 래핑

4. **QtAnnotation 수정**
   - moveCoordinateBy()에서 MoveCoordinateCommand push
   - removeCoordinate() 호출부를 RemoveCoordinateCommand로 래핑

5. **AnnotationTool / PolyAnnotationTool 수정**
   - 생성 중 addCoordinate를 AddCoordinateCommand로 래핑
   - edge 더블클릭을 InsertCoordinateCommand로 래핑

6. **ShortcutManager 통합** (선택)
   - Ctrl+Z / Ctrl+Shift+Z 단축키 등록

## 9. 경계면 검증 기준

QA Inspector가 확인해야 할 핵심 경계면:

### 컴파일 검증
- [ ] 모든 Command 클래스가 QUndoCommand를 올바르게 상속
- [ ] override 메서드 시그니처가 기본 클래스와 일치
- [ ] Q_UNDO_STACK 관련 헤더(#include <QUndoStack>, #include <QUndoCommand>) 포함

### 링크 검증
- [ ] annotationplugin CMakeLists.txt에 모든 새 .cpp 파일이 포함
- [ ] Qt6::Widgets 의존성이 이미 있어 추가 불필요한지 확인

### 통합 검증
- [ ] QUndoStack 포인터가 plugin에서 command로 올바르게 전달됨
- [ ] QtAnnotation의 coordinate 조작 메서드가 command를 통해 간접 호출됨
- [ ] undo() 후 annotation 상태가 조작 전과 정확히 일치함
- [ ] redo() 후 annotation 상태가 undo() 전과 정확히 일치함
- [ ] MoveCoordinateCommand.mergeWith()가 같은 vertex의 연속 이동만 병합함
- [ ] 새 이미지 로드 시 undo stack이 clear됨
- [ ] annotation 삭제 후 undo 시 tree widget과 scene이 모두 복원됨

### 메모리 검증
- [ ] shared_ptr<Annotation>이 command에서 올바르게 유지되어 undo 시 접근 가능
- [ ] QUndoStack의 undo limit(100) 초과 시 가장 오래된 command가 올바르게 폐기됨
