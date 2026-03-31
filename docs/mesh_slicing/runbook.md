# Mesh Slicing Runbook

## 1) 빌드
- 프로젝트 루트에서 프로토타입 타깃(`viewer`, `validation_runner`)을 빌드한다.
- 디버그/릴리스 산출물은 빌드 디렉터리(`build/`)로 분리한다.

예시:
- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`
- `cmake --build build --target viewer validation_runner -j`

## 2) 실행 (Viewer)
- Viewer는 OpenGL 기반 수동 검증용 실행파일이다.
- 실행 후 plane 조작/fragment 선택/재절단/내보내기 버튼으로 상태를 확인한다.

예시:
- `./build/viewer`

기본 출력 파일(현재 작업 디렉터리 기준):
- `viewer_state.obj`
- `viewer_state.json`

## 3) 헤드리스 검증 (Validation Runner)
- `validation_runner`는 OpenGL 없이 시나리오 A~E를 일괄 실행한다.
- 결과 요약은 stdout에 출력되고, 상세 결과는 JSON/OBJ로 저장된다.

예시:
- `./build/validation_runner`

## 4) 아티팩트 경로
- 기본 출력 루트: `outputs/validation/`
- 시나리오별 출력:
  - `outputs/validation/scenario_a/`
  - `outputs/validation/scenario_b/`
  - `outputs/validation/scenario_c/`
  - `outputs/validation/scenario_d/`
  - `outputs/validation/scenario_e/`
- 각 시나리오 폴더에는 기본적으로 `report.json`이 생성되고, OBJ 덤프가 활성화되어 있으면 `fragments.obj`가 함께 생성된다.

## 5) 제약 및 주의사항
- 고정 가정(2-shell, contour 기반 region 구성)을 완화하지 않는다.
- 비정상 입력은 성공으로 위장하지 말고, 명시적으로 실패 처리한다.
- epsilon 값은 메시 스케일 기반으로 계산하며 하드코딩 상수 남용을 피한다.
- 재절단 결과 비교 시 결정성(동일 입력 동일 결과)을 우선 확인한다.
