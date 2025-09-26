import React, { useContext } from 'react';
import { GameContext } from '../context/GameContext.jsx';

// TODO: Refine cell rendering (icons, colors, animations) to match Minesweeper look & feel.
const Cell = ({ cell }) => {
  const { actions } = useContext(GameContext);

  const handleClick = (event) => {
    event.preventDefault();
    if (event.type === 'click') {
      actions.revealCell(cell.position);
    }
    if (event.type === 'contextmenu') {
      actions.flagCell(cell.position);
    }
  };

  return (
    <button
      type="button"
      className={`cell cell--${cell.state}`}
      onClick={handleClick}
      onContextMenu={handleClick}
      data-row={cell.position.row}
      data-column={cell.position.column}
    >
      {cell.displayValue}
    </button>
  );
};

export default Cell;
