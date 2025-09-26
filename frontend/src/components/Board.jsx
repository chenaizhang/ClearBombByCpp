import React, { useContext } from 'react';
import { GameContext } from '../context/GameContext.jsx';
import Cell from './Cell.jsx';

// TODO: Implement full board rendering logic, including animations and user feedback.
const Board = () => {
  const { state, actions } = useContext(GameContext);

  return (
    <div
      className="board"
      style={{
        gridTemplateColumns: `repeat(${state.columns}, var(--cell-size))`,
        gridTemplateRows: `repeat(${state.rows}, var(--cell-size))`
      }}
      onMouseDown={actions.beginSelection}
      onMouseUp={actions.endSelection}
      onMouseMove={actions.updateSelection}
    >
      {state.cells.map((cell) => (
        <Cell key={cell.id} cell={cell} />
      ))}
    </div>
  );
};

export default Board;
