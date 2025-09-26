import React, { createContext, useMemo } from 'react';
import { useMinesweeper } from '../hooks/useMinesweeper.js';

export const GameContext = createContext({ state: null, actions: null });
GameContext.displayName = 'GameContext';

export const GameProvider = ({ children }) => {
  const { state, actions } = useMinesweeper();
  const value = useMemo(() => ({ state, actions }), [state, actions]);

  return <GameContext.Provider value={value}>{children}</GameContext.Provider>;
};
