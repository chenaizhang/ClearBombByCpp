import React, { createContext, useMemo } from 'react';
import { useMinesweeper } from '../hooks/useMinesweeper.js';

export const GameContext = createContext();

// TODO: Provide typed context definition if TypeScript or PropTypes are introduced.
export const GameProvider = ({ children }) => {
  const { state, actions } = useMinesweeper();
  const value = useMemo(() => ({ state, actions }), [state, actions]);

  return <GameContext.Provider value={value}>{children}</GameContext.Provider>;
};
