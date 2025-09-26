import React, { useContext } from 'react';
import { GameContext } from '../context/GameContext.jsx';

// TODO: Draw selection rectangles that snap to cells and support multi-touch.
const SelectionOverlay = () => {
  const { state } = useContext(GameContext);

  if (!state.selection.active) {
    return null;
  }

  const { start, end } = state.selection;
  const style = {
    left: `${Math.min(start.x, end.x)}px`,
    top: `${Math.min(start.y, end.y)}px`,
    width: `${Math.abs(end.x - start.x)}px`,
    height: `${Math.abs(end.y - start.y)}px`
  };

  return <div className="selection-overlay" style={style} />;
};

export default SelectionOverlay;
