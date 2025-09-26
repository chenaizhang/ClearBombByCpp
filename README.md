# ClearBombByCpp

前端採用 React + Vite，後端使用 C++ 以模擬掃雷核心邏輯，並在原生玩法上加上「框選後自動標記確定地雷」的進階功能。以下為目前的檔案骨架與後續待辦。

## 檔案結構

```
.
├── frontend/                  # React 前端專案
│   ├── package.json           # TODO: 補齊專案描述與 lint 指令
│   ├── vite.config.js         # TODO: 調整 dev server proxy 與組建設定
│   ├── public/
│   │   └── index.html         # TODO: 更新 favicon 與 SEO metadata
│   └── src/
│       ├── App.jsx            # TODO: 擴充全域版面與額外 provider
│       ├── main.jsx           # TODO: 依需求掛載 router / i18n 等 provider
│       ├── styles/
│       │   ├── app.css        # TODO: 依設計稿美化整體樣式
│       │   └── board.css      # TODO: 完善各種格子狀態樣式
│       ├── components/
│       │   ├── Board.jsx      # TODO: 真正渲染棋盤並串接事件
│       │   ├── Cell.jsx       # TODO: 彈出動畫、右鍵旗標等互動細節
│       │   ├── GameShell.jsx  # TODO: 完成整體版面排版
│       │   ├── SelectionOverlay.jsx # TODO: 精準對齊格子並支援多種操作裝置
│       │   └── Toolbar.jsx    # TODO: 難度選單、計時器、分數紀錄等
│       ├── context/
│       │   └── GameContext.jsx # TODO: 若導入 TS/PropTypes 補型別驗證
│       ├── hooks/
│       │   └── useMinesweeper.js # TODO: 串接後端 API 與完整 reducer 邏輯
│       └── services/
│           └── apiClient.js   # TODO: 補完錯誤處理、權限、重試策略
├── backend/
│   ├── CMakeLists.txt         # TODO: 加入 HTTP 函式庫與測試設定
│   ├── include/
│   │   ├── ApiServer.hpp      # TODO: 具體化路由與事件回圈
│   │   ├── AutoMarker.hpp     # TODO: 注入演算法參數與策略
│   │   ├── GameEngine.hpp     # TODO: 管理遊戲狀態與統計資訊
│   │   └── MinesweeperBoard.hpp # TODO: 實作鄰居判定與格子資料
│   ├── src/
│   │   ├── ApiServer.cpp      # TODO: 整合網路框架並處理請求
│   │   ├── AutoMarker.cpp     # TODO: 實作框選後的確定地雷推理
│   │   ├── GameEngine.cpp     # TODO: 組合核心互動與游戲流程
│   │   ├── MinesweeperBoard.cpp # TODO: 生成棋盤、計算鄰近地雷、翻格邏輯
│   │   └── main.cpp           # TODO: 解析參數、啟動服務、錯誤處理
│   ├── scripts/
│   │   └── run_dev_server.sh  # TODO: 補齊 build + run 流程
│   └── tests/
│       └── GameEngineTests.cpp # TODO: 選定測試框架並填入測試案例
└── README.md                  # TODO: 隨開發進度更新使用說明與建置流程
```

## 待辦摘要

- **前端資料流**：`useMinesweeper.js` 仍待接上後端 API、完成 reducer case 與時間計算。
- **框選自動標記**：前端 `SelectionOverlay.jsx` 需將框選座標以棋盤格位址傳給後端；後端 `AutoMarker` 需實作確定地雷推理並回傳旗標更新。
- **網路層**：`ApiServer` 目前僅為 stub，需選定 C++ Web framework（Drogon、Crow、Restinio 等）並實作 `/api/board`、`/api/reveal`、`/api/flag`、`/api/auto-mark` 等路由。
- **建置流程**：`backend/CMakeLists.txt` 尚未串接 HTTP 函式庫與測試；前端 `package.json` 也待補 lint/testing script。
- **文件化**：完成後需在 README 加上建置步驟、開發者指南、API 約定與前端開發流程。

## 下一步建議

1. 決定後端 HTTP/WS 框架與資料交換格式，填寫 `ApiServer` 與 `apiClient` 邏輯。
2. 釐清 Minesweeper 遊戲流程（翻格、展開、勝敗判定）並完成 `MinesweeperBoard` 與 `GameEngine` 實作。
3. 在前端完成棋盤資料結構與 reducer，實作框選、計時器與旗標邏輯後串接自動標記。
4. 建立測試環境（例如 GTest + Vite 測試）確保核心演算法與前端互動正確。
