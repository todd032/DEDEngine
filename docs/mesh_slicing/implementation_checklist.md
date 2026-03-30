# Mesh Slicing Implementation Checklist

고정 가정: **2-shell**, **contour 기반 region 구성** (삭제/완화 금지)

## Phase 1 — 요구사항/컨텍스트 고정
- [ ] `context_injection.md` 기준으로 고정 컨텍스트 확인
- [ ] 비목표/범위 제외를 팀 내 합의
- [ ] 검증 케이스 A~E 실행 기준 합의

## Phase 2 — 기하 분류/교차 계산
- [ ] signed distance 분류(POS/NEG/ON) 구현
- [ ] triangle-plane 교차 세그먼트 생성
- [ ] coplanar triangle 버킷 처리

## Phase 3 — Contour Stitch
- [ ] endpoint quantization/병합 구현
- [ ] 세그먼트 연결로 loop 복원
- [ ] invalid loop 검출/리포트

## Phase 4 — Region 구성 (Contour 중심)
- [ ] loop 투영(plane-local 2D) 구현
- [ ] 외곽/hole 그룹화
- [ ] shellId 연결 및 region 메트릭 집계

## Phase 5 — 2-shell 파티셔닝
- [ ] positive/negative/coplanar tri 분리
- [ ] shell 통계 계산
- [ ] 가정 위반 입력 감지(2-shell 불일치)

## Phase 6 — Reslicing 파이프라인
- [ ] 결과 캐시 포맷 정의
- [ ] 정확도 우선/속도 우선 모드 구현
- [ ] diff 이벤트(split/merge) 산출

## Phase 7 — Robustness/Determinism
- [ ] epsilonDist/Len/Merge 스케일 기반 계산
- [ ] self-intersection/분기 tie-break 가드
- [ ] 반복 실행 결정성 테스트 통과

## Phase 8 — Viewer/Runbook/검증 자동화
- [ ] 디버그 오버레이(Geometry/Contour/Region/Robustness)
- [ ] export(JSON/PNG/log bundle) 동작 확인
- [ ] headless A~E 실행 및 아티팩트 저장 확인
