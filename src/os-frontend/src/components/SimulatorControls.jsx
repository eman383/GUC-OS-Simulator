import { useState } from 'react'

export default function SimulatorControls({
  status,
  awaitingInput,
  awaitingStep,
  onStart,
  onStop,
  onStep,
  onSubmitInput,
  canStart,
}) {
  const [inputValue, setInputValue] = useState('')

  const handleSubmit = (e) => {
    e.preventDefault()
    onSubmitInput(inputValue)
    setInputValue('')
  }

  return (
    <div className="sim-controls">
      <div className="sim-controls__buttons">
        <button
          type="button"
          className="sim-controls__btn sim-controls__btn--primary"
          disabled={!canStart || status === 'running'}
          onClick={onStart}
        >
          Start
        </button>
        <button
          type="button"
          className="sim-controls__btn"
          disabled={status !== 'running'}
          onClick={onStop}
        >
          Stop
        </button>
        <button
          type="button"
          className={`sim-controls__btn${awaitingStep ? ' sim-controls__btn--pulse' : ''}`}
          disabled={!awaitingStep}
          onClick={onStep}
          title="Send newline when simulator waits for GUI step"
        >
          Step
        </button>
      </div>

      {awaitingInput && (
        <form className="sim-controls__input-form" onSubmit={handleSubmit}>
          <label className="sim-controls__input-label" htmlFor="sim-input">
            Program input required
          </label>
          <div className="sim-controls__input-row">
            <input
              id="sim-input"
              type="text"
              className="sim-controls__input"
              value={inputValue}
              onChange={(e) => setInputValue(e.target.value)}
              placeholder="Value for assign … input"
              autoFocus
            />
            <button type="submit" className="sim-controls__btn sim-controls__btn--primary">
              Send
            </button>
          </div>
        </form>
      )}
    </div>
  )
}
