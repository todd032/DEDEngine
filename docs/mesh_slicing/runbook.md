# Mesh Slicing Runbook

## 1) 빌드
- 프로젝트 루트에서 표준 빌드 파이프라인을 사용한다.
- 디버그 빌드와 릴리스 빌드를 분리해 아티팩트 경로를 관리한다.

예시(환경에 맞게 조정):
- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`
- `cmake --build build -j`

## 2) 실행
- Viewer 실행 시 mesh slicing 실험 플래그를 활성화한다.
- plane 조작 후 수동/자동 slicing으로 결과를 확인한다.

예시:
- `./build/bin/dedengine --scene sample_mesh --feature mesh_slicing`

## 3) 헤드리스 검증
- 시나리오 A~E를 일괄 실행하는 테스트 엔트리를 제공한다.
- headless 모드는 화면 렌더 없이 JSON/로그를 출력한다.

예시:
- `./build/bin/dedengine_tests --mesh-slicing-cases A,B,C,D,E --headless`

## 4) 아티팩트 경로
- 기본 출력 루트: `artifacts/mesh_slicing/`
- 권장 구조:
  - `artifacts/mesh_slicing/logs/`
  - `artifacts/mesh_slicing/json/`
  - `artifacts/mesh_slicing/screenshots/`

## 5) 제약 및 주의사항
- 고정 가정(2-shell, contour 기반 region 구성)을 완화하지 않는다.
- 비정상 입력은 성공으로 위장하지 말고, 명시적으로 실패 처리한다.
- epsilon 값은 메시 스케일 기반으로 계산하며 하드코딩 상수 남용을 피한다.
- 재절단 결과 비교 시 결정성(동일 입력 동일 결과)을 우선 확인한다.
