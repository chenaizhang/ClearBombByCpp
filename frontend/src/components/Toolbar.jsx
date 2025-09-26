import React, { useContext, useEffect, useMemo, useState } from 'react';
import { GameContext } from '../context/GameContext.jsx';

const Toolbar = () => {
  const { state, actions } = useContext(GameContext);
  const [customRows, setCustomRows] = useState('16');
  const [customColumns, setCustomColumns] = useState('16');
  const [customMines, setCustomMines] = useState('40');
  const [customError, setCustomError] = useState('');

  const handleDifficultyChange = (event) => {
    const nextDifficulty = event.target.value;
    actions.changeDifficulty(nextDifficulty);
    if (nextDifficulty !== 'custom') {
      setCustomError('');
    }
  };

  useEffect(() => {
    if (!state.rows || !state.columns || !state.mines) {
      return;
    }
    setCustomRows(String(state.rows));
    setCustomColumns(String(state.columns));
    setCustomMines(String(state.mines));
  }, [state.rows, state.columns, state.mines]);

  const difficultyOptions = useMemo(
    () => [...state.difficultyPresets, { id: 'custom', label: 'Custom' }],
    [state.difficultyPresets]
  );

  const maxMinesAllowed = useMemo(() => {
    const rowsValue = Number.parseInt(customRows, 10);
    const columnsValue = Number.parseInt(customColumns, 10);
    if (Number.isNaN(rowsValue) || Number.isNaN(columnsValue)) {
      return null;
    }
    const totalCells = rowsValue * columnsValue;
    if (totalCells < 3) {
      return null;
    }
    return Math.max(1, totalCells - 2);
  }, [customRows, customColumns]);

  const handleCustomSubmit = async (event) => {
    event.preventDefault();

    const rowsValue = Number.parseInt(customRows, 10);
    const columnsValue = Number.parseInt(customColumns, 10);
    const minesValue = Number.parseInt(customMines, 10);

    if (Number.isNaN(rowsValue) || Number.isNaN(columnsValue) || Number.isNaN(minesValue)) {
      setCustomError('Please enter numeric values for all fields.');
      return;
    }

    if (rowsValue < 2 || rowsValue > 50) {
      setCustomError('Rows must be between 2 and 50.');
      return;
    }

    if (columnsValue < 2 || columnsValue > 50) {
      setCustomError('Columns must be between 2 and 50.');
      return;
    }

    const maxMines = rowsValue * columnsValue - 2;
    if (maxMines < 1 || minesValue < 1 || minesValue > maxMines) {
      setCustomError(`Mines must be between 1 and ${Math.max(1, maxMines)}.`);
      return;
    }

    setCustomError('');
    await actions.applyCustomConfig({ rows: rowsValue, columns: columnsValue, mines: minesValue });
  };

  return (
    <div className="toolbar">
      <div className="toolbar-group">
        <select value={state.difficulty} onChange={handleDifficultyChange} aria-label="Difficulty selection">
          {difficultyOptions.map((preset) => (
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
      {state.difficulty === 'custom' && (
        <div className="toolbar-group">
          <form className="custom-config" onSubmit={handleCustomSubmit}>
            <label>
              Rows
              <input
                type="number"
                min="2"
                max="50"
                value={customRows}
                onChange={(event) => setCustomRows(event.target.value)}
              />
            </label>
            <label>
              Columns
              <input
                type="number"
                min="2"
                max="50"
                value={customColumns}
                onChange={(event) => setCustomColumns(event.target.value)}
              />
            </label>
            <label>
              Mines
              <input
                type="number"
                min="1"
                max={maxMinesAllowed ?? undefined}
                value={customMines}
                onChange={(event) => setCustomMines(event.target.value)}
              />
            </label>
            <button type="submit">Apply Custom</button>
          </form>
          {maxMinesAllowed !== null && (
            <span className="custom-config__hint">Allowed mines range: 1 - {maxMinesAllowed}</span>
          )}
          {customError && <span className="status-message status-message--defeat">{customError}</span>}
        </div>
      )}
      {state.error && <span className="status-message status-message--defeat">{state.error}</span>}
    </div>
  );
};

export default Toolbar;
