import { useState } from 'react'
import { getProcessStyle } from '../theme'

const SLOTS_PER_ROW = 20
const TOTAL_SLOTS = 40

function slotTooltip(index, cell, processes) {
  if (cell.type === 'free' || cell.owner == null) {
    return `Slot ${index} · Free · empty`
  }
  const proc = processes.find((p) => p.pid === cell.owner)
  const name = proc ? proc.name : 'unknown'
  return `Slot ${index} · Owner P${cell.owner} · ${name}`
}

export default function MemoryMap({ memory, processes }) {
  const [hovered, setHovered] = useState(null)

  const usedCount = memory.filter((c) => c.type !== 'free').length
  const row1 = memory.slice(0, SLOTS_PER_ROW)
  const row2 = memory.slice(SLOTS_PER_ROW, TOTAL_SLOTS)

  const uniqueOwners = [...new Set(memory.map((c) => c.owner).filter(Boolean))].sort(
    (a, b) => a - b,
  )

  const onDiskProcesses = processes.filter(
    (p) => p.memLow === -1 && p.memHigh === -1 && p.state !== 'FINISHED',
  )

  function renderRow(cells, startIndex) {
    return (
      <div className="memory-map__row">
        <div className="memory-map__labels">
          {cells.map((_, i) => (
            <span key={startIndex + i} className="memory-map__label">
              {startIndex + i}
            </span>
          ))}
        </div>
        <div className="memory-map__slots">
          {cells.map((cell, i) => {
            const index = startIndex + i
            const isFree = cell.type === 'free' || cell.owner == null
            const style = isFree
              ? null
              : getProcessStyle(processes, cell.owner)

            return (
              <div
                key={index}
                className={`memory-map__slot${isFree ? ' memory-map__slot--free' : ''}`}
                style={
                  isFree
                    ? undefined
                    : { backgroundColor: style.color, borderColor: style.color }
                }
                onMouseEnter={() => setHovered({ index, cell })}
                onMouseLeave={() => setHovered(null)}
              />
            )
          })}
        </div>
      </div>
    )
  }

  return (
    <section className="panel memory-map">
      <header className="panel__header">
        <h2 className="panel__title">Memory Map</h2>
        <span className="panel__meta">
          {usedCount} / {TOTAL_SLOTS} slots used
        </span>
      </header>

      <div className="memory-map__grid">
        {renderRow(row1, 0)}
        {renderRow(row2, SLOTS_PER_ROW)}
      </div>

      {hovered && (
        <p className="memory-map__tooltip" role="status">
          {slotTooltip(hovered.index, hovered.cell, processes)}
        </p>
      )}

      <div className="memory-map__legend">
        {uniqueOwners.map((pid) => {
          const style = getProcessStyle(processes, pid)
          return (
            <span key={pid} className="memory-map__legend-item">
              <span
                className="memory-map__legend-swatch"
                style={{ backgroundColor: style.color }}
              />
              P{pid}
            </span>
          )
        })}
        <span className="memory-map__legend-item">
          <span className="memory-map__legend-swatch memory-map__legend-swatch--free" />
          Free
        </span>
      </div>

      {onDiskProcesses.length > 0 && (
        <div className="memory-map__disk">
          <h3 className="memory-map__disk-title">On disk</h3>
          <div className="memory-map__disk-list">
            {onDiskProcesses.map((proc) => (
              <span key={proc.pid} className="memory-map__disk-badge">
                P{proc.pid} — on disk
                {proc.name ? ` (${proc.name})` : ''}
              </span>
            ))}
          </div>
        </div>
      )}
    </section>
  )
}
