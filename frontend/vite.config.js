import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

// TODO: Adjust Vite configuration (aliases, proxy to backend, env vars) as integration progresses.
export default defineConfig({
  plugins: [react()],
  server: {
    port: 5173,
    proxy: {
      // TODO: Update the proxy target once the C++ backend exposes HTTP endpoints.
      '/api': 'http://localhost:8080'
    }
  }
});
