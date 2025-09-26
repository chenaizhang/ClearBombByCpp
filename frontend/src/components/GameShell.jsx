import React from 'react';
import Toolbar from './Toolbar.jsx';
import Board from './Board.jsx';
import SelectionOverlay from './SelectionOverlay.jsx';
import '../styles/board.css';

// TODO: Replace placeholder layout with polished UI (grid, panels, dialogs, etc.).
const GameShell = () => {
  return (
    <div className="game-shell">
      <Toolbar />
      <div className="board-wrapper">
        <Board />
        <SelectionOverlay />
      </div>
    </div>
  );
};

export default GameShell;
