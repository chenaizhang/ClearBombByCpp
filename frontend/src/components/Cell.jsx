import React, { useContext } from 'react';
import { GameContext } from '../context/GameContext.jsx';

const Cell = ({ cell, disabled }) => {
  const { actions } = useContext(GameContext);

  const handleClick = (event) => {
    if (event.button !== undefined && event.button !== 0) {
      return;
    }
    if (disabled || cell.state === 'revealed') {
      return;
    }
    actions.revealCell(cell.position);
  };

  const handleContextMenu = (event) => {
    event.preventDefault();
    if (disabled) {
      return;
    }
    actions.flagCell(cell.position);
  };

  const handleMouseDown = (event) => {
    if (disabled) {
      return;
    }
    if (event.button === 1) {
      event.preventDefault();
      actions.chordCell(cell.position);
    }
  };

  const handleDoubleClick = (event) => {
    if (disabled) {
      return;
    }
    if (event.button !== undefined && event.button !== 0) {
      return;
    }
    event.preventDefault();
    actions.chordCell(cell.position);
  };

  const classes = ['cell', `cell--${cell.state}`];
  if (cell.exploded) {
    classes.push('cell--exploded');
  }
  if (disabled && cell.state !== 'revealed') {
    classes.push('cell--disabled');
  }

  let content = '';
  if (cell.state === 'revealed') {
    content = cell.isMine ? '💣' : cell.adjacentMines > 0 ? cell.adjacentMines : '';
  } else if (cell.state === 'flagged') {
    content = '🚩';
  }

  return (
    <button
      type="button"
      className={classes.join(' ')}
      data-row={cell.position.row}
      data-column={cell.position.column}
      onClick={handleClick}
      onContextMenu={handleContextMenu}
      onMouseDown={handleMouseDown}
      onDoubleClick={handleDoubleClick}
      aria-disabled={disabled || cell.state === 'revealed'}
    >
      {content}
    </button>
  );
};

export default Cell;
