import React, { useContext } from 'react';
import { GameContext } from '../context/GameContext.jsx';

const Toolbar = () => {
  const { state, actions } = useContext(GameContext);

  const handleDifficultyChange = (event) => {
    actions.changeDifficulty(event.target.value);
  };

  return (
    <div className="toolbar">
      <div className="toolbar-group">
        <select value={state.difficulty} onChange={handleDifficultyChange} aria-label="Difficulty selection">
          {state.difficultyPresets.map((preset) => (
            <option value={preset.id} key={preset.id}>
              {preset.label}
            </option>
          ))}
        </select>
        <button type="button" onClick={() => actions.resetGame()}>
          New Game
        </button>
      </div>
      <div className="toolbar-group">
        <label>
          <input type="checkbox" checked={state.autoMarkEnabled} onChange={actions.toggleAutoMark} />
          Auto Mark Selection
        </label>
      </div>
      {state.error && <span className="status-message status-message--defeat">{state.error}</span>}
    </div>
  );
};

export default Toolbar;
