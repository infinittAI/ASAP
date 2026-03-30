---
name: implement-plugin
description: ASAP 플러그인 시스템의 세 가지 인터페이스(ToolPluginInterface, ImageFilterPluginInterface, WorkstationExtensionPluginInterface)를 기반으로 새 플러그인을 구현하는 스킬. "플러그인 만들어줘", "새 도구 추가", "필터 플러그인", "확장 플러그인", "워크스테이션 확장" 등 플러그인 관련 요청 시 반드시 이 스킬을 사용할 것. ASAP/ 하위의 annotation/, basictools/, filters/, visualization/ 디렉토리에 있는 기존 플러그인을 참조하거나 새로 만들 때 트리거.
---

# implement-plugin — ASAP 플러그인 구현 스킬

ASAP의 플러그인 아키텍처는 `ASAP/interfaces/interfaces.h`에 정의된 세 가지 인터페이스를 통해 확장된다. 이 스킬은 각 인터페이스의 구조와 올바른 구현 방법을 안내한다.

## ASAP 플러그인 시스템 개요

플러그인은 Qt의 플러그인 메커니즘을 기반으로 동적 공유 라이브러리(.so/.dll)로 빌드된다. ASAP 실행 시 `plugins/` 디렉토리에서 자동으로 로드된다. 각 플러그인은 정확히 하나의 인터페이스를 구현해야 한다.

필수 매크로:
- `Q_PLUGIN_METADATA(IID "ASAP.{InterfaceName}/1.0" FILE "{plugin}.json")`
- `Q_INTERFACES({InterfaceName})`

## 인터페이스 선택 가이드

| 인터페이스 | 용도 | 키 메서드 |
|-----------|------|-----------|
| **ToolPluginInterface** | 마우스/키보드 입력을 처리하는 대화형 도구 | `mousePressEvent`, `mouseMoveEvent`, `mouseReleaseEvent`, `keyPressEvent`, `getToolButton` |
| **ImageFilterPluginInterface** | 이미지 패치 단위 실시간 필터 | `filter(Patch<double>&, QVariant&)`, `getSettingsPanel`, `filterParametersChanged` |
| **WorkstationExtensionPluginInterface** | 전체 워크스테이션 확장 (도구바, 메뉴, 도킹 위젯, 하위 도구 포함) | `initialize`, `getToolBar`, `getMenu`, `getDockWidget`, `getTools`, `onNewImageLoaded` |

### 선택 기준
- 마우스 상호작용이 핵심이면 → **ToolPluginInterface**
- 픽셀 단위 이미지 변환이 핵심이면 → **ImageFilterPluginInterface**
- 복합 UI(도구바 + 설정 패널 + 여러 도구)가 필요하면 → **WorkstationExtensionPluginInterface**

## 파일 구조

새 플러그인은 `ASAP/{category}/{PluginName}/` 디렉토리에 배치한다:

```
ASAP/{category}/{PluginName}/
├── CMakeLists.txt
├── {PluginName}.h
├── {PluginName}.cpp
└── {PluginName}.json    (Qt 플러그인 메타데이터, 빈 JSON 객체 {})
```

`{category}`는 기존 디렉토리 중 선택: `annotation/`, `basictools/`, `filters/`, `visualization/` 또는 새 카테고리 생성.

## CMakeLists.txt 템플릿

모든 플러그인은 공통으로 ASAPLib에 연결한다. 추가 의존성은 인터페이스 종류에 따라 다르다:

```cmake
cmake_minimum_required(VERSION 3.15)

# 의존성은 인터페이스에 따라 추가:
# ToolPluginInterface: ASAPLib, Qt6::Widgets
# ImageFilterPluginInterface: ASAPLib, basicfilters (+ imgproc 필요)
# WorkstationExtensionPluginInterface: ASAPLib, (+ annotation 등 필요 시)

add_library({PluginName} SHARED
  {PluginName}.h
  {PluginName}.cpp
)

target_link_libraries({PluginName}
  ASAPLib
  Qt6::Widgets
  # 추가 의존성...
)

# 플러그인으로 설치
install(TARGETS {PluginName}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION plugins
)
```

## ToolPluginInterface 구현 패턴

사용자의 마우스/키보드 입력을 처리하는 대화형 도구를 만들 때 사용한다. 예: 주석 도구, 측정 도구, 선택 도구.

핵심 구현 포인트:
- `getToolButton()`은 툴바에 표시될 QAction을 반환한다. 아이콘과 텍스트를 설정하라.
- `setActive(true)`가 호출되면 도구가 활성화된 상태이며, 이때만 마우스/키보드 이벤트를 처리한다.
- `_viewer`를 통해 PathologyViewer의 scene, viewport에 접근할 수 있다.
- 이벤트를 처리했으면 `event->accept()`, 무시하면 `event->ignore()`를 호출한다.

```cpp
#ifndef {PLUGINNAME}_H
#define {PLUGINNAME}_H

#include "ASAP/interfaces/interfaces.h"

class {PluginName} : public QObject, public ToolPluginInterface {
  Q_OBJECT
  Q_INTERFACES(ToolPluginInterface)
  Q_PLUGIN_METADATA(IID "ASAP.ToolPluginInterface/1.0" FILE "{pluginname}.json")

public:
  {PluginName}();
  ~{PluginName}();
  std::string name() override;
  void setActive(bool active) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  QAction* getToolButton() override;
};

#endif // {PLUGINNAME}_H
```

## ImageFilterPluginInterface 구현 패턴

이미지 패치를 실시간으로 변환하는 필터를 만들 때 사용한다. 예: 색 분해, 핵 검출, 색 정규화.

핵심 구현 포인트:
- `filter()`는 `Patch<double>` 입력을 받아 `QVariant` 출력을 생성한다. 출력은 `QImage` 또는 `QPixmap`이어야 화면에 표시된다.
- `_filter` 멤버로 `FilterBase` 파생 클래스를持有하여 백그라운드 스레드에서 실행한다.
- `getSettingsPanel()`은 필터 매개변수를 조절할 Qt 위젯을 반환한다.
- `filterParametersChanged()` 시그널을 발생시키면 뷰어가 필터를 다시 적용한다.
- `clone()`은 동일한 설정의 새 인스턴스를 반환해야 한다 (멀티스레드 안전성).

```cpp
#ifndef {PLUGINNAME}_H
#define {PLUGINNAME}_H

#include "ASAP/interfaces/interfaces.h"

class {PluginName} : public QObject, public ImageFilterPluginInterface {
  Q_OBJECT
  Q_INTERFACES(ImageFilterPluginInterface)
  Q_PLUGIN_METADATA(IID "ASAP.ImageFilterPluginInterface/1.0" FILE "{pluginname}.json")

public:
  {PluginName}();
  ~{PluginName}();
  QString name() const override;
  bool initialize(const ImageSource* image) override;
  void filter(const Patch<double>& input, QVariant& output) override;
  ImageFilterPluginInterface* clone() const override;
  QIcon icon() const override;

signals:
  void filterParametersChanged() override;
};

#endif // {PLUGINNAME}_H
```

## WorkstationExtensionPluginInterface 구현 패턴

워크스테이션 전체에 걸쳐 복합 기능을 제공하는 확장을 만들 때 사용한다. 예: 시각화 확장, 워크리스트 브라우저.

핵심 구현 포인트:
- `initialize(viewer)`는 플러그인 로드 후 처음 호출된다. 여기서 UI 컴포넌트를 생성하고 시그널을 연결한다.
- `getTools()`로 하위 ToolPluginInterface들을 제공할 수 있다.
- `onNewImageLoaded()`로 이미지 로드 이벤트에 반응한다.
- `getDockWidget()`, `getToolBar()`, `getMenu()`로 UI 요소를 메인 윈도우에 통합한다.

```cpp
#ifndef {PLUGINNAME}_H
#define {PLUGINNAME}_H

#include "ASAP/interfaces/interfaces.h"

class {PluginName} : public QObject, public WorkstationExtensionPluginInterface {
  Q_OBJECT
  Q_INTERFACES(WorkstationExtensionPluginInterface)
  Q_PLUGIN_METADATA(IID "ASAP.WorkstationExtensionPluginInterface/1.0" FILE "{pluginname}.json")

public:
  {PluginName}();
  ~{PluginName}();
  bool initialize(PathologyViewer* viewer) override;
  QToolBar* getToolBar() override;
  QMenu* getMenu() override;
  QDockWidget* getDockWidget() override;
  std::vector<std::shared_ptr<ToolPluginInterface>> getTools() override;

public slots:
  void onNewImageLoaded(MultiResolutionImage* img, std::string fileName) override;
  void onImageClosed() override;
};

#endif // {PLUGINNAME}_H
```

## 기존 플러그인 참조

구현 전 반드시 기존 플러그인 코드를 읽고 패턴을 파악하라:

| 인터페이스 | 참조 경로 |
|-----------|----------|
| ToolPluginInterface | `ASAP/annotation/AnnotationPlugin.*` (다각형/점/스플라인 도구) |
| ToolPluginInterface | `ASAP/basictools/PanToolPlugin.*`, `ZoomToolPlugin.*` |
| ImageFilterPluginInterface | `ASAP/filters/ColorDeconvolutionFilterPlugin.*` |
| ImageFilterPluginInterface | `ASAP/filters/NucleiDetectionFilterPlugin.*` |
| WorkstationExtensionPluginInterface | `ASAP/visualization/VisualizationWorkstationExtensionPlugin.*` |
| WorkstationExtensionPluginInterface | `ASAP/filters/FilterWorkstationExtensionPlugin.*` |

## JSON 메타데이터 파일

각 플러그인 디렉토리에 빈 JSON 파일이 필요하다:

```json
{}
```

Qt 플러그인 시스템에서 요구하는 메타데이터 파일이다. 내용은 비어 있어도 되지만 파일 자체는 존재해야 한다.

## 의존성 매트릭스

| 인터페이스 | 필수 의존성 | 선택적 의존성 |
|-----------|------------|--------------|
| ToolPluginInterface | ASAPLib, Qt6::Widgets | annotation (주석 도구 시) |
| ImageFilterPluginInterface | ASAPLib, basicfilters | OpenCV (고급 필터 시) |
| WorkstationExtensionPluginInterface | ASAPLib | annotation, multiresolutionimageinterface |

ASAPLib에 의해 core, multiresolutionimageinterface, Qt6::Core, Qt6::Widgets가 전이적으로 포함된다.
