import { spawn } from 'child_process'
import path from 'path'
import { fileURLToPath } from 'url'
import { OsOutputParser, algorithmToChoice } from './parser.js'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const SRC_DIR = path.resolve(__dirname, '..', 'src')
const EXE_PATH = path.join(SRC_DIR, 'os_test.exe')

const MAX_LOG_LINES = 500

export class SimulatorBridge {
  constructor() {
    this.proc = null
    this.parser = new OsOutputParser()
    this.logs = []
    this.status = 'idle'
    this.awaitingAlgorithm = false
    this.awaitingInput = false
    this.awaitingStep = false
    this.pendingAlgorithm = 'Round Robin'
    this.lineBuffer = ''
    this.exitCode = null
    this.algorithmSentForRun = false
  }

  getStatePayload() {
    return {
      status: this.status,
      exitCode: this.exitCode,
      awaitingAlgorithm: this.awaitingAlgorithm,
      awaitingInput: this.awaitingInput,
      awaitingStep: this.awaitingStep,
      logs: this.logs,
      state: this.parser.getState(),
    }
  }

  appendLog(text, stream = 'stdout') {
    if (!text) return
    const lines = text.split(/\r?\n/)
    for (const line of lines) {
      if (line.length === 0 && lines.length > 1) continue
      this.logs.push({ stream, text: line, ts: Date.now() })
    }
    while (this.logs.length > MAX_LOG_LINES) this.logs.shift()
  }

  handleStdoutChunk(chunk) {
    const text = chunk.toString()
    this.appendLog(text, 'stdout')
    this.lineBuffer += text
    const parts = this.lineBuffer.split(/\r?\n/)
    this.lineBuffer = parts.pop() ?? ''
    for (const line of parts) {
      this.processStdoutLine(line)
    }
    this.checkPromptsInBuffer()
  }

  checkPromptsInBuffer() {
    const buf = this.lineBuffer

    if (/Enter your choice \(1-3\):/.test(buf)) {
      this.awaitingAlgorithm = !this.algorithmSentForRun
      this.sendAlgorithmChoice()
    }

    if (/Please enter a value:/.test(buf)) {
      this.awaitingInput = true
    }

    if (/Waiting for GUI/.test(buf)) {
      this.awaitingStep = true
    }
  }

  processStdoutLine(line) {
    this.parser.processLine(line)

    if (/Enter your choice \(1-3\):/.test(line)) {
      this.awaitingAlgorithm = !this.algorithmSentForRun
      this.sendAlgorithmChoice()
    }

    if (line.includes('Please enter a value:')) {
      this.awaitingInput = true
    }

    if (line.includes('Waiting for GUI')) {
      this.awaitingStep = true
    }
  }

  writeStdin(text) {
    if (!this.proc?.stdin?.writable) return false
    this.proc.stdin.write(text)
    return true
  }

  sendAlgorithmChoice() {
    if (this.algorithmSentForRun) return false
    const choice = algorithmToChoice(this.pendingAlgorithm)
    const ok = this.writeStdin(`${choice}\n`)
    if (ok) {
      this.algorithmSentForRun = true
      this.awaitingAlgorithm = false
      this.parser.algorithm = this.pendingAlgorithm
    }
    return ok
  }

  start(algorithm = 'Round Robin') {
    if (this.proc) return { ok: false, error: 'Simulator already running' }

    this.pendingAlgorithm = algorithm
    this.parser.reset()
    this.logs = []
    this.lineBuffer = ''
    this.exitCode = null
    this.awaitingAlgorithm = false
    this.awaitingInput = false
    this.awaitingStep = false
    this.algorithmSentForRun = false
    this.status = 'running'

    this.proc = spawn(EXE_PATH, [], {
      cwd: SRC_DIR,
      stdio: ['pipe', 'pipe', 'pipe'],
      windowsHide: true,
    })

    this.proc.stdout.on('data', (chunk) => this.handleStdoutChunk(chunk))

    this.proc.stderr.on('data', (chunk) => {
      this.appendLog(chunk.toString(), 'stderr')
    })

    this.proc.on('error', (err) => {
      this.appendLog(`[bridge] spawn error: ${err.message}`, 'stderr')
      this.status = 'error'
    })

    this.proc.on('close', (code) => {
      this.clearAlgorithmRetry()
      if (this.lineBuffer) {
        this.processStdoutLine(this.lineBuffer)
        this.lineBuffer = ''
      }
      this.exitCode = code
      this.status = code === 0 ? 'finished' : 'stopped'
      this.proc = null
      this.awaitingAlgorithm = false
      this.awaitingInput = false
      this.awaitingStep = false
    })

    this.scheduleAlgorithmRetry()
    return { ok: true }
  }

  clearAlgorithmRetry() {
    if (this._algoRetry) {
      clearInterval(this._algoRetry)
      this._algoRetry = null
    }
  }

  scheduleAlgorithmRetry() {
    this.clearAlgorithmRetry()
    let attempts = 0
    this._algoRetry = setInterval(() => {
      attempts += 1
      if (this.algorithmSentForRun || !this.proc || attempts > 60) {
        this.clearAlgorithmRetry()
        return
      }
      if (/Enter your choice \(1-3\):/.test(this.lineBuffer)) {
        this.sendAlgorithmChoice()
      }
    }, 250)
  }

  stop() {
    if (!this.proc) return { ok: false, error: 'Not running' }
    this.clearAlgorithmRetry()
    this.proc.kill()
    return { ok: true }
  }

  submitInput(value) {
    if (!this.awaitingInput) return { ok: false, error: 'Not waiting for input' }
    const ok = this.writeStdin(`${value}\n`)
    if (ok) this.awaitingInput = false
    return { ok }
  }

  step() {
    if (!this.awaitingStep) return { ok: false, error: 'Not waiting for step' }
    const ok = this.writeStdin('\n')
    if (ok) this.awaitingStep = false
    return { ok }
  }

  setAlgorithm(algorithm) {
    this.pendingAlgorithm = algorithm
    if (!this.algorithmSentForRun) this.sendAlgorithmChoice()
    this.parser.algorithm = algorithm
    return { ok: true }
  }
}
