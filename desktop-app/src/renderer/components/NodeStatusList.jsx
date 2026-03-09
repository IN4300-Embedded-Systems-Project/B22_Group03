import React from 'react'
import { useLora } from '../context/LoraContext'

export default function NodeStatusList() {
    const { nodes } = useLora()

    const online = nodes.filter(n => n.status === 'online').length
    const offline = nodes.filter(n => n.status === 'offline').length

    return (
        <div className="node-list">
            <div className="node-list-header">
                <span className="node-list-title">Nodes</span>
                <div className="node-counts">
                    <span className="badge badge-online">{online} online</span>
                    <span className="badge badge-offline">{offline} offline</span>
                </div>
            </div>

            {nodes.length === 0 && (
                <p className="node-empty">Waiting for data…</p>
            )}

            <ul className="node-items">
                {nodes.map(node => (
                    <NodeItem key={node.nodeId} node={node} />
                ))}
            </ul>
        </div>
    )
}

function NodeItem({ node }) {
    const isOnline = node.status === 'online'
    const lastSeen = node.lastSeen
        ? timeSince(node.lastSeen)
        : '—'

    return (
        <li className={`node-item ${isOnline ? 'node-online' : 'node-offline'}`}>
            <div className="node-item-header">
                <span className={`status-dot ${isOnline ? 'dot-online' : 'dot-offline'}`} />
                <span className="node-id">{node.nodeId}</span>
                <span className={`node-badge ${isOnline ? 'badge-online' : 'badge-offline'}`}>
                    {isOnline ? 'ONLINE' : 'OFFLINE'}
                </span>
            </div>
            <div className="node-item-meta">
                <span className="meta-label">RSSI</span>
                <span className={`meta-value rssi-value ${rssiClass(node.rssi)}`}>
                    {node.rssi != null ? `${node.rssi} dBm` : '—'}
                </span>
            </div>
            <div className="node-item-meta">
                <span className="meta-label">Last seen</span>
                <span className="meta-value">{lastSeen}</span>
            </div>
            {node.lastMessage && (
                <div className="node-last-msg">{node.lastMessage}</div>
            )}
        </li>
    )
}

function rssiClass(rssi) {
    if (rssi == null) return ''
    if (rssi >= -70) return 'rssi-good'
    if (rssi >= -85) return 'rssi-fair'
    return 'rssi-poor'
}

function timeSince(ts) {
    const sec = Math.floor((Date.now() - ts) / 1000)
    if (sec < 60) return `${sec}s ago`
    if (sec < 3600) return `${Math.floor(sec / 60)}m ago`
    return `${Math.floor(sec / 3600)}h ago`
}
