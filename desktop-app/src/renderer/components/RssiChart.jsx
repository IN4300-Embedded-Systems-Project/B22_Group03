import React, { useMemo } from 'react'
import { useLora } from '../context/LoraContext'
import {
    LineChart, Line, XAxis, YAxis, CartesianGrid,
    Tooltip, Legend, ResponsiveContainer
} from 'recharts'

// Fixed color palette per node (cycles if > 8 nodes)
const NODE_COLORS = [
    '#60a5fa', '#34d399', '#f59e0b', '#f87171',
    '#a78bfa', '#fb923c', '#38bdf8', '#4ade80'
]

export default function RssiChart() {
    const { nodes } = useLora()

    // Build a unified time-series: [{t, N1, N2, ...}]
    const { series, nodeIds } = useMemo(() => {
        const nodeIds = nodes.map(n => n.nodeId)
        if (nodes.length === 0) return { series: [], nodeIds: [] }

        // Collect all timestamps across all nodes, sort
        const allTs = Array.from(
            new Set(nodes.flatMap(n => (n.rssiHistory || []).map(p => p.t)))
        ).sort()

        // Build a map: ts -> { t, N1: rssi, N2: rssi, ... }
        const tsMap = new Map()
        allTs.forEach(t => tsMap.set(t, { t }))

        nodes.forEach(node => {
            ; (node.rssiHistory || []).forEach(({ t, rssi }) => {
                if (tsMap.has(t)) tsMap.get(t)[node.nodeId] = rssi
            })
        })

        return {
            series: Array.from(tsMap.values()).slice(-60),
            nodeIds
        }
    }, [nodes])

    const formatTime = (t) => {
        const d = new Date(t)
        return `${String(d.getHours()).padStart(2, '0')}:${String(d.getMinutes()).padStart(2, '0')}:${String(d.getSeconds()).padStart(2, '0')}`
    }

    if (series.length === 0) {
        return <div className="chart-empty">Waiting for RSSI data…</div>
    }

    return (
        <ResponsiveContainer width="100%" height={220}>
            <LineChart data={series} margin={{ top: 5, right: 20, left: 0, bottom: 5 }}>
                <CartesianGrid strokeDasharray="3 3" stroke="#1e2535" />
                <XAxis
                    dataKey="t"
                    tickFormatter={formatTime}
                    tick={{ fill: '#94a3b8', fontSize: 11 }}
                    stroke="#334155"
                />
                <YAxis
                    domain={[-110, -40]}
                    tick={{ fill: '#94a3b8', fontSize: 11 }}
                    stroke="#334155"
                    unit=" dBm"
                />
                <Tooltip
                    contentStyle={{ background: '#1e2535', border: '1px solid #334155', borderRadius: 8 }}
                    labelStyle={{ color: '#94a3b8' }}
                    formatter={(v, name) => [`${v} dBm`, name]}
                    labelFormatter={(t) => formatTime(t)}
                />
                <Legend wrapperStyle={{ color: '#94a3b8', fontSize: 12 }} />
                {nodeIds.map((id, i) => (
                    <Line
                        key={id}
                        type="monotone"
                        dataKey={id}
                        stroke={NODE_COLORS[i % NODE_COLORS.length]}
                        strokeWidth={2}
                        dot={false}
                        isAnimationActive={false}
                        connectNulls
                    />
                ))}
            </LineChart>
        </ResponsiveContainer>
    )
}
