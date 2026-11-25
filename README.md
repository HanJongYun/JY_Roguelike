# JY_Roguelike (현재 진행 중)

## 프로젝트 소개

게임 「하데스(Hades)」의 방 단위 런 구조에 핵 앤 슬래시 전투를 넣은 3D 쿼터뷰 액션 로그라이크입니다. 축복 시스템은 하데스에서 착안했습니다.

<br>

## 개발 환경

<details>
<summary>펼치기 / 접기</summary>

<br>

| 구분 | 내용 |
|:---:|---|
| 엔진 | Unreal Engine 5.8 |
| 언어 | C++ |
| 사용 기술 | Gameplay Ability System · Mass Entity · MVVM (UMG ViewModel) |
| 네트워크 구조 | 서버·클라 분리 구조 (Play As Client) |

</details>

<br>

## 핵심 기능

### 몬스터 대량 처리 최적화 (액터 → Mass Entity)

<details>
<summary>펼치기 / 접기</summary>

<br>

몬스터를 액터 대신 Mass Entity로 만들어 같은 늑대 500마리에서 프레임 시간을 비교했습니다.

| Mass Entity 500마리 | 액터 500마리 |
|:---:|:---:|
| ![Mass 500마리](Docs/Gifs/mass_500_mass.gif) | ![액터 500마리](Docs/Gifs/mass_500_actor.gif) |

- 왼쪽 위 숫자가 한 프레임에 걸린 시간(Frame)과 게임 스레드 시간(Game).

$\color{#A371F7}{\textbf{\textsf{측정}}}$ (한 프레임에 걸린 시간, 3회 평균)

| 몬스터 수 | Mass Entity | 액터 | 차이 |
|:---:|:---:|:---:|:---:|
| 50 | 9.7ms (103 FPS) | 14.4ms (70 FPS) | 1.5배 |
| 100 | 10.1ms (99 FPS) | 23.3ms (43 FPS) | 2.3배 |
| 200 | 11.0ms (91 FPS) | 41.2ms (24 FPS) | 3.8배 |
| 500 | 13.0ms (77 FPS) | 95.3ms (10 FPS) | 7.3배 |

</details>

<br>

### 몬스터 네트워크 동기화 (Replication)

<details>
<summary>펼치기 / 접기</summary>

<br>

| 클라 1 · 클라 2 (몬스터 10마리) |
|:---:|
| ![몬스터 복제](Docs/Gifs/mass_replication.gif) |

- 서버가 계산한 몬스터 위치를 클라로 복제해, 두 클라가 같은 움직임을 그립니다.

</details>

<br>

### 몬스터 길찾기 (플로우 필드)

<details>
<summary>펼치기 / 접기</summary>

<br>

$\color{#A371F7}{\textbf{\textsf{개요}}}$

- 몬스터마다 플레이어까지 길찾기를 따로 돌리면, $\color{orange}{	extsf{몬스터 수만큼 같은 계산이 반복}}$되므로 비효율적.

$\color{#A371F7}{\textbf{\textsf{구현}}}$

1. 플로우 필드의 칸으로 $\color{orange}{	extsf{네비메쉬 다각형을 그대로 이용}}$ (별도 격자 없음)
2. 플레이어 다각형에서 $\color{orange}{	extsf{거꾸로 퍼지며}}$ 다각형마다 플레이어 쪽으로 넘어갈 문(경계선)과 남은 거리 기록
3. 몬스터는 $\color{orange}{	extsf{자기 다각형의 문으로 이동}}$ → 문에 닿으면 다음 다각형의 문으로. 플레이어와 같은 다각형이면 플레이어에게 직행
4. 표는 플레이어가 움직일 때만 다시 생성 ($\color{orange}{	extsf{몬스터 수와 무관하게 1회}}$)

- 참고: 플로우 필드 = 맵의 칸마다 목표로 가는 다음 방향을 미리 적어 둔 표. 몬스터는 길찾기 없이 자기 칸의 방향만 읽어 이동.

| 플레이어가 움직이면 길 표가 다시 만들어진다 |
|:---:|
| ![플로우 필드](Docs/Gifs/mass_flowfield.gif) |

- $\color{#3FB950}{\textbf{\textsf{초록 다각형}}}$ = 네비메쉬 다각형
- $\color{#E3B341}{\textbf{\textsf{노란 화살표}}}$ = 다각형마다 플레이어 쪽으로 넘어갈 방향
- $\color{#F85149}{\textbf{\textsf{빨간 화살표}}}$ = 몬스터마다 갈 위치

</details>

<br>

### UI (MVVM)

<details>
<summary>펼치기 / 접기</summary>

<br>

화면(위젯)은 뷰모델 값에 바인딩만 하고, 게임 코드는 뷰모델 값만 바꿉니다. 값이 바뀌면 화면이 자동으로 갱신됩니다.

| 타이틀 | 로비 도착 배너 |
|:---:|:---:|
| ![타이틀](Docs/Gifs/ui_01_title.gif) | ![로비 도착 배너](Docs/Gifs/ui_02_lobby_banner.gif) |
| **정지 메뉴** | **영혼의 제단 (메타 강화)** |
| ![정지 메뉴](Docs/Gifs/ui_03_pause_menu.gif) | ![영혼의 제단](Docs/Gifs/ui_04_soul_altar.gif) |
| **기록판** | |
| ![기록판](Docs/Gifs/ui_05_record_board.gif) | |

</details>

<br>

## AI 환경 및 활용 사례

### 환경

<details>
<summary>펼치기 / 접기</summary>

<br>

| 구분 | 내용 |
|:---:|---|
| 도구 | Claude Code |
| 공용 규칙 | - 작업의 큰 틀(방향 · 가이드라인)은 $\color{orange}{\textsf{사람이 먼저 정함}}$<br>- AI는 그 틀 안에서 레퍼런스(엔진 소스 · 공식 레퍼런스 프로젝트)를 확인해 설계안을 제시<br>- $\color{orange}{\textsf{사람이 첨삭해 확정}}$하고, $\color{orange}{\textsf{AI는 확정된 것만 구현}}$<br>- $\color{orange}{\textsf{추측 금지}}$ — 확인 못 한 것은 미확인으로 표시<br>- 커밋은 승인 후 |
| 스킬 | $\color{#2EC4B6}{\textbf{\textsf{설계 · 구현}}}$<br>- 구현 전 레퍼런스 확인 (엔진 소스 · 공식 레퍼런스 프로젝트)<br>- 클래스 · 컴포넌트 · 인터페이스 등 구조 설계 기준<br>- Gameplay Ability System 구현 전 레퍼런스 구조 확인<br>- C++ 코딩 컨벤션<br>- 빌드 · 프로젝트 파일 생성 · 빌드 오류 추출<br>- 디버그 로그 · 상태 추적<br><br>$\color{#2EC4B6}{\textbf{\textsf{설명 · 문서}}}$<br>- 코드 · 엔진 동작 설명 형식<br>- 함수 호출 · 데이터 흐름 추적<br>- 클래스 다이어그램 (PlantUML)<br>- 기능 흐름 HTML 페이지<br>- 전체 구조도 HTML 페이지<br>- 분석 문서 작성<br>- 문서 저장 위치 · 중복 방지<br><br>$\color{#2EC4B6}{\textbf{\textsf{에디터 · 에셋}}}$<br>- 에디터 MCP 호출 절차<br>- Fab 에셋 가져오기<br>- 프로젝트에 쓸 에셋 고르기<br>- 스크린샷 읽기<br>- 설명한 코드를 Visual Studio에 읽는 순서대로 열기 (헤더 · cpp 좌우 배치)<br><br>$\color{#2EC4B6}{\textbf{\textsf{작업 관리}}}$<br>- 진행 문서 기록 · 이어가기<br>- 두 세션 동시 작업 규칙 |
| 훅 | - 세션 시작 — 진행 문서의 다음 할 일을 자동으로 불러옴<br>- 빌드 명령 직전 — 에디터가 켜져 있는지 확인해 알림<br>- 파일 수정 후 — 고친 C++ 소스를 기록<br>- 응답 종료 전 — C++ 소스를 고치고 빌드하지 않았으면 끝내지 못하게 알림<br>- 입력 대기 — 윈도우 알림 |
| 에디터 연동 | - 언리얼 에디터 MCP<br>- 에디터 전용 C++ 툴셋 (StateTree · AnimGraph · Level 등) |
| 진행 관리 | - 기능별 진행 문서에 기록<br>- 결정과 이유 · 지금까지 한 일 · 다음 할 일 · 막힌 문제 |

</details>

<br>

### 활용 사례

<details>
<summary>펼치기 / 접기</summary>

<br>

$\color{#A371F7}{\textbf{\textsf{1. 목표}}}$

공격 판정, 피격 처리, 플레이어 길막이 캡슐 배치처럼 특정 위치 주변의 몬스터가 필요한 상황에서, 수백 마리인 몬스터 전체를 순회하지 않고 주변 몬스터만 검사하도록 최적화합니다.

$\color{#A371F7}{\textbf{\textsf{2. 진행 과정}}}$

| 순&#8288;서 | 사람 (방향 · 결정) | AI (제안 · 조사 · 구현) |
|:---:|---|---|
| 1 | $\color{orange}{\textbf{\textsf{설계 제시}}}$<br>공격 · 피격 · 캡슐 배치 때마다 몬스터 전체를 순회하는 것은 비효율적이므로, 같은 규격의 그리드를 새로 만들어 주변 몬스터만 검사하게 하려는데, 이 방식에 문제가 없는지 검토를 요청. | |
| 2 | | $\color{orange}{\textbf{\textsf{제안}}}$<br>1. 길 찾기에 쓰는 NavMesh 플로우 필드를 쓰는 안.<br>2. 그리드를 새로 만드는 안. |
| 3 | $\color{orange}{\textbf{\textsf{거절}}}$<br>NavMesh 플로우 필드는 칸이 너무 잘게 쪼개져 있고 범위가 일정하지 않아 쓰지 않음.<br><br>$\color{orange}{\textbf{\textsf{결정}}}$<br>같은 규격의 그리드로 나눠, 필요한 위치 주변의 칸만 보기로 결정.<br><br>$\color{orange}{\textbf{\textsf{지시}}}$<br>그리드를 구현하기 전에, Mass에 같은 최적화가 이미 있는지 먼저 찾아볼 것. | |
| 4 | | $\color{orange}{\textbf{\textsf{조사}}}$<br>엔진 소스에서 회피 기능이 그리드를 쓰고 있다는 것을 찾아옴. |
| 5 | $\color{orange}{\textbf{\textsf{확인}}}$<br>그 엔진 코드를 직접 보고 칸이 고정 규격(2.5m)인 것을 확인.<br><br>$\color{orange}{\textbf{\textsf{결정}}}$<br>공격 범위와 몬스터 크기에 적당하므로 이 그리드를 쓰기로 결정.<br><br>$\color{orange}{\textbf{\textsf{지시}}}$<br>공격할 때 범위를 정하고, 그&nbsp;범위에&nbsp;걸친&nbsp;칸에서만&nbsp;몬스터를&nbsp;찾아올&nbsp;것. | |
| 6 | | $\color{orange}{\textbf{\textsf{구현}}}$<br>지시대로 구현하고 빌드. |
| 7 | $\color{orange}{\textbf{\textsf{확인}}}$<br>구현된 코드를 호출 흐름대로 직접 따라가며 확인.<br><br>$\color{orange}{\textbf{\textsf{최종 결정}}}$<br>이 구조로 확정. | |
| 8 | $\color{orange}{\textbf{\textsf{지시}}}$<br>몬스터 수(50 · 100 · 200)별로 전체 순회와 그리드의 처리 시간을 측정할 것.<br>몬스터가 서로 겹치지 않게 흩어진 상태에서 측정할 것. | |
| 9 | | $\color{orange}{\textbf{\textsf{측정}}}$<br>측정용 임시 코드로 두 방식을 각 1000회 실행해 평균 시간을 측정.<br>※ 세부 수치와 스크린샷은 아래 3. 결과의 측정 참고. |
| 10 | $\color{orange}{\textbf{\textsf{확인}}}$<br>측정 결과로 그리드의 처리 시간이 몬스터 수와 상관없이 일정한 것을 확인. | |

$\color{#A371F7}{\textbf{\textsf{3. 결과}}}$

- 그리드를 불필요하게 만들지 않고 회피 그리드 사용.
- 공격 판정 · 피격 처리 · 플레이어 길막이 캡슐 배치에서 몬스터 전체 순회를 없애 최적화.

<br>

$\color{#A371F7}{\textbf{\textsf{측정}}}$ (기본 공격 1타 범위 판정 1회, 1000회 평균)

| 몬스터 수 | 전체 순회 | 회피 그리드 | 차이 |
|:---:|:---:|:---:|:---:|
| 50 | 0.0054ms | 0.0023ms | 2.4배 |
| 100 | 0.0110ms | 0.0025ms | 4.4배 |
| 200 | 0.0265ms | 0.0035ms | 7.6배 |

- 회피 그리드는 몬스터 수와 상관없이 주변 칸의 몬스터만 검사해 시간이 거의 일정.
- 공격 판정 1회 기준이며, 피격 처리 · 캡슐 배치 · 함정 판정까지 합하면 효과가 더 큼.
- 참고: 원활한 테스트를 위해 몬스터를 서로 겹치지 않게 2m 간격으로 배치하고, 움직이지 않게 멈춘 상태에서 측정.

| 50마리 | 100마리 | 200마리 |
|:---:|:---:|:---:|
| ![측정 50마리](Docs/Screenshots/08_bench_50.png) | ![측정 100마리](Docs/Screenshots/09_bench_100.png) | ![측정 200마리](Docs/Screenshots/10_bench_200.png) |

- 스크린샷의 빨간 네모 안이 그 마리 수의 측정 결과. full scan = 전체 순회, grid = 회피 그리드, us = 마이크로초(1000us = 1ms), checked = 검사한 몬스터 수.

<br>

#### $\color{#A371F7}{\textbf{\textsf{그 밖의 사례}}}$

같은 방식으로 진행한 사례 중 일부입니다.

| 상황 | 진행 과정 |
|:---:|---|
| **웨이브 시작 시점** | $\color{#2EC4B6}{\textbf{\textsf{AI}}}$ — 방마다 웨이브 액터를 두고, 맵이 생길 때 액터가 몬스터를 내보내는 구조를 제시.<br><br>$\color{#2EC4B6}{\textbf{\textsf{나}}}$ — 방 이동은 다음 방 맵을 먼저 불러온 뒤, 클라에도 맵이 생기기를 기다렸다가 플레이어를 옮기는 순서임.<br>그래서 맵이 생길 때 몬스터를 내보내면 플레이어가 도착하기 전에 몬스터가 먼저 나온다고 지적.<br><br>$\color{#2EC4B6}{\textbf{\textsf{결과}}}$ — 웨이브는 플레이어가 방에 들어온 뒤 방 관리 서브시스템이 보내는 입장 신호를 받아서만 시작하게 함. |
| **몬스터 피격 · 넉백 상태** | $\color{#2EC4B6}{\textbf{\textsf{AI}}}$ — 넉백 상태와 피격 상태를 따로 두고 구현.<br><br>$\color{#2EC4B6}{\textbf{\textsf{나}}}$ — 둘을 나눈 설계상 이유가 있는지 물음.<br>이유 없이 만든 순서 때문에 나뉜 것임을 확인.<br><br>$\color{#2EC4B6}{\textbf{\textsf{결과}}}$ — 피격 상태 하나로 합침.<br>두 상태가 따로 처리하던 상태 종료 시점 · 길 막기 · 플레이어 밀어내기 · 모션 선택이 한 곳으로 정리됨. |
| **몬스터 스폰 배치** | $\color{#2EC4B6}{\textbf{\textsf{AI}}}$ — 확인 없이 지점마다 몬스터를 자동으로 나누고 원 모양으로 배치하는 방식을 정해 구현.<br><br>$\color{#2EC4B6}{\textbf{\textsf{나}}}$ — 묻지 않고 정한 것을 지적하고 배치 방식을 다시 정함.<br>한 프레임에 몇 마리씩 만들지 같은 세부 수치는 AI에게 맡김.<br><br>$\color{#2EC4B6}{\textbf{\textsf{결과}}}$ — 지점마다 반경을 두고, 그 안에서 벽 밖으로 나가지 않게 배치. |

</details>

<br>

## 클래스 구조도

<details>
<summary>펼치기 / 접기</summary>

<br>

[클래스 구조도 보기](https://hanjongyun.github.io/JY_Roguelike/)

</details>

<br>

## Milestones

<details>
<summary>펼치기 / 접기</summary>

<br>

| 단계 | 목표 | 작업일 | 세부 항목 |
|:---:|:---:|:---:|---|
| **M1** | 기획 · 캐릭터 기반 | 4일 | ✅ 게임 기획<br>✅ 에셋 후보 설정<br>✅ 쿼터뷰 카메라 · 이동<br>✅ GAS 기반 체력 시스템<br>✅ CommonUI 도입 · UI 구조 설계 |
| **M2** | UI 시스템 | 2일 | ✅ CommonUI 기반 화면 층 구성<br>✅ UI 관리자 Subsystem<br>✅ HUD (체력 · 영혼석)<br>✅ Tab 정지 메뉴<br>✅ 확인 팝업<br>✅ MVVM 데이터 바인딩 |
| **M3** | 로비 | 3일 | ✅ 배너 · 토스트 알림<br>✅ 상호작용 시스템<br>✅ 기록판<br>✅ 포털 · 맵 이동<br>✅ 영혼의 제단 (메타 강화 구매 · 환불)<br>✅ 서버 세이브<br>✅ 맵별 HUD 표시 · 커스텀 커서 |
| **M4** | 이동 · 전투 | 4일 | ✅ 이동 애니메이션 · 점프 · 2단 점프 · 대시<br>✅ 타이틀 · 로딩 화면 · 심리스 맵 이동<br>✅ 기본 공격 콤보 (지상 · 공중)<br>✅ 서버 피격 판정 · 피격 반응<br>✅ 스킬 슬롯 · 쿨타임<br>✅ 찌르기 · 회피 · 내려찍기<br>✅ 카메라를 가리는 벽 페이드 |
| **M5** | 던전 | 5일 | ✅ 방 6종 (전투 · 상점 · 보스)<br>✅ 함정 3종<br>✅ 부서지는 오브젝트 5종<br>✅ 레벨 스트리밍 기반 방 전환<br>✅ 무작위 방 순서<br>✅ 방 클리어 · 출구 문 잠금<br>✅ 오브젝트 파괴 연출 |
| **M5.5** | 전투 리팩토링 | 2일 | ✅ 피격 반응 공용 컴포넌트 · 공중 넉백<br>✅ 공격 · 함정 · 폭발 판정 통일<br>✅ 함정 상태이상 (화상 · 독)<br>✅ GA 구조 개편 (공격 공용 부모 · AbilityTask 분리)<br>✅ GameplayEffect 데이터 에셋화 |
| **M6** 🚧 | **기본 몬스터**<br>(M7과 동시 진행 중) | 12일 (진행 중) | ✅ Mass Entity 구조 설계<br>✅ 서버 → 클라 복제<br>✅ 웨이브 스폰 · 등장 예고<br>✅ 길찾기 (플로우 필드)<br>✅ 이동<br>✅ 피격<br>✅ 행동 StateTree (예고 → 추격 ⇄ 대기)<br>✅ 몬스터 그리기 · 상태별 모션 · 위치 보간<br>✅ 몬스터끼리 밀어내기<br>✅ 플레이어 충돌 (밀어내기 · 길 막기)<br>✅ 넉백 (스킬 · 기본 공격 연결)<br>✅ 피격 모션 · 사망 모션<br>⬜ 피격 소리 · 이펙트<br>⬜ 축복 연결<br>⬜ 둘러싸기 · 공격 차례 (보류)<br>⬜ 몬스터 공격<br>⬜ 다음 웨이브 · 방 클리어 연결<br>⬜ 기본 몬스터 4종 · 엘리트 |
| **M7** 🚧 | **축복** | 진행 중 | ✅ 피해 공식<br>✅ 공격 정보 묶음<br>✅ 플레이어 상태이상 (화상 · 독)<br>✅ 가진 축복 — 축복이 공격의 속성 · 상태이상 · 피해에 적용<br>🚧 가진 축복 — 레벨업 · 교체<br>🚧 Mass 적 상태이상 (서버 완료 · 몸 이펙트 보류)<br>⬜ 패시브 · 조합 축복<br>⬜ 축복 연출 · 특수 효과<br>⬜ 3택1 선택 화면<br>⬜ 방 클리어 보상 획득 → 3택1 → 문 열림<br>⬜ HUD 축복 표시<br>⬜ 상태이상 폴리싱 |
| **M8** | 보스 | — | ⬜ 3페이즈 보스 패턴<br>⬜ 보스 HP바 |
| **M9** | 런 루프 | — | ⬜ 문에 다음 방 보상 미리 표시<br>⬜ 사망 · 클리어 처리 → 로비 복귀<br>⬜ 골드 · 상점<br>⬜ 소모품 인벤토리 · 스탯창<br>⬜ 설정 화면<br>⬜ 피해 숫자 |

</details>

<br>

## 진행 상황 참고자료 (스크린샷)

<details>
<summary>펼치기 / 접기</summary>

<br>

| 타이틀 | 로비 도착 배너 |
|:---:|:---:|
| ![타이틀](Docs/Screenshots/01_title.png) | ![로비 도착 배너](Docs/Screenshots/02_lobby_banner.png) |
| **영혼의 제단 (메타 강화)** | **정지 메뉴** |
| ![영혼의 제단](Docs/Screenshots/03_soul_altar.png) | ![정지 메뉴](Docs/Screenshots/04_pause_menu.png) |
| **포털 확인 팝업** | **종료 확인 팝업** |
| ![포털 확인 팝업](Docs/Screenshots/06_portal_confirm.png) | ![종료 확인 팝업](Docs/Screenshots/05_quit_confirm.png) |
| **던전 인게임 (HP 바)** | |
| ![던전 인게임](Docs/Screenshots/07_dungeon_skill.png) | |

</details>
