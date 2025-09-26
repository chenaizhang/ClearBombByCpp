import { useCallback, useEffect, useMemo, useReducer, useRef } from 'react';
import {
  autoMarkSelection as autoMarkSelectionRequest,
  fetchBoard,
  flagCell as flagCellRequest,
  resetGame as resetGameRequest,
  revealCell as revealCellRequest
} from '../services/apiClient.js';

const DIFFICULTY_PRESETS = [
  { id: 'beginner', label: 'Beginner (9×9 · 10 mines)', config: { rows: 9, columns: 9, mines: 10 } },
  { id: 'intermediate', label: 'Intermediate (16×16 · 40 mines)', config: { rows: 16, columns: 16, mines: 40 } },
  { id: 'expert', label: 'Expert (16×30 · 99 mines)', config: { rows: 16, columns: 30, mines: 99 } }
];

const PRESET_LOOKUP = new Map(DIFFICULTY_PRESETS.map((preset) => [preset.id, preset.config]));

const emptySelection = {
  active: false,
  startCell: null,
  endCell: null,
  box: null,
  hasDragged: false,
  pointerId: null,
  mode: null
};

const normaliseCell = (cell) => ({
  id: `${cell.row}-${cell.column}`,
  position: { row: cell.row, column: cell.column },
  state: cell.state,
  adjacentMines: cell.adjacentMines,
  isMine: cell.isMine,
  exploded: cell.exploded
});

const mergeCells = (current, updates) => {
  if (!updates.length) {
    return current;
  }
  const replacement = new Map();
  updates.forEach((cell) => {
    replacement.set(cell.id, cell);
  });
  return current.map((cell) => replacement.get(cell.id) ?? cell);
};

const initialState = {
  rows: 0,
  columns: 0,
  mines: 0,
  cells: [],
  flagsRemaining: 0,
  status: 'loading',
  elapsedSeconds: 0,
  autoMarkEnabled: true,
  difficulty: 'intermediate',
  difficultyPresets: DIFFICULTY_PRESETS,
  selection: { ...emptySelection },
  loading: true,
  error: null,
  timerActive: false
};

const reducer = (state, action) => {
  switch (action.type) {
    case 'BOOTSTRAP_START':
      return {
        ...state,
        loading: true,
        status: 'loading',
        error: null,
        elapsedSeconds: 0,
        selection: { ...emptySelection }
      };
    case 'BOOTSTRAP_SUCCESS':
      return {
        ...state,
        rows: action.payload.rows,
        columns: action.payload.columns,
        mines: action.payload.mines,
        cells: action.payload.cells.map(normaliseCell),
        flagsRemaining: action.payload.flagsRemaining,
        status: action.payload.status,
        elapsedSeconds: 0,
        loading: false,
        error: null,
        selection: { ...emptySelection },
        timerActive: false
      };
    case 'BOOTSTRAP_FAILURE':
      return {
        ...state,
        loading: false,
        error: action.payload,
        status: 'error'
      };
    case 'APPLY_CELL_UPDATES': {
      const updates = (action.payload.cells ?? []).map(normaliseCell);
      return {
        ...state,
        cells: state.cells.length ? mergeCells(state.cells, updates) : updates,
        flagsRemaining: action.payload.flagsRemaining ?? state.flagsRemaining,
        status: action.payload.status ?? state.status,
        error: null
      };
    }
    case 'SET_TIMER_ACTIVE':
      if (state.timerActive === action.payload) {
        return state;
      }
      return { ...state, timerActive: action.payload };
    case 'SET_ERROR':
      return { ...state, error: action.payload };
    case 'SELECTION_START':
      return {
        ...state,
        selection: {
          active: true,
          startCell: action.payload.startCell,
          endCell: action.payload.endCell,
          box: action.payload.box,
          hasDragged: false,
          pointerId: action.payload.pointerId ?? null,
          mode: 'drag'
        }
      };
    case 'SELECTION_UPDATE':
      if (!state.selection.active || state.selection.mode !== 'drag') {
        return state;
      }
      return {
        ...state,
        selection: {
          ...state.selection,
          endCell: action.payload.endCell,
          box: action.payload.box,
          hasDragged: state.selection.hasDragged || action.payload.hasDragged
        }
      };
    case 'SELECTION_CLEAR':
      return { ...state, selection: { ...emptySelection } };
    case 'FLASH_SELECTION':
      return {
        ...state,
        selection: {
          active: true,
          startCell: action.payload.startCell,
          endCell: action.payload.endCell,
          box: null,
          hasDragged: false,
          pointerId: null,
          mode: 'flash'
        }
      };
    case 'FLASH_SELECTION_CLEAR':
      if (state.selection.mode !== 'flash') {
        return state;
      }
      return { ...state, selection: { ...emptySelection } };
    case 'TICK':
      if (state.status !== 'playing') {
        return state;
      }
      return { ...state, elapsedSeconds: state.elapsedSeconds + 1 };
    case 'TOGGLE_AUTO_MARK':
      return { ...state, autoMarkEnabled: !state.autoMarkEnabled };
    case 'SET_DIFFICULTY':
      return { ...state, difficulty: action.payload };
    default:
      return state;
  }
};

const clampToBoard = (value, max) => {
  if (value < 0) {
    return 0;
  }
  if (value > max) {
    return max;
  }
  return value;
};

const extractPointerCoordinates = (event) => {
  if ('clientX' in event && 'clientY' in event) {
    return { clientX: event.clientX, clientY: event.clientY };
  }

  const native = event.nativeEvent ?? {};
  if ('clientX' in native && 'clientY' in native) {
    return { clientX: native.clientX, clientY: native.clientY };
  }

  return null;
};

const computeCellFromEvent = (event, boardElement, rows, columns) => {
  if (!boardElement) {
    return null;
  }

  const target = event.target instanceof Element ? event.target.closest('.cell') : null;
  if (target && boardElement.contains(target)) {
    const row = Number(target.getAttribute('data-row'));
    const column = Number(target.getAttribute('data-column'));
    if (Number.isFinite(row) && Number.isFinite(column)) {
      return { row, column };
    }
  }

  const coords = extractPointerCoordinates(event);
  if (!coords) {
    return null;
  }

  const rect = boardElement.getBoundingClientRect();
  const relativeX = coords.clientX - rect.left;
  const relativeY = coords.clientY - rect.top;
  const cellWidth = boardElement.clientWidth / columns;
  const cellHeight = boardElement.clientHeight / rows;
  const column = clampToBoard(Math.floor(relativeX / cellWidth), columns - 1);
  const row = clampToBoard(Math.floor(relativeY / cellHeight), rows - 1);

  return { row, column };
};

const collectNeighbors = (center, rows, columns) => {
  const neighbors = [];
  for (let dr = -1; dr <= 1; dr += 1) {
    for (let dc = -1; dc <= 1; dc += 1) {
      if (dr === 0 && dc === 0) {
        continue;
      }
      const row = center.row + dr;
      const column = center.column + dc;
      if (row < 0 || column < 0 || row >= rows || column >= columns) {
        continue;
      }
      neighbors.push({ row, column });
    }
  }
  return neighbors;
};

const computeSelectionBox = (start, end, boardElement) => {
  if (!boardElement || !start || !end) {
    return null;
  }

  const wrapper = boardElement.parentElement ?? boardElement;
  const wrapperRect = wrapper.getBoundingClientRect();

  const minRow = Math.min(start.row, end.row);
  const maxRow = Math.max(start.row, end.row);
  const minCol = Math.min(start.column, end.column);
  const maxCol = Math.max(start.column, end.column);

  const firstCell = boardElement.querySelector(
    `[data-row="${minRow}"][data-column="${minCol}"]`
  );
  const lastCell = boardElement.querySelector(
    `[data-row="${maxRow}"][data-column="${maxCol}"]`
  );

  if (!firstCell || !lastCell) {
    return null;
  }

  const firstRect = firstCell.getBoundingClientRect();
  const lastRect = lastCell.getBoundingClientRect();

  return {
    left: firstRect.left - wrapperRect.left,
    top: firstRect.top - wrapperRect.top,
    width: lastRect.right - firstRect.left,
    height: lastRect.bottom - firstRect.top
  };
};

const shouldIgnorePointer = (event) => {
  const native = event.nativeEvent ?? {};
  if (native.pointerType && native.pointerType !== 'mouse' && native.pointerType !== 'pen') {
    return true;
  }
  return false;
};

export const useMinesweeper = () => {
  const [state, dispatch] = useReducer(reducer, initialState);
  const flashTimeoutRef = useRef(null);

  const cellLookup = useMemo(() => {
    const map = new Map();
    state.cells.forEach((cell) => {
      map.set(`${cell.position.row}-${cell.position.column}`, cell);
    });
    return map;
  }, [state.cells]);

  const loadBoardSnapshot = useCallback(async () => {
    dispatch({ type: 'BOOTSTRAP_START' });
    try {
      const snapshot = await fetchBoard();
      dispatch({ type: 'BOOTSTRAP_SUCCESS', payload: snapshot });
    } catch (error) {
      dispatch({ type: 'BOOTSTRAP_FAILURE', payload: error.message || 'Unable to load board' });
    }
  }, []);

  useEffect(() => {
    loadBoardSnapshot();
  }, [loadBoardSnapshot]);

  useEffect(() => () => {
    if (flashTimeoutRef.current) {
      clearTimeout(flashTimeoutRef.current);
    }
  }, []);

  useEffect(() => {
    if (state.status !== 'playing' || !state.timerActive) {
      return () => {};
    }

    const interval = setInterval(() => {
      dispatch({ type: 'TICK' });
    }, 1000);

    return () => clearInterval(interval);
  }, [state.status, state.timerActive]);

  const applyUpdates = useCallback((payload) => {
    dispatch({ type: 'APPLY_CELL_UPDATES', payload });
  }, []);

  const autoMarkFromCells = useCallback(
    async (cells) => {
      if (!cells || !cells.length) {
        return;
      }

      const candidateMap = new Map();

      const registerCandidate = (rawCell) => {
        if (!rawCell) {
          return;
        }
        const row = rawCell.position ? rawCell.position.row : rawCell.row;
        const column = rawCell.position ? rawCell.position.column : rawCell.column;
        const stateValue = rawCell.state;
        const adjacentMines = rawCell.adjacentMines ?? 0;
        const isMine = rawCell.isMine ?? false;
        const key = `${row}-${column}`;
        if (!candidateMap.has(key)) {
          candidateMap.set(key, { row, column, state: stateValue, adjacentMines, isMine });
        }
      };

      cells.forEach(registerCandidate);

      const offsets = [-1, 0, 1];
      for (const candidate of Array.from(candidateMap.values())) {
        offsets.forEach((dRow) => {
          offsets.forEach((dCol) => {
            if (dRow === 0 && dCol === 0) {
              return;
            }
            const neighborRow = candidate.row + dRow;
            const neighborColumn = candidate.column + dCol;
            if (
              neighborRow < 0 ||
              neighborColumn < 0 ||
              neighborRow >= state.rows ||
              neighborColumn >= state.columns
            ) {
              return;
            }
            const neighbor = cellLookup.get(`${neighborRow}-${neighborColumn}`);
            if (!neighbor) {
              return;
            }
            registerCandidate({
              row: neighbor.position.row,
              column: neighbor.position.column,
              state: neighbor.state,
              adjacentMines: neighbor.adjacentMines,
              isMine: neighbor.isMine
            });
          });
        });
      }

      const processed = new Set();

      for (const candidate of candidateMap.values()) {
        if (
          candidate.state !== 'revealed' ||
          candidate.isMine ||
          candidate.adjacentMines <= 0
        ) {
          continue;
        }

        const key = `${candidate.row}-${candidate.column}`;
        if (processed.has(key)) {
          continue;
        }
        processed.add(key);

        try {
          const result = await autoMarkSelectionRequest({
            rowBegin: candidate.row,
            rowEnd: candidate.row,
            colBegin: candidate.column,
            colEnd: candidate.column
          });

          if (result && result.flaggedCells && result.flaggedCells.length) {
            applyUpdates({
              cells: result.flaggedCells,
              flagsRemaining: result.flagsRemaining,
              status: result.status
            });
          }
        } catch (error) {
          dispatch({ type: 'SET_ERROR', payload: error.message });
          break;
        }
      }
    },
    [applyUpdates, cellLookup, dispatch, state.columns, state.rows]
  );

  const revealMultipleCells = useCallback(
    async (positions) => {
      if (!positions.length) {
        return { cells: [], flagsRemaining: state.flagsRemaining, status: state.status };
      }
      try {
        const results = await Promise.all(positions.map((position) => revealCellRequest(position)));
        const aggregated = results.reduce(
          (acc, response) => {
            const updated = response?.updatedCells ?? [];
            if (updated.length) {
              acc.cells.push(...updated);
            }
            if (typeof response?.flagsRemaining === 'number') {
              acc.flagsRemaining = response.flagsRemaining;
            }
            if (response?.status) {
              acc.status = response.status;
            }
            return acc;
          },
          { cells: [], flagsRemaining: state.flagsRemaining, status: state.status }
        );
        if (aggregated.cells.length) {
          applyUpdates(aggregated);
        }
        return aggregated;
      } catch (error) {
        dispatch({ type: 'SET_ERROR', payload: error.message });
        return null;
      }
    },
    [applyUpdates, dispatch, state.flagsRemaining, state.status]
  );

  const flashSelection = useCallback(
    (startCell, endCell) => {
      if (flashTimeoutRef.current) {
        clearTimeout(flashTimeoutRef.current);
      }
      dispatch({
        type: 'FLASH_SELECTION',
        payload: { startCell, endCell }
      });
      flashTimeoutRef.current = window.setTimeout(() => {
        dispatch({ type: 'FLASH_SELECTION_CLEAR' });
        flashTimeoutRef.current = null;
      }, 220);
    },
    [dispatch]
  );

  const revealCell = useCallback(
    async (position) => {
      if (state.status !== 'playing') {
        return;
      }
      try {
        const response = await revealCellRequest(position);
        applyUpdates({
          cells: response.updatedCells,
          flagsRemaining: response.flagsRemaining,
          status: response.status
        });

        if (!state.timerActive && response.updatedCells?.length) {
          dispatch({ type: 'SET_TIMER_ACTIVE', payload: true });
        }

        if (state.autoMarkEnabled && response.status === 'playing' && response.updatedCells?.length) {
          await autoMarkFromCells(response.updatedCells);
        }
      } catch (error) {
        dispatch({ type: 'SET_ERROR', payload: error.message });
      }
    },
    [applyUpdates, autoMarkFromCells, state.autoMarkEnabled, state.status, state.timerActive]
  );

  const chordCell = useCallback(
    async (position) => {
      if (state.status !== 'playing') {
        return;
      }
      const centerKey = `${position.row}-${position.column}`;
      const centerCell = cellLookup.get(centerKey);
      if (!centerCell || centerCell.state !== 'revealed') {
        return;
      }

      const neighbors = collectNeighbors(position, state.rows, state.columns);
      let flaggedCount = 0;
      const hiddenTargets = [];
      neighbors.forEach((neighbor) => {
        const key = `${neighbor.row}-${neighbor.column}`;
        const neighborCell = cellLookup.get(key);
        if (!neighborCell) {
          return;
        }
        if (neighborCell.state === 'flagged') {
          flaggedCount += 1;
        } else if (neighborCell.state === 'hidden') {
          hiddenTargets.push(neighbor);
        }
      });

      if (flaggedCount === centerCell.adjacentMines && hiddenTargets.length) {
        const aggregated = await revealMultipleCells(hiddenTargets);
        if (!aggregated) {
          return;
        }
        if (!state.timerActive && aggregated.cells.length) {
          dispatch({ type: 'SET_TIMER_ACTIVE', payload: true });
        }
        if (state.autoMarkEnabled && aggregated.status === 'playing' && aggregated.cells.length) {
          await autoMarkFromCells(aggregated.cells);
        }
      } else {
        const startRow = Math.max(0, position.row - 1);
        const startCol = Math.max(0, position.column - 1);
        const endRow = Math.min(state.rows - 1, position.row + 1);
        const endCol = Math.min(state.columns - 1, position.column + 1);
        flashSelection({ row: startRow, column: startCol }, { row: endRow, column: endCol });
      }
    },
    [
      state.status,
      cellLookup,
      state.rows,
      state.columns,
      flashSelection,
      revealMultipleCells,
      state.timerActive,
      dispatch,
      state.autoMarkEnabled,
      autoMarkFromCells
    ]
  );

  const flagCell = useCallback(
    async (position) => {
      if (state.status === 'loading' || state.status === 'error') {
        return;
      }
      try {
        const response = await flagCellRequest(position);
        applyUpdates({
          cells: [response.updatedCell],
          flagsRemaining: response.flagsRemaining,
          status: response.status
        });
      } catch (error) {
        dispatch({ type: 'SET_ERROR', payload: error.message });
      }
    },
    [applyUpdates, state.status]
  );

  const beginSelection = useCallback(
    ({ event, boardElement }) => {
      if (state.status !== 'playing' || !boardElement) {
        return;
      }

      if (shouldIgnorePointer(event)) {
        return;
      }

      const native = event.nativeEvent ?? {};
      if (typeof native.button === 'number' && native.button !== 0) {
        return;
      }

      const cell = computeCellFromEvent(event, boardElement, state.rows, state.columns);
      if (!cell) {
        return;
      }

      const box = computeSelectionBox(cell, cell, boardElement);
      if (!box) {
        return;
      }

      dispatch({
        type: 'SELECTION_START',
        payload: {
          startCell: cell,
          endCell: cell,
          box,
          pointerId: native.pointerId ?? null
        }
      });
    },
    [state.status, state.rows, state.columns]
  );

  const updateSelection = useCallback(
    ({ event, boardElement }) => {
      if (!state.selection.active || !boardElement) {
        return;
      }

      if (shouldIgnorePointer(event)) {
        return;
      }

      const native = event.nativeEvent ?? {};
      if (typeof native.buttons === 'number' && native.buttons !== 1) {
        return;
      }

      const cell = computeCellFromEvent(event, boardElement, state.rows, state.columns);
      if (!cell) {
        return;
      }

      const hasDragged =
        cell.row !== state.selection.startCell.row ||
        cell.column !== state.selection.startCell.column;

      const box = computeSelectionBox(state.selection.startCell, cell, boardElement);
      if (!box) {
        return;
      }

      dispatch({
        type: 'SELECTION_UPDATE',
        payload: {
          endCell: cell,
          box,
          hasDragged
        }
      });
    },
    [state.selection, state.rows, state.columns]
  );

  const endSelection = useCallback(
    async ({ cancel = false } = {}) => {
      if (!state.selection.active || state.selection.mode !== 'drag') {
        return;
      }

      const selection = state.selection;
      dispatch({ type: 'SELECTION_CLEAR' });

      if (cancel || !state.autoMarkEnabled || !selection.startCell || !selection.endCell) {
        return;
      }

      const rowBegin = Math.min(selection.startCell.row, selection.endCell.row);
      const rowEnd = Math.max(selection.startCell.row, selection.endCell.row);
      const colBegin = Math.min(selection.startCell.column, selection.endCell.column);
      const colEnd = Math.max(selection.startCell.column, selection.endCell.column);

      if (!selection.hasDragged && rowBegin === rowEnd && colBegin === colEnd) {
        const targetCell = cellLookup.get(`${rowBegin}-${colBegin}`);
        if (!targetCell || targetCell.state !== 'revealed' || targetCell.adjacentMines === 0) {
          return;
        }
      }

      try {
        const response = await autoMarkSelectionRequest({ rowBegin, rowEnd, colBegin, colEnd });
        applyUpdates({
          cells: response.flaggedCells,
          flagsRemaining: response.flagsRemaining,
          status: response.status
        });
      } catch (error) {
        dispatch({ type: 'SET_ERROR', payload: error.message });
      }
    },
    [applyUpdates, cellLookup, state.autoMarkEnabled, state.selection]
  );

  const toggleAutoMark = useCallback(() => {
    dispatch({ type: 'TOGGLE_AUTO_MARK' });
  }, []);

  const resetGame = useCallback(
    async (config) => {
      dispatch({ type: 'BOOTSTRAP_START' });
      dispatch({ type: 'SET_TIMER_ACTIVE', payload: false });
      try {
        const effectiveConfig = config ?? PRESET_LOOKUP.get(state.difficulty);
        const snapshot = await resetGameRequest(effectiveConfig ?? undefined);
        dispatch({ type: 'BOOTSTRAP_SUCCESS', payload: snapshot });
      } catch (error) {
        dispatch({ type: 'BOOTSTRAP_FAILURE', payload: error.message || 'Unable to reset game' });
      }
    },
    [state.difficulty]
  );

  const changeDifficulty = useCallback(
    (nextDifficulty) => {
      dispatch({ type: 'SET_DIFFICULTY', payload: nextDifficulty });
      const preset = PRESET_LOOKUP.get(nextDifficulty);
      if (preset) {
        resetGame(preset);
      }
    },
    [resetGame]
  );

  const applyCustomConfig = useCallback(
    async (config) => {
      dispatch({ type: 'SET_DIFFICULTY', payload: 'custom' });
      await resetGame(config);
    },
    [resetGame]
  );

  const actions = useMemo(
    () => ({
      revealCell,
      flagCell,
      beginSelection,
      updateSelection,
      endSelection,
      chordCell,
      resetGame: () => resetGame(),
      toggleAutoMark,
      changeDifficulty,
      applyCustomConfig
    }),
    [
      revealCell,
      flagCell,
      beginSelection,
      updateSelection,
      endSelection,
      chordCell,
      resetGame,
      toggleAutoMark,
      changeDifficulty,
      applyCustomConfig
    ]
  );

  return { state, actions };
};
