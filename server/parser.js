const MAX_MEMORY = 40
const GANTT_CAP = 200

const DEFAULT_SEMAPHORES = {
  userInput: { value: 1, ownerID: -1 },
  userOutput: { value: 1, ownerID: -1 },
  fileSystem: { value: 1, ownerID: -1 },
}

const ALGO_FROM_SCHEDULER = [
  [/Starting Round Robin/i, 'Round Robin'],
  [/Starting HRRN/i, 'HRRN'],
  [/Starting MLFQ/i, 'MLFQ'],
]

function emptyMemory() {
  return Array(MAX_MEMORY)
    .fill(null)
    .map(() => ({ owner: null, type: 'free' }))
}

function parseQueuePids(text) {
  const m = text.match(/\[(.*)\]/)
  if (!m || !m[1].trim()) return []
  return m[1]
    .split(',')
    .map((s) => parseInt(s.replace(/P/i, '').trim(), 10))
    .filter((n) => !Number.isNaN(n))
}

function normalizeSemName(name) {
  if (name === 'userInput' || name === 'userOutput' || name === 'fileSystem') return name
  return null
}

export class OsOutputParser {
  constructor() {
    this.reset()
  }

  reset() {
    this.clock = 0
    this.algorithm = 'Round Robin'
    this.processes = new Map()
    this.memoryWords = {}
    this.queues = {
      ready: [],
      inputBlocked: [],
      outputBlocked: [],
      diskBlocked: [],
    }
    this.semaphores = structuredClone(DEFAULT_SEMAPHORES)
    this.gantt = new Map()
    this.maxGanttClock = 0
    this.inMemoryTable = false
  }

  processLine(line) {
    const trimmed = line.trimEnd()

    for (const [re, name] of ALGO_FROM_SCHEDULER) {
      if (re.test(trimmed)) this.algorithm = name
    }

    const clockMem = trimmed.match(/System Clock:\s*(\d+)/)
    if (clockMem) {
      this.clock = parseInt(clockMem[1], 10)
      this.maxGanttClock = Math.max(this.maxGanttClock, this.clock)
    }

    const clockArrival = trimmed.match(/\[CLOCK\s+(\d+)\]/)
    if (clockArrival) {
      this.clock = parseInt(clockArrival[1], 10)
    }

    const loader = trimmed.match(/\[LOADER\] Loaded \d+ instructions from (.+)$/)
    if (loader) {
      const path = loader[1].trim()
      const name = path.split(/[/\\]/).pop()
      const pendingPid = this.pendingLoaderPid ?? this.processes.size + 1
      this.ensureProcess(pendingPid, { name })
      this.pendingLoaderPid = null
    }

    const pcbCreated = trimmed.match(/PCB Created: ID (\d+) \| Bounds \[(\d+)-(\d+)\]/)
    if (pcbCreated) {
      const pid = parseInt(pcbCreated[1], 10)
      this.ensureProcess(pid, {
        memLow: parseInt(pcbCreated[2], 10),
        memHigh: parseInt(pcbCreated[3], 10),
      })
    }

    const stateChange = trimmed.match(/Process (\d+) state changed to (\w+)/)
    if (stateChange) {
      const pid = parseInt(stateChange[1], 10)
      this.ensureProcess(pid, { state: stateChange[2] })
    }

    const executing = trimmed.match(
      />>>\s*(?:RR|HRRN|MLFQ).*Executing Process (\d+)(?:\s*\(PC:\s*(\d+)\))?/i,
    )
    const executingPlain = trimmed.match(/Executing Process (\d+)(?:\s*\(PC:\s*(\d+)\))?/i)
    const execMatch = executing || executingPlain
    if (execMatch) {
      const pid = parseInt(execMatch[1], 10)
      const pc = execMatch[2] != null ? parseInt(execMatch[2], 10) : undefined
      this.ensureProcess(pid, { state: 'RUNNING', ...(pc !== undefined ? { pc } : {}) })
      this.markGantt(pid, this.clock)
    }

    const blocked = trimmed.match(/Process (\d+) BLOCKED/)
    if (blocked) {
      this.ensureProcess(parseInt(blocked[1], 10), { state: 'BLOCKED' })
    }

    const finished = trimmed.match(/Process (\d+) FINISHED/)
    if (finished) {
      this.ensureProcess(parseInt(finished[1], 10), { state: 'FINISHED' })
    }

    const alloc = trimmed.match(/Process (\d+) was allocated words (\d+)[–-](\d+)/)
    if (alloc) {
      this.ensureProcess(parseInt(alloc[1], 10), {
        memLow: parseInt(alloc[2], 10),
        memHigh: parseInt(alloc[3], 10),
      })
      this.pendingLoaderPid = parseInt(alloc[1], 10)
    }

    const arrival = trimmed.match(/Process (\d+) Arrival Time Reached/)
    if (arrival) this.pendingLoaderPid = parseInt(arrival[1], 10)

    const swapOut = trimmed.match(/\[SWAP OUT\] Process (\d+)/)
    if (swapOut) {
      this.ensureProcess(parseInt(swapOut[1], 10), { memLow: -1, memHigh: -1 })
    }

    if (trimmed.includes('Ready Queue:')) {
      this.queues.ready = parseQueuePids(trimmed)
    }
    if (trimmed.includes('userInput Blocked:')) {
      this.queues.inputBlocked = parseQueuePids(trimmed)
    }
    if (trimmed.includes('userOutput Blocked:')) {
      this.queues.outputBlocked = parseQueuePids(trimmed)
    }
    if (trimmed.includes('file Blocked:')) {
      this.queues.diskBlocked = parseQueuePids(trimmed)
    }

    const semBlock = trimmed.match(/Process (\d+) BLOCKED waiting for (\w+)/)
    if (semBlock) {
      const sem = normalizeSemName(semBlock[2])
      if (sem) {
        this.semaphores[sem].value = 0
      }
    }

    const semLock = trimmed.match(/Process (\d+) successfully locked (\w+)/)
    if (semLock) {
      const sem = normalizeSemName(semLock[2])
      if (sem) {
        this.semaphores[sem].value = 0
        this.semaphores[sem].ownerID = parseInt(semLock[1], 10)
      }
    }

    const semRelease = trimmed.match(/Process (\d+) released lock for (\w+)/)
    if (semRelease) {
      const sem = normalizeSemName(semRelease[2])
      if (sem) {
        const blocked = this.queues[
          sem === 'userInput'
            ? 'inputBlocked'
            : sem === 'userOutput'
              ? 'outputBlocked'
              : 'diskBlocked'
        ]
        if (!blocked.length) {
          this.semaphores[sem].value = 1
          this.semaphores[sem].ownerID = -1
        }
      }
    }

    if (trimmed.includes('MEMORY STATE')) {
      this.inMemoryTable = true
      this.memoryWords = {}
      return
    }
    if (this.inMemoryTable && (trimmed.startsWith('╚') || /Words in use:/.test(trimmed))) {
      this.inMemoryTable = false
      this.applyMemoryTable()
      return
    }

    if (this.inMemoryTable) {
      const row = trimmed.match(/║\s*(\d+)\s*║\s*([^║]+?)\s*║\s*([^║]*?)\s*║/)
      if (row) {
        const index = parseInt(row[1], 10)
        const key = row[2].trim()
        const val = row[3].trim()
        if (key === '[free]') {
          this.memoryWords[index] = { free: true }
        } else {
          this.memoryWords[index] = { key, val }
        }
      }
    }
  }

  ensureProcess(pid, patch) {
    const existing = this.processes.get(pid) || {
      pid,
      name: `prog_${pid}.txt`,
      state: 'READY',
      pc: 0,
      memLow: -1,
      memHigh: -1,
    }
    this.processes.set(pid, { ...existing, ...patch })
  }

  applyMemoryTable() {
    const pcbFields = {}

    for (const [indexStr, cell] of Object.entries(this.memoryWords)) {
      const index = parseInt(indexStr, 10)
      if (cell.free) continue
      const m = cell.key.match(/^PCB_(\d+)_(id|state|PC|memLow|memHigh)$/)
      if (m) {
        const pid = parseInt(m[1], 10)
        if (!pcbFields[pid]) pcbFields[pid] = {}
        pcbFields[pid][m[2]] = cell.val
      }
    }

    for (const [pidStr, fields] of Object.entries(pcbFields)) {
      const pid = parseInt(pidStr, 10)
      const patch = {}
      if (fields.id) patch.pid = parseInt(fields.id, 10)
      if (fields.state) patch.state = fields.state
      if (fields.PC != null) patch.pc = parseInt(fields.PC, 10)
      if (fields.memLow != null) patch.memLow = parseInt(fields.memLow, 10)
      if (fields.memHigh != null) patch.memHigh = parseInt(fields.memHigh, 10)
      this.ensureProcess(pid, patch)

      if (fields.state === 'RUNNING') {
        this.markGantt(pid, this.clock)
      }
    }

    this.rebuildMemorySlots()
  }

  markGantt(pid, tick) {
    if (!this.gantt.has(pid)) this.gantt.set(pid, [])
    const slots = this.gantt.get(pid)
    while (slots.length <= tick) slots.push(0)
    slots[tick] = 1
    this.maxGanttClock = Math.max(this.maxGanttClock, tick)
  }

  rebuildMemorySlots() {
    const ranges = []
    for (const p of this.processes.values()) {
      if (p.memLow >= 0 && p.memHigh >= p.memLow) {
        ranges.push({ pid: p.pid, low: p.memLow, high: p.memHigh })
      }
    }

    this.memorySlots = emptyMemory()
    for (let i = 0; i < MAX_MEMORY; i++) {
      const owner = ranges.find((r) => i >= r.low && i <= r.high)
      if (owner) {
        this.memorySlots[i] = { owner: owner.pid, type: 'process' }
      }
    }
  }

  getState() {
    this.rebuildMemorySlots()
    const processes = [...this.processes.values()].sort((a, b) => a.pid - b.pid)
    const ganttPids = [...new Set([...this.gantt.keys(), ...processes.map((p) => p.pid)])].sort(
      (a, b) => a - b,
    )
    const width = Math.min(
      Math.max(this.maxGanttClock + 1, 1),
      GANTT_CAP,
    )

    const gantt = ganttPids.map((pid) => {
      const slots = this.gantt.get(pid) || []
      const row = Array(width).fill(0)
      for (let i = 0; i < width; i++) {
        row[i] = slots[i] ? 1 : 0
      }
      return { pid, slots: row }
    })

    return {
      clock: this.clock,
      algorithm: this.algorithm,
      processes,
      memory: this.memorySlots || emptyMemory(),
      queues: { ...this.queues },
      semaphores: structuredClone(this.semaphores),
      gantt,
    }
  }
}

export function algorithmToChoice(algorithm) {
  switch (algorithm) {
    case 'HRRN':
      return '2'
    case 'MLFQ':
      return '3'
    default:
      return '1'
  }
}
