import React, { useContext } from 'react';
import { GameContext } from '../context/GameContext.jsx';

// TODO: Flesh out controls for difficulty, timer, mines remaining, and auto-flag toggle.
const Toolbar = () => {
  const { state, actions } = useContext(GameContext);

  return (
    <div className="toolbar">
      <button type="button" onClick={actions.resetGame}>
        New Game
      </button>
      <div className="stats">
        <span>Flags: {state.flagsPlaced}</span>
        <span>Timer: {state.elapsedSeconds}s</span>
      </div>
      <label>
        <input
          type="checkbox"
          checked={state.autoMarkEnabled}
          onChange={actions.toggleAutoMark}
        />
        Auto Mark Selection
      </label>
      {/* TODO: Implement difficulty selector, scoreboard, and custom grid sizing. */}
    </div>
  );
};

export default Toolbar;
