# Mesh Slicing Validation Cases (A~E)

아래 케이스는 공통적으로 **2-shell 가정** 및 **contour 기반 region 구성** 유지 여부를 확인한다.

## Case A: 정중앙 단일 절단 (정상)
- Setup
  - watertight 샘플 메시 1개
  - plane: 모델 중심 통과, 주축과 직교
- Action
  - 단일 slicing 실행
- Expected
  - 폐합 contour loop >= 1
  - shell 2개 분할 성공
- Check
  - invalid loop = 0
  - region 수가 contour 구성과 일치

## Case B: 얕은 각도 절단 (수치 민감)
- Setup
  - 길쭉한 메시 + plane normal이 거의 평행한 edge 다수
- Action
  - 동일 plane로 10회 반복 실행
- Expected
  - contour 수/길이 통계가 반복 간 동일(결정성)
- Check
  - run-to-run diff가 허용 오차 이내
  - epsilon 로그가 기록됨

## Case C: coplanar 면 다수 포함
- Setup
  - plane과 거의 일치하는 triangle band 포함 메시
- Action
  - slicing + region 빌드
- Expected
  - coplanar 버킷 처리 후도 contour loop 폐합 유지
- Check
  - coplanarTris 카운트 > 0
  - region 생성 실패 없음

## Case D: 재절단 시나리오
- Setup
  - Case A 결과를 캐시
  - plane을 소폭 이동해 2차 slicing
- Action
  - re-slicing (정확도 우선/속도 우선 각 1회)
- Expected
  - 두 모드 모두 유효한 contour/region 생성
  - diff 이벤트가 논리적으로 타당
- Check
  - split/merge 이벤트 수동 검토 통과
  - invalid loop = 0

## Case E: 실패 유도 입력
- Setup
  - 자기 교차 가능성이 있는 비정상 메시 또는 끊긴 경계
- Action
  - slicing 실행
- Expected
  - 시스템이 조용히 성공 처리하지 않고 경고/실패를 명시
- Check
  - 오류 코드/로그 메시지 확인
  - 아티팩트에 실패 사유(JSON 또는 텍스트) 저장
