import React, { useContext, useMemo } from 'react';
import { GameContext } from '../context/GameContext.jsx';

const SelectionOverlay = () => {
  const { state } = useContext(GameContext);

  const box = useMemo(() => {
    const selection = state.selection;
    if (!selection.active) {
      return null;
    }
    if (selection.box) {
      return selection.box;
    }
    if (!selection.startCell || !selection.endCell) {
      return null;
    }

    const board = document.querySelector('.board');
    if (!board) {
      return null;
    }

    const wrapper = board.parentElement ?? board;
    const wrapperRect = wrapper.getBoundingClientRect();

    const minRow = Math.min(selection.startCell.row, selection.endCell.row);
    const maxRow = Math.max(selection.startCell.row, selection.endCell.row);
    const minCol = Math.min(selection.startCell.column, selection.endCell.column);
    const maxCol = Math.max(selection.startCell.column, selection.endCell.column);

    const first = board.querySelector(
      `[data-row="${minRow}"][data-column="${minCol}"]`
    );
    const last = board.querySelector(
      `[data-row="${maxRow}"][data-column="${maxCol}"]`
    );

    if (!first || !last) {
      return null;
    }

    const firstRect = first.getBoundingClientRect();
    const lastRect = last.getBoundingClientRect();

    return {
      left: firstRect.left - wrapperRect.left,
      top: firstRect.top - wrapperRect.top,
      width: lastRect.right - firstRect.left,
      height: lastRect.bottom - firstRect.top
    };
  }, [state.selection]);

  if (!box) {
    return null;
  }

  return <div className="selection-overlay" style={box} />;
};

export default SelectionOverlay;
