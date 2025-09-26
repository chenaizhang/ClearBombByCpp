const API_BASE_URL = '/api';

// TODO: Handle authentication, error reporting, retries, and abort controllers.
export const fetchBoard = async () => {
  const response = await fetch(`${API_BASE_URL}/board`);
  if (!response.ok) {
    throw new Error('Failed to fetch board');
  }
  return response.json();
};

export const revealCell = async (position) => {
  const response = await fetch(`${API_BASE_URL}/reveal`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(position)
  });
  if (!response.ok) {
    throw new Error('Failed to reveal cell');
  }
  return response.json();
};

export const flagCell = async (position) => {
  const response = await fetch(`${API_BASE_URL}/flag`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(position)
  });
  if (!response.ok) {
    throw new Error('Failed to flag cell');
  }
  return response.json();
};

export const autoMarkSelection = async (selection) => {
  const response = await fetch(`${API_BASE_URL}/auto-mark`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(selection)
  });
  if (!response.ok) {
    throw new Error('Failed to auto-mark selection');
  }
  return response.json();
};
