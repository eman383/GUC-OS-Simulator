import { useState } from 'react'
import './App.css'
import Sidebar from './components/Sidebar'
import MainContent from './components/MainContent'
import TerminalPanel from './components/TerminalPanel'
import SimulatorControls from './components/SimulatorControls'
import { useSimulator } from './hooks/useSimulator'

function App() {
  const [activeView, setActiveView] = useState('Processes')
  const {
    osState,
    selectedAlgorithm,
    runningAlgorithm,
    logs,
    status,
    awaitingInput,
    awaitingStep,
    bridgeOnline,
    error,
    start,
    stop,
    sendInput,
    step,
    changeAlgorithm,
  } = useSimulator()

  const simRunning = status === 'running'

  return (
    <div className="app">
      <Sidebar
        activeView={activeView}
        onViewChange={setActiveView}
        algorithm={selectedAlgorithm}
        onAlgorithmChange={changeAlgorithm}
        clock={osState.clock}
        algorithmDisabled={simRunning}
        runningAlgorithm={runningAlgorithm}
      />
      <div className="app__main-column">
        <SimulatorControls
          status={status}
          awaitingInput={awaitingInput}
          awaitingStep={awaitingStep}
          onStart={() => start(selectedAlgorithm)}
          onStop={stop}
          onStep={step}
          onSubmitInput={sendInput}
          canStart={bridgeOnline}
        />
        <MainContent activeView={activeView} state={osState} />
        <TerminalPanel
          logs={logs}
          status={status}
          bridgeOnline={bridgeOnline}
          error={error}
        />
      </div>
    </div>
  )
}

export default App
