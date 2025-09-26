import { useCallback, useEffect, useMemo, useReducer } from 'react';

// TODO: Replace temporary constants with values from backend configuration or user settings.
const initialState = {
  rows: 16,
  columns: 16,
  mines: 40,
  cells: [],
  flagsPlaced: 0,
  elapsedSeconds: 0,
  selection: {
    active: false,
    start: { x: 0, y: 0 },
    end: { x: 0, y: 0 }
  },
  autoMarkEnabled: true
};

// TODO: Implement reducer cases for full Minesweeper behavior.
const reducer = (state, action) => {
  switch (action.type) {
    case 'BOOTSTRAP':
      return { ...state, ...action.payload };
    case 'SET_SELECTION':
      return { ...state, selection: action.payload };
    case 'TOGGLE_AUTO_MARK':
      return { ...state, autoMarkEnabled: !state.autoMarkEnabled };
    default:
      return state;
  }
};

export const useMinesweeper = () => {
  const [state, dispatch] = useReducer(reducer, initialState);

  // TODO: Replace with real backend calls to fetch initial board state.
  useEffect(() => {
    const bootstrap = async () => {
      const payload = {
        cells: [],
        flagsPlaced: 0
      };
      dispatch({ type: 'BOOTSTRAP', payload });
    };

    bootstrap();
  }, []);

  const beginSelection = useCallback((event) => {
    const { offsetX, offsetY } = event.nativeEvent;
    dispatch({
      type: 'SET_SELECTION',
      payload: {
        active: true,
        start: { x: offsetX, y: offsetY },
        end: { x: offsetX, y: offsetY }
      }
    });
  }, []);

  const updateSelection = useCallback((event) => {
    if (!state.selection.active) {
      return;
    }
    const { offsetX, offsetY } = event.nativeEvent;
    dispatch({
      type: 'SET_SELECTION',
      payload: {
        ...state.selection,
        end: { x: offsetX, y: offsetY }
      }
    });
  }, [state.selection]);

  const endSelection = useCallback(() => {
    if (!state.selection.active) {
      return;
    }
    dispatch({
      type: 'SET_SELECTION',
      payload: {
        ...state.selection,
        active: false
      }
    });
    // TODO: Send selection coordinates to backend for auto-marking.
  }, [state.selection]);

  const revealCell = useCallback((position) => {
    // TODO: Trigger backend reveal API and update local state with response.
    console.debug('Reveal cell at', position);
  }, []);

  const flagCell = useCallback((position) => {
    // TODO: Trigger backend flag API and update local state with response.
    console.debug('Flag cell at', position);
  }, []);

  const resetGame = useCallback(() => {
    // TODO: Request new game parameters from backend and reset timers.
    console.debug('Reset game requested');
  }, []);

  const toggleAutoMark = useCallback(() => {
    dispatch({ type: 'TOGGLE_AUTO_MARK' });
    // TODO: Persist auto-mark preference if necessary.
  }, []);

  const actions = useMemo(
    () => ({
      beginSelection,
      updateSelection,
      endSelection,
      revealCell,
      flagCell,
      resetGame,
      toggleAutoMark
    }),
    [beginSelection, updateSelection, endSelection, revealCell, flagCell, resetGame, toggleAutoMark]
  );

  return { state, actions };
};
