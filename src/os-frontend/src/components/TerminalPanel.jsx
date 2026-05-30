import { useEffect, useRef } from 'react'

const NEAR_BOTTOM_PX = 48

export default function TerminalPanel({ logs, status, bridgeOnline, error }) {
  const logRef = useRef(null)
  const prevLogCountRef = useRef(0)

  useEffect(() => {
    const el = logRef.current
    if (!el) return

    const prevCount = prevLogCountRef.current
    const grew = logs.length > prevCount
    prevLogCountRef.current = logs.length

    if (!grew) return

    const distanceFromBottom = el.scrollHeight - el.scrollTop - el.clientHeight
    const wasNearBottom = distanceFromBottom <= NEAR_BOTTOM_PX

    if (wasNearBottom || prevCount === 0) {
      el.scrollTop = el.scrollHeight
    }
  }, [logs])

  return (
    <section className="panel terminal-panel">
      <header className="panel__header">
        <h2 className="panel__title">Simulator output</h2>
        <span className="terminal-panel__status">
          {bridgeOnline ? (
            <>
              <span className={`terminal-panel__dot terminal-panel__dot--${status}`} />
              {status}
            </>
          ) : (
            <span className="terminal-panel__offline">bridge offline</span>
          )}
        </span>
      </header>
      {error && <p className="terminal-panel__error">{error}</p>}
      <pre ref={logRef} className="terminal-panel__log">
        {logs.length === 0 ? (
          <span className="terminal-panel__placeholder">
            Start the simulator to stream os_test.exe output here.
          </span>
        ) : (
          logs.map((entry, i) => (
            <span
              key={`${entry.ts}-${i}`}
              className={
                entry.stream === 'stderr' ? 'terminal-panel__line--err' : undefined
              }
            >
              {entry.text}
              {'\n'}
            </span>
          ))
        )}
      </pre>
    </section>
  )
}
