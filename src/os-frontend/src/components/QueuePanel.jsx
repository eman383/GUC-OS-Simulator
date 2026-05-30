import { getProcessStyle } from '../theme'

const QUEUE_CONFIG = [
  { key: 'ready', label: 'Ready Queue' },
  { key: 'inputBlocked', label: 'Input Blocked' },
  { key: 'outputBlocked', label: 'Output Blocked' },
  { key: 'diskBlocked', label: 'Disk Blocked' },
]

export default function QueuePanel({ queues, processes }) {
  return (
    <section className="panel queue-panel">
      <h2 className="panel__title">Scheduler Queues</h2>
      <div className="queue-panel__grid">
        {QUEUE_CONFIG.map(({ key, label }) => {
          const pids = queues[key] ?? []
          return (
            <div key={key} className="queue-panel__queue">
              <h3 className="queue-panel__queue-name">{label}</h3>
              <div className="queue-panel__pills">
                {pids.length === 0 ? (
                  <span className="queue-panel__empty">—</span>
                ) : (
                  pids.map((pid) => {
                    const style = getProcessStyle(processes, pid)
                    return (
                      <span
                        key={pid}
                        className="queue-panel__pill"
                        style={{
                          color: style.color,
                          backgroundColor: style.badgeBg,
                          borderColor: style.color,
                        }}
                      >
                        P{pid}
                      </span>
                    )
                  })
                )}
              </div>
            </div>
          )
        })}
      </div>
    </section>
  )
}
