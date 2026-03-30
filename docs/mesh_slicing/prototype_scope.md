# Mesh Slicing Prototype Scope

## 원칙
- 본 프로토타입은 **2-shell + contour 기반 region 구성**을 핵심 전제로 한다.
- MVP는 “반복 가능한 절단/검증/가시화”에 집중하고, 일반화는 Deferred로 미룬다.

## MVP (반드시 구현)
1. 단일 평면 절단
   - 삼각형-평면 교차 계산
   - 절단선 세그먼트 생성 및 contour 연결
2. 2-shell 분리 결과 생성
   - 평면 기준 양/음 영역 분할
   - shell별 기본 통계(삼각형 수, 경계 수)
3. contour 기반 region 구성
   - contour loop를 기준으로 region ID 부여
   - region별 인접/면적(또는 근사치) 집계
4. 재절단 흐름(기초)
   - 이전 결과를 입력으로 다시 절단 가능
   - 누적 오차 완화를 위한 정규화 단계 포함
5. 디버그 뷰
   - 절단면, contour, region 색상 표시
6. 검증 케이스 A~E 실행 가능
   - headless 모드에서 로그/아티팩트 출력

## Deferred (후속 단계)
1. 2-shell 초과 다중 shell 완전 지원
2. 비다양체 자동 복구/홀 브리징 고도화
3. 고급 수치 안정화(적응형 epsilon, interval arithmetic)
4. 대규모 메시 성능 최적화(SIMD, 병렬 contour stitch)
5. CAD급 정확도 export 포맷 확장
6. GUI 기반 시나리오 배치 실행기

## 범위 제외(명시)
- “모든 입력에서 무조건 watertight 보장”은 MVP 요구사항이 아니다.
- 고정 가정(2-shell, contour-region)은 MVP/Deferred 모두에서 삭제/완화하지 않는다.
