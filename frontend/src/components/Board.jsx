import React, { useContext, useRef } from 'react';
import { GameContext } from '../context/GameContext.jsx';
import Cell from './Cell.jsx';

const isMouseLikePointer = (event) => {
  const { pointerType } = event;
  return pointerType === 'mouse' || pointerType === 'pen' || pointerType === undefined;
};

const Board = () => {
  const { state, actions } = useContext(GameContext);
  const boardRef = useRef(null);

  const handlePointerDown = (event) => {
    if (!isMouseLikePointer(event) || event.button !== 0) {
      return;
    }
    actions.beginSelection({ event, boardElement: boardRef.current });
  };

  const handlePointerMove = (event) => {
    if (!isMouseLikePointer(event)) {
      return;
    }
    actions.updateSelection({ event, boardElement: boardRef.current });
  };

  const handlePointerUp = (event) => {
    if (!isMouseLikePointer(event)) {
      return;
    }
    actions.endSelection();
  };

  const handlePointerCancel = (event) => {
    if (!isMouseLikePointer(event)) {
      return;
    }
    actions.endSelection({ cancel: true });
  };

  const style = {
    gridTemplateColumns: `repeat(${state.columns}, var(--cell-size))`,
    gridTemplateRows: `repeat(${state.rows}, var(--cell-size))`
  };

  return (
    <div
      className="board"
      ref={boardRef}
      style={style}
      onPointerDown={handlePointerDown}
      onPointerMove={handlePointerMove}
      onPointerUp={handlePointerUp}
      onPointerLeave={handlePointerCancel}
      onPointerCancel={handlePointerCancel}
    >
      {state.cells.map((cell) => (
        <Cell key={cell.id} cell={cell} disabled={state.status !== 'playing'} />
      ))}
    </div>
  );
};

export default Board;
