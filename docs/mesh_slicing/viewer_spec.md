# Mesh Slicing Viewer Spec

## 1) 기본 조작
- 카메라: orbit/pan/zoom
- plane gizmo:
  - 이동(translation)
  - 회전(rotation)
  - 스냅(주축 정렬)
- slicing 트리거:
  - 수동 실행 버튼
  - 자동 실행 토글(plane 변경 시)

## 2) 디버그 모드
- `Debug: Geometry`
  - 교차 세그먼트 표시
  - endpoint index 라벨
- `Debug: Contour`
  - loop별 번호
  - loop 방향(CCW/CW) 화살표
- `Debug: Region`
  - region ID 오버레이
  - shell ID(0/1) 표시
- `Debug: Robustness`
  - epsilonDist/epsilonMerge 값 표시
  - invalid loop, self-intersection 경고 배지

## 3) 색상 규약
- Positive side triangles: 파랑 계열
- Negative side triangles: 주황 계열
- Coplanar triangles: 회색
- Contour loop: 노랑(기본), 선택 시 라임
- Invalid loop/오류: 빨강
- Region fill:
  - shell 0: 저채도 파랑 팔레트
  - shell 1: 저채도 주황 팔레트

## 4) Export
- 스냅샷 이미지(PNG)
- 절단 결과 JSON
  - contour loops (점 목록)
  - region 메타데이터(shellId, boundaryLoopIds, metrics)
  - robustness 통계(epsilon, invalid count)
- 로그 번들(zip)
  - 실행 파라미터
  - 검증 체크 결과

## 5) UX 가이드
- 실패 상태는 토스트 + 패널 로그에 동시에 표시.
- 자동 실행 토글이 켜져 있을 때 debounce(예: 150ms) 적용.
- 대용량 메시에서 디버그 라벨 렌더는 거리 기반으로 축약한다.
