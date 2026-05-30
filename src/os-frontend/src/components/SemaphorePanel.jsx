import { formatOwner } from '../theme'

const SEMAPHORE_CONFIG = [
  { key: 'userInput', label: 'User Input' },
  { key: 'userOutput', label: 'User Output' },
  { key: 'fileSystem', label: 'File System' },
]

export default function SemaphorePanel({ semaphores }) {
  return (
    <section className="panel semaphore-panel">
      <h2 className="panel__title">Semaphores</h2>
      <div className="semaphore-panel__grid">
        {SEMAPHORE_CONFIG.map(({ key, label }) => {
          const sem = semaphores[key]
          const locked = sem.value === 0
          return (
            <article key={key} className="semaphore-card">
              <h3 className="semaphore-card__name">{label}</h3>
              <p className="semaphore-card__row">
                <span className="semaphore-card__label">Value</span>
                <span className="semaphore-card__value">
                  {sem.value}{' '}
                  <span
                    className={`semaphore-card__status${locked ? ' semaphore-card__status--locked' : ''}`}
                  >
                    ({locked ? 'locked' : 'free'})
                  </span>
                </span>
              </p>
              <p className="semaphore-card__row">
                <span className="semaphore-card__label">Owner</span>
                <span className="semaphore-card__value">
                  {formatOwner(sem.ownerID)}
                </span>
              </p>
            </article>
          )
        })}
      </div>
    </section>
  )
}
