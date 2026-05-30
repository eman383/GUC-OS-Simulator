const API = '/api'

async function request(path, options = {}) {
  const res = await fetch(`${API}${path}`, {
    headers: { 'Content-Type': 'application/json', ...options.headers },
    ...options,
  })
  if (!res.ok) {
    const err = await res.json().catch(() => ({}))
    throw new Error(err.error || res.statusText)
  }
  return res.json()
}

export function fetchState() {
  return request('/state')
}

export function startSimulator(algorithm) {
  return request('/start', {
    method: 'POST',
    body: JSON.stringify({ algorithm }),
  })
}

export function stopSimulator() {
  return request('/stop', { method: 'POST' })
}

export function submitInput(value) {
  return request('/input', {
    method: 'POST',
    body: JSON.stringify({ value }),
  })
}

export function sendStep() {
  return request('/step', { method: 'POST' })
}

export function setAlgorithm(algorithm) {
  return request('/algorithm', {
    method: 'POST',
    body: JSON.stringify({ algorithm }),
  })
}
