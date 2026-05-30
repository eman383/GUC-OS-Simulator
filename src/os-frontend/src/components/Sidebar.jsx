const NAV_ITEMS = ['Processes', 'Memory', 'Scheduler', 'Semaphores']
const ALGORITHMS = ['Round Robin', 'HRRN', 'MLFQ']

export default function Sidebar({
  activeView,
  onViewChange,
  algorithm,
  onAlgorithmChange,
  clock,
  algorithmDisabled = false,
  runningAlgorithm,
}) {
  const showRunning =
    algorithmDisabled && runningAlgorithm && runningAlgorithm !== algorithm

  return (
    <aside className="sidebar">
      <h1 className="sidebar__title">GUC-OS</h1>

      <nav className="sidebar__nav">
        {NAV_ITEMS.map((item) => (
          <button
            key={item}
            type="button"
            className={`sidebar__nav-link${activeView === item ? ' sidebar__nav-link--active' : ''}`}
            onClick={() => onViewChange(item)}
          >
            {item}
          </button>
        ))}
      </nav>

      <fieldset className="sidebar__algo">
        <legend className="sidebar__algo-legend">Algorithm</legend>
        {!algorithmDisabled && (
          <p className="sidebar__algo-note">Selected for next run</p>
        )}
        {ALGORITHMS.map((name) => (
          <label key={name} className="sidebar__radio">
            <input
              type="radio"
              name="algorithm"
              value={name}
              checked={algorithm === name}
              disabled={algorithmDisabled}
              onChange={() => onAlgorithmChange(name)}
            />
            <span className="sidebar__radio-mark" />
            <span>{name}</span>
          </label>
        ))}
        {algorithmDisabled && runningAlgorithm && (
          <p className="sidebar__algo-note sidebar__algo-note--active">
            Running: {runningAlgorithm}
          </p>
        )}
        {showRunning && (
          <p className="sidebar__algo-note">Next run: {algorithm}</p>
        )}
      </fieldset>

      <div className="sidebar__clock-wrap">
        <span className="sidebar__clock-label">System clock</span>
        <span className="sidebar__clock">{clock}</span>
      </div>
    </aside>
  )
}
