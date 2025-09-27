# ClearBombByCpp

Clear Bomb delivers a browser-based Minesweeper that couples a modern React interface with a handcrafted C++ game engine. Beyond faithfully recreating the original ruleset, it introduces a smart selection workflow: drag across the grid to highlight a region and the backend will flag any mines that are logically certain.

## Architecture Overview

- **Frontend (`frontend/`)** – React (Vite) application rendering the board, tracking UI state, and orchestrating API calls.
- **Backend (`backend/`)** – CMake-driven C++ service exposing a minimal REST interface, generating boards, resolving reveals and flags, and running the certainty-based auto marker.
- **Shared contract** – JSON payloads exchanged over `/api` endpoints. The frontend normalises server responses so UI state remains a flat collection of cells keyed by row/column.

## Project Layout

```
.
├── backend/
│   ├── CMakeLists.txt          # Build definitions for the core library, server binary, and smoke tests
│   ├── include/                # Public headers for the board, engine, auto marker, and server
│   ├── scripts/run_dev_server.sh # Convenience wrapper for configuring, building, and launching the server
│   ├── src/                    # Engine, board, auto-marker, and HTTP server implementations
│   └── tests/GameEngineTests.cpp # Lightweight assertions exercising reset and flag workflows
├── frontend/
│   ├── package.json            # Vite, React, ESLint configuration & scripts
│   ├── src/                    # React components, hooks, context, styling, and API client
│   ├── index.html             # Vite entry point mounting the React bundle
│   └── public/                # Static assets copied as-is to the dev server
└── README.md                   # Project overview, setup, and documentation
```

## Backend Endpoints

| Method | Path          | Description                                |
| ------ | ------------- | ------------------------------------------ |
| GET    | `/api/board`  | Fetch the current board snapshot           |
| POST   | `/api/reset`  | Rebuild the board (optional size payload)  |
| POST   | `/api/reveal` | Reveal a cell and resolve cascades         |
| POST   | `/api/flag`   | Toggle a flag on a cell                    |
| POST   | `/api/auto-mark` | Flag certain mines inside a selection |

All POST payloads accept/return JSON. Cells are described by `row`, `column`, `state`, `adjacentMines`, `isMine` (revealed only), and `exploded` flags, letting the UI update a targeted subset without reloading the entire grid.

## Running the Backend

```bash
cd backend
./scripts/run_dev_server.sh            # Builds and runs on port 8080 by default
./scripts/run_dev_server.sh 9090       # Pass a custom port if desired
```

Smoke tests build automatically when `BUILD_TESTS=ON` (default). Run them with CTest or execute the `clear_bomb_tests` binary directly from the build directory.

## Running the Frontend

```bash
cd frontend
npm install
npm run dev                           # Launches Vite dev server on port 5173
npm run lint                          # ESLint over the React source tree
```

During development, Vite proxies API calls to `http://localhost:8080`. Adjust `frontend/vite.config.js` if you run the backend on another port or host.

## One-Command Development Run

```bash
./run_dev.sh
```

- Starts the C++ backend (default port 8080) and the Vite frontend (default port 5173).
- Pass custom ports with `./run_dev.sh <backend-port> <frontend-port>`.
- Press `Ctrl+C` to stop; the script cleans up both processes before exiting.
- Ensure Node.js, npm, CMake, and a C++20 toolchain are installed locally.

## Deployment From Scratch

The steps below assume a clean workstation with no prior toolchain. They produce a release build of the C++ API server and a static React bundle that you can host with any HTTP server.

### Shared Steps

1. Clone the repository and enter it:

   ```bash
   git clone https://github.com/your-org/ClearBombByCpp.git
   cd ClearBombByCpp
   ```

2. Build the backend in release mode:

   ```bash
   cmake -S backend -B backend/build -DCMAKE_BUILD_TYPE=Release
   cmake --build backend/build --target clear_bomb_server
   ```

   On multi-configuration generators (Visual Studio, Xcode) append `--config Release` to the second command.

3. Build the frontend bundle:

   ```bash
   cd frontend
   npm install
   npm run build
   cd ..
   ```

4. Run the backend server from the project root:

   ```bash
   ./backend/build/clear_bomb_server 8080
   ```

   On Windows the executable lives under `backend\build\Release\clear_bomb_server.exe` when using Visual Studio.

5. Serve the frontend `frontend/dist` directory with your preferred static host (Nginx, Caddy, `npx serve -s dist`, S3 + CDN, etc.). The React app expects the API to be reachable at `/api`, so keep the backend on the same origin or configure a reverse proxy.

### macOS

1. Install Apple Command Line Tools (provides Clang and headers):

   ```bash
   xcode-select --install
   ```

2. Install Homebrew (skip if you already have it) and the required packages:

   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   brew update
   brew install cmake node git
   ```

3. Follow the shared steps above. Run the backend binary with `./backend/build/clear_bomb_server 8080` and serve `frontend/dist` with a static host.

### Linux (Ubuntu/Debian)

1. Install build tools, CMake, and Node.js (using the system packages or NodeSource for newer LTS releases):

   ```bash
   sudo apt update
   sudo apt install -y build-essential cmake curl git
   curl -fsSL https://deb.nodesource.com/setup_lts.x | sudo -E bash -
   sudo apt install -y nodejs
   ```

2. Execute the shared steps. Use `./backend/build/clear_bomb_server 8080` to launch the API server and host `frontend/dist` via your web server of choice (Nginx, Apache, or a process manager like PM2 running `npx serve`).

### Windows 10/11

1. Install the required tooling via `winget` (an elevated PowerShell prompt is recommended):

   ```powershell
   winget install -e --id Git.Git
   winget install -e --id Kitware.CMake
   winget install -e --id OpenJS.NodeJS.LTS
   winget install -e --id Microsoft.VisualStudio.2022.BuildTools --override "--add Microsoft.VisualStudio.Component.VC.Tools.x86.x64"
   ```

2. Open the "x64 Native Tools Command Prompt for VS 2022" (or a PowerShell session with the `vcvars64.bat` environment loaded) and run the shared steps. When building, append `--config Release` to the `cmake --build` command:

   ```powershell
   cmake -S backend -B backend/build -DCMAKE_BUILD_TYPE=Release
   cmake --build backend/build --target clear_bomb_server --config Release
   ```

   The release executable is produced at `backend\build\Release\clear_bomb_server.exe`.

3. Start the backend with:

   ```powershell
   backend\build\Release\clear_bomb_server.exe 8080
   ```

4. Serve the frontend bundle (`frontend\dist`) with IIS Static Content, Nginx for Windows, or a lightweight option such as:

   ```powershell
   cd frontend
   npx serve -s dist -l 4173
   ```

   Update DNS or firewall rules as needed so the static host can reach the backend API at `http://<your-host>:8080/api`.

## Gameplay Enhancements

1. **Difficulty presets** – Beginner, Intermediate, and Expert presets map to classic Minesweeper sizes, and you can introduce new presets via `DIFFICULTY_PRESETS` in `useMinesweeper.js`.
2. **Selection auto marker** – Drag with the primary mouse button (or touch) to highlight a rectangle. Upon release, the backend evaluates revealed neighbours and flags cells that are conclusively mines.
3. **Timer & statistics** – The toolbar surfaces elapsed time, remaining flags, and the current game state (in progress, victory, defeat).

## Development Notes

- The auto-marker currently implements deterministic deductions (neighbour counts that fully match hidden cells). It is structured to accept richer heuristics later.
- HTTP parsing is intentionally lightweight to keep dependencies minimal. If you plan to expose the service publicly, consider swapping in a hardened networking stack.
- The frontend keeps cell state normalised to avoid nested data structures—updating multiple cells relies on mapping API payloads back into a single array keyed by `row-column` identifiers.

Happy sweeping!
