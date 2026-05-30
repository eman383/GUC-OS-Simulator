export default function GanttChart({ gantt }) {
  const tickCount = gantt[0]?.slots.length ?? 0
  const ticks = Array.from({ length: tickCount }, (_, i) => i)

  return (
    <section className="panel gantt">
      <h2 className="panel__title">Gantt Chart</h2>
      <div className="gantt__body">
        {gantt.map((row) => (
          <div key={row.pid} className="gantt__row">
            <span className="gantt__label">P{row.pid}</span>
            <div className="gantt__slots">
              {row.slots.map((active, i) => (
                <div
                  key={i}
                  className={`gantt__slot${active ? ' gantt__slot--active' : ''}`}
                />
              ))}
            </div>
          </div>
        ))}
        <div className="gantt__row gantt__row--ticks">
          <span className="gantt__label" />
          <div className="gantt__slots gantt__slots--labels">
            {ticks.map((t) => (
              <span key={t} className="gantt__tick">
                {t}
              </span>
            ))}
          </div>
        </div>
      </div>
    </section>
  )
}
