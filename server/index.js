import express from 'express'
import cors from 'cors'
import { SimulatorBridge } from './simulator.js'

const PORT = process.env.PORT || 3001
const app = express()
const bridge = new SimulatorBridge()

app.use(cors())
app.use(express.json())

app.get('/api/health', (_req, res) => {
  res.json({ ok: true })
})

app.get('/api/state', (_req, res) => {
  res.json(bridge.getStatePayload())
})

app.post('/api/start', (req, res) => {
  const algorithm = req.body?.algorithm || 'Round Robin'
  const result = bridge.start(algorithm)
  res.json({ ...result, ...bridge.getStatePayload() })
})

app.post('/api/stop', (_req, res) => {
  const result = bridge.stop()
  res.json({ ...result, ...bridge.getStatePayload() })
})

app.post('/api/input', (req, res) => {
  const value = req.body?.value ?? ''
  const result = bridge.submitInput(String(value))
  res.json({ ...result, ...bridge.getStatePayload() })
})

app.post('/api/step', (_req, res) => {
  const result = bridge.step()
  res.json({ ...result, ...bridge.getStatePayload() })
})

app.post('/api/algorithm', (req, res) => {
  const algorithm = req.body?.algorithm
  if (!algorithm) {
    res.status(400).json({ ok: false, error: 'algorithm required' })
    return
  }
  const result = bridge.setAlgorithm(algorithm)
  res.json({ ...result, ...bridge.getStatePayload() })
})

app.listen(PORT, () => {
  console.log(`GUC-OS bridge listening on http://localhost:${PORT}`)
  console.log('POST /api/start spawns src/os_test.exe (cwd: src/)')
})
