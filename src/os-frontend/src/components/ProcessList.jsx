import { STATE_STYLES } from '../theme'

export default function ProcessList({ processes }) {
  return (
    <section className="panel process-list">
      <h2 className="panel__title">Process List</h2>
      <ul className="process-list__items">
        {processes.map((proc) => {
          const style = STATE_STYLES[proc.state] || STATE_STYLES.FINISHED
          return (
            <li
              key={proc.pid}
              className="process-card"
              style={{
                borderLeftColor: style.color,
                backgroundColor: style.bg,
              }}
            >
              <div className="process-card__top">
                <span className="process-card__pid">PID {proc.pid}</span>
                <span
                  className="process-card__badge"
                  style={{ color: style.color, backgroundColor: style.badgeBg }}
                >
                  {proc.state}
                </span>
              </div>
              <p className="process-card__name">{proc.name}</p>
              <div className="process-card__meta">
                <span>
                  PC: <code>{proc.pc}</code>
                </span>
                <span>
                  Mem: <code>{proc.memLow}–{proc.memHigh}</code>
                </span>
              </div>
            </li>
          )
        })}
      </ul>
    </section>
  )
}
