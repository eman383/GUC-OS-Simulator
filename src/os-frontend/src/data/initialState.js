// Fallback shape before the Node bridge returns parsed os_test.exe output.

export const initialState = {
  clock: 4,
  algorithm: 'Round Robin',
  processes: [
    { pid: 1, name: 'prog_1.txt', state: 'RUNNING', pc: 3, memLow: 0, memHigh: 9 },
    { pid: 2, name: 'prog_2.txt', state: 'BLOCKED', pc: 2, memLow: 10, memHigh: 19 },
    { pid: 3, name: 'prog_3.txt', state: 'READY', pc: 0, memLow: 20, memHigh: 29 },
  ],
  memory: Array(40)
    .fill(null)
    .map((_, i) => {
      if (i <= 9) return { owner: 1, type: 'process' }
      if (i <= 19) return { owner: 2, type: 'process' }
      if (i <= 29) return { owner: 3, type: 'process' }
      return { owner: null, type: 'free' }
    }),
  queues: {
    ready: [1, 3],
    inputBlocked: [2],
    outputBlocked: [],
    diskBlocked: [],
  },
  semaphores: {
    userInput: { value: 0, ownerID: 2 },
    userOutput: { value: 1, ownerID: -1 },
    fileSystem: { value: 1, ownerID: -1 },
  },
  gantt: [
    { pid: 1, slots: [1, 1, 0, 1, 1] },
    { pid: 2, slots: [0, 0, 1, 0, 0] },
    { pid: 3, slots: [0, 0, 0, 0, 0] },
  ],
}
