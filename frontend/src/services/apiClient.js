const API_BASE_URL = '/api';

const handleResponse = async (response) => {
  if (!response.ok) {
    const message = await response.text();
    throw new Error(message || 'Unexpected API error');
  }
  if (response.status === 204) {
    return null;
  }
  return response.json();
};

export const fetchBoard = async () => {
  const response = await fetch(`${API_BASE_URL}/board`);
  return handleResponse(response);
};

export const revealCell = async (position) => {
  const response = await fetch(`${API_BASE_URL}/reveal`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(position)
  });
  return handleResponse(response);
};

export const flagCell = async (position) => {
  const response = await fetch(`${API_BASE_URL}/flag`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(position)
  });
  return handleResponse(response);
};

export const autoMarkSelection = async (selection) => {
  const response = await fetch(`${API_BASE_URL}/auto-mark`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(selection)
  });
  return handleResponse(response);
};

export const resetGame = async (config) => {
  const response = await fetch(`${API_BASE_URL}/reset`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: config ? JSON.stringify(config) : ''
  });
  return handleResponse(response);
};
