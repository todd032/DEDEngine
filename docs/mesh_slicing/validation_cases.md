# Mesh Slicing Validation Cases (고정 5개 시나리오)

아래 시나리오는 `scenario_runner`의 preset과 1:1로 매칭되며, 검증 기준은 **총면적 비교**가 아니라 **surface-type 조합 규칙** 중심으로 정의한다.

## Case A: `InitialVisibility`
- Setup
  - 2-shell procedural cube(Outer/Inner) 생성
  - 절단 plane 적용 없음
- Action
  - 초기 fragment 상태를 그대로 수집
- Expected / Check
  - 외부 관찰 가능한 경계(surface-type 기준)에서 `OuterSurface`만 노출
  - `CutSurface == 0` 및 `CutCap == 0`

## Case B: `VerticalHalfCut`
- Setup
  - 모델 중심을 통과하는 수직 절단 plane 1개(X축 법선)
- Action
  - plane 1회 적용
- Expected / Check
  - 절단면에서 `CutSurface`(피부 경계)와 `CutCap`(속살)이 동시에 존재
  - 즉, 집계 기준으로 `CutSurface > 0` AND `CutCap > 0`

## Case C: `VerticalThenHorizontalCut`
- Setup
  - 1차 수직 절단 plane(X축 법선)
  - 2차 수평 절단 plane(Y축 법선)
- Action
  - plane을 순차 적용(2-step)
- Expected / Check
  - 2차 절단 이후에도 절단 경계가 있는 각 fragment는
    - `CutSurface > 0` 이면 `CutCap > 0`
    - `CutCap > 0` 이면 `CutSurface > 0`
  - 즉, 절단면 조합 규칙이 fragment 단위로 유지

## Case D: `ProgressiveSkinRemoval`
- Setup
  - +X 방향에서 안쪽으로 진입하는 다단계 수직 plane 시퀀스
- Action
  - 동일 방향 plane을 단계적으로 적용하여 skin 영역을 반복 제거
- Expected / Check
  - 특정 fragment/영역에서 `OuterSurface`와 `CutSurface` 면적이 0에 수렴(≈0)
  - 검증은 `OuterSurface <= epsilon` AND `CutSurface <= epsilon` fragment 존재 여부로 수행

## Case E: `ShallowCutNoMeatExposure`
- Setup
  - outer shell만 교차하고 inner shell은 교차하지 않는 얕은 수직 plane 1개
- Action
  - plane 1회 적용
- Expected / Check
  - inner shell 미교차 조건에서 모든 결과 fragment에 대해 `CutCap == 0`
  - aggregate/fragment 단위 모두 `CutCap` 비노출을 확인
