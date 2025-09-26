import React from 'react';
import { GameProvider } from './context/GameContext.jsx';
import GameShell from './components/GameShell.jsx';

// TODO: Expand layout with header/footer and responsive design tweaks.
const App = () => {
  return (
    <GameProvider>
      <GameShell />
    </GameProvider>
  );
};

export default App;
