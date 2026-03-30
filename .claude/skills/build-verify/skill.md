---
name: build-verify
description: ASAP 프로젝트의 CMake 빌드 구성, 컴파일, 링크, 테스트 실행을 수행하고 결과를 검증하는 스킬. "빌드해줘", "컴파일해줘", "테스트 실행", "빌드 확인", "에러 확인", "CMake 구성" 등 빌드 및 테스트 관련 요청 시 반드시 이 스킬을 사용할 것. 코드 변경 후 빌드 성공 여부를 확인하거나 테스트를 실행할 때 트리거.
---

# build-verify — ASAP 빌드 및 검증 스킬

ASAP의 CMake 기반 빌드 시스템을 구성하고 실행하여 코드 변경이 정상적으로 빌드되는지 검증한다.

## 빌드 디렉토리

ASAP는 in-source 빌드를 지원하지 않는다. 빌드 디렉토리를 별도로 생성해야 한다.

```bash
# 프로젝트 루트에서
mkdir -p build && cd build
```

## CMake 구성

### 전체 빌드 (모든 옵션 활성화)

```bash
cmake .. \
  -DBUILD_ASAP=ON \
  -DBUILD_IMAGEPROCESSING=ON \
  -DBUILD_EXECUTABLES=ON \
  -DBUILD_TESTS=ON \
  -DCMAKE_BUILD_TYPE=Release
```

### 부분 빌드 (GUI 없이 라이브러리만)

```bash
cmake .. \
  -DBUILD_ASAP=OFF \
  -DBUILD_IMAGEPROCESSING=OFF \
  -DBUILD_EXECUTABLES=OFF \
  -DBUILD_TESTS=OFF \
  -DCMAKE_BUILD_TYPE=Release
```

### 새 플러그인 추가 후 재구성

새 플러그인의 CMakeLists.txt가 추가된 경우, CMake를 재실행하여 타겟을 인식시킨다:

```bash
cmake .. -DBUILD_ASAP=ON -DBUILD_IMAGEPROCESSING=ON -DCMAKE_BUILD_TYPE=Release
```

## 빌드 실행

```bash
# 전체 빌드
cmake --build . --parallel $(nproc)

# 특정 타겟만 빌드
cmake --build . --target {PluginName} --parallel $(nproc)
```

## 테스트 실행

```bash
# 전체 테스트
ctest --output-on-failure

# 특정 테스트만
ctest -R {test_name} --output-on-failure
```

## 빌드 검증 체크리스트

CMake 구성 후 다음 항목을 확인한다:

1. **의존성 해결**: 모든 `find_package`가 성공했는지 확인. 실패한 패키지가 있으면 설치 방법을 안내한다.
2. **타겟 등록**: `cmake --build . --target help`로 새 타겟이 등록되었는지 확인.
3. **컴파일 에러**: C++17 호환성, 누락된 include, 타입 불일치를 확인.
4. **링크 에러**: undefined reference는 CMakeLists.txt의 `target_link_libraries` 누락이 원인인 경우가 많다.
5. **플러그인 메타데이터**: `.json` 파일이 소스 디렉토리에 존재하는지 확인.

## 일반적인 빌드 문제와 해결

| 문제 | 원인 | 해결 |
|------|------|------|
| `undefined reference to vtable` | 가상 메서드가 선언되었지만 구현되지 않음 | .cpp에 모든 순수 가상 메서드 구현 추가 |
| `undefined reference to ...` | CMakeLists.txt에 소스 파일 또는 의존성 누락 | `add_library`에 .cpp 추가 또는 `target_link_libraries`에 라이브러리 추가 |
| `cannot find -l...` | 의존 라이브러리 미설치 | 해당 라이브러리 설치 (apt install 등) |
| `fatal error: ...: No such file` | include 경로 누락 | CMakeLists.txt의 `target_include_directories` 확인 |
| `Q_PLUGIN_METADATA` 오류 | .json 파일 누락 | 플러그인 디렉토리에 빈 `{name}.json` 파일 생성 |
| CMake 변경 미반영 | 캐시된 CMake 구성 | 빌드 디렉토리의 `CMakeCache.txt` 삭제 후 재구성 |

## 빌드 결과 보고 형식

```
## 빌드 결과

### CMake 구성
- 상태: SUCCESS / FAIL
- 활성화된 옵션: BUILD_ASAP=ON, BUILD_IMAGEPROCESSING=ON, ...
- 경고: (있으면 나열)

### 컴파일
- 상태: SUCCESS / FAIL
- 에러: (있으면 파일:줄번호와 메시지)
- 경고: (있으면 나열)

### 링크
- 상태: SUCCESS / FAIL
- 미해결 심볼: (있으면 나열)

### 테스트
- 총 테스트 수: N
- 통과: M
- 실패: K
- 실패 상세: (테스트명 + 에러 메시지)
```
