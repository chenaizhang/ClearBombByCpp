import React from 'react';
import { GameProvider } from './context/GameContext.jsx';
import GameShell from './components/GameShell.jsx';

const App = () => {
  return (
    <GameProvider>
      <div className="app-frame">
        <header className="app-header">
          <h1>Clear Bomb</h1>
          <p>Selection-aware Minesweeper powered by a C++ backend.</p>
        </header>
        <GameShell />
      </div>
    </GameProvider>
  );
};

export default App;
