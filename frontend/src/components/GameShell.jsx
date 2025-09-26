import React, { useContext } from 'react';
import Toolbar from './Toolbar.jsx';
import Board from './Board.jsx';
import SelectionOverlay from './SelectionOverlay.jsx';
import { GameContext } from '../context/GameContext.jsx';
import '../styles/board.css';

const statusLabel = (status) => {
  switch (status) {
    case 'victory':
      return 'Victory';
    case 'defeat':
      return 'Game Over';
    default:
      return 'In Progress';
  }
};

const GameShell = () => {
  const { state } = useContext(GameContext);

  return (
    <div className="game-shell">
      <Toolbar />
      <div className="info-banner">
        <span>
          <strong>Flags</strong> {state.flagsRemaining}
        </span>
        <span>
          <strong>Timer</strong> {state.elapsedSeconds}s
        </span>
        <span
          className={`status-message${state.status === 'victory' ? ' status-message--victory' : ''}${
            state.status === 'defeat' ? ' status-message--defeat' : ''
          }`}
        >
          {statusLabel(state.status)}
        </span>
      </div>
      <div className="board-wrapper">
        <Board />
        <SelectionOverlay />
      </div>
    </div>
  );
};

export default GameShell;
