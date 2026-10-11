# 로컬 마을 모니터

1. 게임을 빌드하고 마을 모드로 실행한다.
2. 이 폴더의 `start-monitor.cmd`를 실행한다. 현재 PC의 Codex 내장 Python을 우선 사용한다.
   다른 PC에서는 Python 3 설치가 필요할 수 있다.
3. 자동으로 열리는 http://127.0.0.1:8765 에서 확인한다. 서버 종료는 Ctrl+C.
   `py` 명령이 없지만 Python이 설치되어 있다면 `python server.py`를 사용한다.

게임 실행 파일 옆 monitoring-state.json → 로컬 HTTP 서버 /api/state → HTML 화면 순서다.
WebSocket 푸시가 아닌 1초 주기 JSON 폴링이다. 서버는 127.0.0.1에만 바인딩한다.
추가 Python 패키지는 필요하지 않다. HTML을 더블클릭해서 여는 대신 서버로 접속한다.
기본적으로 프로젝트 아래에서 가장 최근 수정된 스냅샷을 선택한다.
여러 게임을 실행하면 `py -3 server.py --source "실행폴더/monitoring-state.json"`으로 지정한다.
포트 충돌 시 `--port 8766` 사용. 서버를 시작해도 게임을 자동 실행하지 않는다.

중앙 마을/북서/북동/남서/남동은 하나의 맵을 나눈 관찰 구역이다.
집 단위 정착지나 독립된 마을 시스템으로 구현한 것이 아니다.
NPC 상태는 역할, 위치, cooldown/stun, 일시정지와 대화 상태에서 산출한다.
이동 여부는 1초 간격 두 위치 차이다. 개별 AI의 내부 목표/모든 행동을 직접 추적하지 않는다.
NPC에는 체력 시스템이 없어 체력 수치는 표시하지 않는다. 몬스터 health는 JSON에 포함한다.
구역별 상태는 주민 수, 산적 수, 살아 있는 몬스터와 재생성 대기 수로 표시한다.
게임 종료/최소화 등으로 스냅샷이 5초 이상 오래되면 '오래된 데이터'가 표시된다.
현재는 마을 모드만 송신한다. --level1에서는 새 스냅샷을 보내지 않는다.

schema_version=1, session_id=프로세스ID, sequence=세션 내 스냅샷 순서.
npcs/monsters/regions/player/writer 항목으로 구성된다.
writer.errors_total, dropped_total, queued는 파일 전송 측의 누적 오류/누락/대기 수다.
실시간 FPS와 GPU 시간의 상세 기록은 기존 profile-*.jsonl을 계속 사용한다.
현재 JSON 저장 버튼으로 AI 분석용 스냅샷을 내려받을 수 있다.
