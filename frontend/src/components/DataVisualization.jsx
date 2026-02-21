import { useState, useEffect } from 'react'
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, BarChart, Bar, ResponsiveContainer } from 'recharts'
import { call } from '../api/client'

export function DataVisualization({ streamId = 1, pollIntervalMs = 1000 }) {
  const [data, setData] = useState([])
  const [mode, setMode] = useState('line') // 'line' | 'bar'

  useEffect(() => {
    const fetchStream = () => {
      call('readStream', { streamId })
        .then((r) => {
          if (r?.result?.error_code === 0) {
            const point = {
              time: new Date(r.result.timestamp).toLocaleTimeString(),
              value: parseFloat(r.result.payload) || 0,
              payload: r.result.payload,
            }
            setData((prev) => [...prev.slice(-59), point])
          }
        })
        .catch(() => {})
    }
    fetchStream()
    const id = setInterval(fetchStream, pollIntervalMs)
    return () => clearInterval(id)
  }, [streamId, pollIntervalMs])

  return (
    <div style={{ marginTop: 16 }}>
      <div style={{ display: 'flex', gap: 8, marginBottom: 8 }}>
        <button onClick={() => setMode('line')}>Line</button>
        <button onClick={() => setMode('bar')}>Bar</button>
      </div>
      <ResponsiveContainer width="100%" height={200}>
        {mode === 'line' ? (
          <LineChart data={data}>
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis dataKey="time" />
            <YAxis dataKey="value" />
            <Tooltip />
            <Line type="monotone" dataKey="value" stroke="#8884d8" strokeWidth={2} dot={false} />
          </LineChart>
        ) : (
          <BarChart data={data}>
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis dataKey="time" />
            <YAxis dataKey="value" />
            <Tooltip />
            <Bar dataKey="value" fill="#82ca9d" />
          </BarChart>
        )}
      </ResponsiveContainer>
    </div>
  )
}
