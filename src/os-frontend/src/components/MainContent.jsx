import MemoryMap from './MemoryMap'
import ProcessList from './ProcessList'
import GanttChart from './GanttChart'
import QueuePanel from './QueuePanel'
import SemaphorePanel from './SemaphorePanel'

export default function MainContent({ activeView, state }) {
  const { memory, processes, gantt, queues, semaphores } = state

  if (activeView === 'Memory') {
    return (
      <main className="main-content">
        <MemoryMap memory={memory} processes={processes} />
      </main>
    )
  }

  if (activeView === 'Scheduler') {
    return (
      <main className="main-content">
        <div className="main-content__stack">
          <QueuePanel queues={queues} processes={processes} />
          <GanttChart gantt={gantt} />
        </div>
      </main>
    )
  }

  if (activeView === 'Semaphores') {
    return (
      <main className="main-content">
        <SemaphorePanel semaphores={semaphores} />
      </main>
    )
  }

  return (
    <main className="main-content">
      <MemoryMap memory={memory} processes={processes} />
      <div className="main-content__columns">
        <ProcessList processes={processes} />
        <GanttChart gantt={gantt} />
      </div>
    </main>
  )
}
