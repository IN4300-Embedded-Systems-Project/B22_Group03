import React, { useState, useMemo } from 'react'
import { useLora } from '../context/LoraContext'

const SEV_OPTIONS = ['all', 'info', 'warn', 'critical']

export default function AlertLogTable() {
    const { alerts } = useLora()
    const [filterSev, setFilterSev] = useState('all')
    const [filterNode, setFilterNode] = useState('')
    const [filterDate, setFilterDate] = useState('')

    const filtered = useMemo(() => {
        return alerts.filter(a => {
            if (filterSev !== 'all' && a.severity !== filterSev) return false
            if (filterNode && !a.nodeId.toLowerCase().includes(filterNode.toLowerCase())) return false
            if (filterDate) {
                const alertDay = a.timestamp.slice(0, 10)
                if (alertDay !== filterDate) return false
            }
            return true
        })
    }, [alerts, filterSev, filterNode, filterDate])

    return (
        <div className="alert-log">
            {/* Filter Bar */}
            <div className="alert-log-filters">
                <div className="filter-group">
                    <label className="filter-label">Severity</label>
                    <select
                        className="filter-select"
                        value={filterSev}
                        onChange={e => setFilterSev(e.target.value)}
                    >
                        {SEV_OPTIONS.map(s => <option key={s} value={s}>{s}</option>)}
                    </select>
                </div>
                <div className="filter-group">
                    <label className="filter-label">Node ID</label>
                    <input
                        className="filter-input"
                        type="text"
                        placeholder="Filter by node…"
                        value={filterNode}
                        onChange={e => setFilterNode(e.target.value)}
                    />
                </div>
                <div className="filter-group">
                    <label className="filter-label">Date</label>
                    <input
                        className="filter-input"
                        type="date"
                        value={filterDate}
                        onChange={e => setFilterDate(e.target.value)}
                    />
                </div>
                <span className="filter-count">{filtered.length} entries</span>
            </div>

            {/* Table */}
            <div className="alert-log-scroll">
                <table className="alert-table">
                    <thead>
                        <tr>
                            <th>Time</th>
                            <th>Node</th>
                            <th>Severity</th>
                            <th>RSSI</th>
                            <th>Message</th>
                        </tr>
                    </thead>
                    <tbody>
                        {filtered.length === 0 && (
                            <tr><td colSpan={5} className="log-empty">No alerts match the current filter.</td></tr>
                        )}
                        {filtered.map(alert => (
                            <tr key={alert.id} className={`log-row sev-${alert.severity}`}>
                                <td className="log-time">{formatTime(alert.timestamp)}</td>
                                <td className="log-node">{alert.nodeId}</td>
                                <td>
                                    <span className={`sev-badge sev-badge-${alert.severity}`}>
                                        {alert.severity}
                                    </span>
                                </td>
                                <td className="log-rssi">{alert.rssi != null ? `${alert.rssi} dBm` : '—'}</td>
                                <td className="log-msg">{alert.message || '—'}</td>
                            </tr>
                        ))}
                    </tbody>
                </table>
            </div>
        </div>
    )
}

function formatTime(iso) {
    const d = new Date(iso)
    return d.toLocaleTimeString('en-GB', { hour: '2-digit', minute: '2-digit', second: '2-digit' })
}
