# Mesh Slicing Technical Design

## 1. 데이터 모델

### 1.1 기본 엔티티
- `Vertex { id, position }`
- `Triangle { id, v0, v1, v2, normal }`
- `Plane { n, d }` (정규화된 법선 `n`, 식 `dot(n, x) + d = 0`)

### 1.2 절단 산출 엔티티
- `CutEdgeSample`
  - `triId`
  - `p0, p1` (절단 세그먼트 양 끝점)
  - `classMask` (ON_PLANE/STRADDLE 등)
- `ContourLoop`
  - `loopId`
  - `orderedPoints[]`
  - `closed` (폐합 여부)
- `ShellPartition`
  - `shellId` (2-shell 기준 0/1)
  - `positiveTris[]`, `negativeTris[]`, `coplanarTris[]`
- `Region`
  - `regionId`
  - `shellId`
  - `boundaryLoopIds[]` (외곽 + hole)
  - `metrics { approxArea, triCount }`

### 1.3 인덱싱/연결 구조
- 절단 세그먼트 endpoint는 공간 해시(quantized key)로 봉합한다.
- `edgeKey -> [segmentIds]` 멀티맵을 사용해 contour loop를 생성한다.
- 루프 방향성(CCW/CW)은 평면 좌표계 투영 후 signed area로 판단한다.

## 2. 루프/영역 구성

### 2.1 세그먼트 생성
1. 각 triangle vertex의 signed distance 계산.
2. 부호 변화가 있는 edge에서 보간으로 교차점 계산.
3. coplanar triangle은 별도 버킷에 저장(직접 contour로 간주하지 않음).

### 2.2 contour stitch
1. endpoint quantization으로 근접점 병합.
2. degree-2 우선 연결로 단순 loop 복원.
3. 분기(degree>2)는 최소 회전각/최근접 기준 tie-break.
4. 폐합 실패 loop는 invalid로 표기하고 검증 단계에서 실패 처리.

### 2.3 region 빌드 (contour 기반)
- 평면 로컬 2D 좌표계로 contour를 투영.
- 외곽 loop와 hole loop를 winding 규칙으로 그룹화.
- 각 그룹을 하나의 `Region`으로 생성하고 shellId를 연결한다.
- 삼각형 flood-fill은 보조 검증용으로만 사용한다.

## 3. 재절단(Reslicing) 흐름

1. 이전 절단 결과(`ShellPartition`, `ContourLoop`, `Region`)를 캐시에서 복원.
2. 새로운 plane 입력 시:
   - 기존 메시 기준 재평가 모드(정확도 우선)
   - 이전 분할 기준 증분 모드(속도 우선)
3. 결과 생성 후 정규화:
   - 중복 vertex 병합
   - 짧은 세그먼트 제거(길이 < epsilonLen)
   - loop 방향 재정렬
4. 변경점(diff) 산출:
   - 추가/삭제 contour 개수
   - region split/merge 이벤트

## 4. epsilon / robustness

## 4.1 epsilon 정책
- `epsilonDist`: 점-평면 거리 임계값 (예: bbox diag * 1e-6)
- `epsilonLen`: 세그먼트 최소 길이 (예: bbox diag * 1e-7)
- `epsilonMerge`: endpoint 병합 반경 (예: bbox diag * 5e-7)

> 구현 시 절대값 상수 대신 메시 스케일(bbox diagonal) 기반 상대값을 기본으로 한다.

### 4.2 분류 규칙
- `|dist| <= epsilonDist` -> ON_PLANE
- `dist > epsilonDist` -> POSITIVE
- `dist < -epsilonDist` -> NEGATIVE

### 4.3 안정성 가드
- 거의 평행한 edge 교차 계산 시 분모 하한 체크.
- quantization 키 충돌 시 원래 부동소수 좌표로 2차 검증.
- 루프 self-intersection 검출 시 region 생성 중단 + 경고 로그.

### 4.4 결정성(Determinism)
- segment 정렬 키를 `(triId, minEdgeId, maxEdgeId)`로 고정.
- 동일 입력/평면에서 contour loop 순서를 재현 가능하게 유지.
