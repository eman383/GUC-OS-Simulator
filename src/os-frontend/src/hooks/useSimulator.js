import { useCallback, useEffect, useRef, useState } from 'react'
import {
  fetchState,
  sendStep,
  setAlgorithm as apiSetAlgorithm,
  startSimulator,
  stopSimulator,
  submitInput,
} from '../api/simulatorApi'
import { initialState } from '../data/initialState'

const POLL_MS = 400
const DEFAULT_ALGORITHM = 'Round Robin'

export function useSimulator() {
  const [osState, setOsState] = useState(initialState)
  const [selectedAlgorithm, setSelectedAlgorithm] = useState(DEFAULT_ALGORITHM)
  const [logs, setLogs] = useState([])
  const [status, setStatus] = useState('idle')
  const [awaitingInput, setAwaitingInput] = useState(false)
  const [awaitingStep, setAwaitingStep] = useState(false)
  const [awaitingAlgorithm, setAwaitingAlgorithm] = useState(false)
  const [bridgeOnline, setBridgeOnline] = useState(false)
  const [error, setError] = useState(null)
  const pollRef = useRef(null)

  const applyPayload = useCallback((data) => {
    if (data.state) {
      setOsState(data.state)
    }
    if (data.logs) setLogs(data.logs)
    if (data.status) setStatus(data.status)
    setAwaitingInput(!!data.awaitingInput)
    setAwaitingStep(!!data.awaitingStep)
    setAwaitingAlgorithm(!!data.awaitingAlgorithm)
  }, [])

  const refresh = useCallback(async () => {
    try {
      const data = await fetchState()
      applyPayload(data)
      setBridgeOnline(true)
      setError(null)
    } catch (e) {
      setBridgeOnline(false)
      setError(e.message)
    }
  }, [applyPayload])

  useEffect(() => {
    refresh()
    pollRef.current = setInterval(refresh, POLL_MS)
    return () => clearInterval(pollRef.current)
  }, [refresh])

  const start = useCallback(
    async (algorithm) => {
      const choice = algorithm || selectedAlgorithm
      try {
        const data = await startSimulator(choice)
        applyPayload(data)
        setError(null)
      } catch (e) {
        setError(e.message)
      }
    },
    [applyPayload, selectedAlgorithm],
  )

  const stop = useCallback(async () => {
    try {
      const data = await stopSimulator()
      applyPayload(data)
    } catch (e) {
      setError(e.message)
    }
  }, [applyPayload])

  const sendInput = useCallback(
    async (value) => {
      try {
        const data = await submitInput(value)
        applyPayload(data)
      } catch (e) {
        setError(e.message)
      }
    },
    [applyPayload],
  )

  const step = useCallback(async () => {
    try {
      const data = await sendStep()
      applyPayload(data)
    } catch (e) {
      setError(e.message)
    }
  }, [applyPayload])

  const changeAlgorithm = useCallback(
    async (algorithm) => {
      setSelectedAlgorithm(algorithm)
      if (bridgeOnline && status !== 'running') {
        try {
          await apiSetAlgorithm(algorithm)
        } catch {
          /* bridge will receive choice on Start */
        }
      }
    },
    [bridgeOnline, status],
  )

  return {
    osState,
    selectedAlgorithm,
    runningAlgorithm: osState.algorithm,
    logs,
    status,
    awaitingInput,
    awaitingStep,
    awaitingAlgorithm,
    bridgeOnline,
    error,
    start,
    stop,
    sendInput,
    step,
    changeAlgorithm,
  }
}
