export const STATE_STYLES = {
  RUNNING: {
    color: '#a78bfa',
    bg: 'rgba(167, 139, 250, 0.08)',
    badgeBg: 'rgba(167, 139, 250, 0.18)',
  },
  BLOCKED: {
    color: '#fb923c',
    bg: 'rgba(251, 146, 60, 0.06)',
    badgeBg: 'rgba(251, 146, 60, 0.18)',
  },
  READY: {
    color: '#2dd4bf',
    bg: 'rgba(45, 212, 191, 0.06)',
    badgeBg: 'rgba(45, 212, 191, 0.18)',
  },
  FINISHED: {
    color: '#4b5563',
    bg: 'transparent',
    badgeBg: 'rgba(75, 85, 99, 0.18)',
  },
}

const PID_FALLBACK = ['#a78bfa', '#fb923c', '#2dd4bf', '#6b7280']

export function getProcessStyle(processes, pid) {
  const proc = processes.find((p) => p.pid === pid)
  if (proc && STATE_STYLES[proc.state]) {
    return STATE_STYLES[proc.state]
  }
  return {
    color: PID_FALLBACK[(pid - 1) % PID_FALLBACK.length],
    bg: 'rgba(255,255,255,0.05)',
    badgeBg: 'rgba(255,255,255,0.1)',
  }
}

export function formatOwner(ownerID) {
  if (ownerID === -1 || ownerID == null) return 'none'
  return `P${ownerID}`
}
