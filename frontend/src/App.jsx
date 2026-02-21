import { useState, useEffect } from 'react'
import { call, notify } from './api/client'
import { DataVisualization } from './components/DataVisualization'
import './App.css'

function App() {
  const [appStatus, setAppStatus] = useState(null)
  const [streamData, setStreamData] = useState(null)
  const [logs, setLogs] = useState([])
  const [error, setError] = useState(null)

  const addLog = (msg) => setLogs((prev) => [...prev.slice(-99), `${new Date().toISOString().slice(11, 19)} ${msg}`])

  useEffect(() => {
    call('readApp', { id: 1 })
      .then((resp) => {
        if (resp?.result?.error_code === 0) {
          setAppStatus(resp.result)
        } else {
          setError(resp?.result?.error ?? 'unknown')
        }
      })
      .catch((e) => setError(e.message))
    call('readStream', { streamId: 1 })
      .then((resp) => resp?.result?.error_code === 0 && setStreamData(resp.result))
      .catch(() => {})
  }, [])

  const handleStart = () => {
    notify('writeApp', { status: 'start' })
    addLog('Start')
    call('readApp', { id: 1 }).then((r) => r?.result?.error_code === 0 && setAppStatus(r.result))
  }
  const handleStop = () => {
    notify('writeApp', { status: 'stop' })
    addLog('Stop')
    call('readApp', { id: 1 }).then((r) => r?.result?.error_code === 0 && setAppStatus(r.result))
  }
  const handleReset = () => {
    notify('writeApp', { status: 'reset' })
    addLog('Reset')
    call('readApp', { id: 1 }).then((r) => r?.result?.error_code === 0 && setAppStatus(r.result))
  }

  return (
    <>
      <h1>CEF + React Demo</h1>
      <div className="card">
        {error && <p style={{ color: 'red' }}>{error}</p>}
        {appStatus && (
          <p>
            <strong>ApplicationData:</strong> id={appStatus.id}, name={appStatus.name}, status=
            <strong>{appStatus.status}</strong>
          </p>
        )}
        {streamData && (
          <p>
            <strong>StreamData:</strong> streamId={streamData.streamId}, payload={streamData.payload}, timestamp=
            {streamData.timestamp}
          </p>
        )}
        <div style={{ display: 'flex', gap: '8px', flexWrap: 'wrap' }}>
          <button onClick={handleStart}>Start</button>
          <button onClick={handleStop}>Stop</button>
          <button onClick={handleReset}>Reset</button>
        </div>
        {logs.length > 0 && (
          <div style={{ marginTop: 12, fontFamily: 'monospace', fontSize: 12, maxHeight: 120, overflow: 'auto' }}>
            <strong>Log:</strong>
            <ul style={{ margin: 4, paddingLeft: 20 }}>
              {logs.map((l, i) => (
                <li key={i}>{l}</li>
              ))}
            </ul>
          </div>
        )}
        <DataVisualization streamId={1} pollIntervalMs={1500} />
      </div>
    </>
  )
}

export default App
