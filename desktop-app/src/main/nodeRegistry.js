const HEARTBEAT_TIMEOUT = 30_000
const MAX_ALERTS = 500
const MAX_RSSI_HISTORY = 60

/** @type {Map<string, object>} */
const registry = new Map()
const alertHistory = []

// Pre-populate with initial dummy data
const initialMockNodes = [
    { nodeId: 'N1', lat: 6.927, lng: 79.861 },
    { nodeId: 'N2', lat: 6.932, lng: 79.855 },
    { nodeId: 'N3', lat: 6.921, lng: 79.870 }
];

const now = Date.now();
initialMockNodes.forEach(n => {
    const history = [];
    for (let j = 0; j < 60; j++) {
        history.push({ t: now - (60 - j) * 2000, rssi: -60 - Math.floor(Math.random() * 40) });
    }
    registry.set(n.nodeId, {
        nodeId: n.nodeId,
        status: 'online',
        firstSeen: now - 120000,
        lastSeen: now,
        rssiHistory: history,
        rssi: history[history.length - 1].rssi,
        lat: n.lat,
        lng: n.lng,
        lastType: 'data',
        lastMessage: '',
        lastSeverity: 'info'
    });
});

alertHistory.push({
    id: `N1-${now - 10000}`,
    timestamp: new Date(now - 10000).toISOString(),
    nodeId: 'N1',
    type: 'alert',
    severity: 'warn',
    message: 'Motion detected at sector 2',
    rssi: -75
});
alertHistory.push({
    id: `N3-${now - 50000}`,
    timestamp: new Date(now - 50000).toISOString(),
    nodeId: 'N3',
    type: 'alert',
    severity: 'critical',
    message: 'Motion detected at sector 7',
    rssi: -82
});

let heartbeatInterval = null
let _win = null

export function updateNode(packet) {
    const { nodeId, rssi, lat, lng, type, message, severity } = packet
    const now = Date.now()
    const existing = registry.get(nodeId) || {
        nodeId, status: 'online', firstSeen: now, rssiHistory: [], lat: null, lng: null
    }

    const rssiHistory = [...existing.rssiHistory, { t: now, rssi: rssi ?? null }].slice(-MAX_RSSI_HISTORY)

    registry.set(nodeId, {
        ...existing,
        status: 'online',
        lastSeen: now,
        rssi: rssi ?? existing.rssi,
        rssiHistory,
        lat: lat ?? existing.lat,
        lng: lng ?? existing.lng,
        lastType: type,
        lastMessage: message ?? existing.lastMessage,
        lastSeverity: severity ?? existing.lastSeverity
    })

    return getRegistrySnapshot()
}

export function addAlert(packet) {
    const alert = {
        id: `${packet.nodeId}-${Date.now()}`,
        timestamp: new Date().toISOString(),
        nodeId: packet.nodeId,
        type: packet.type,
        severity: packet.severity || 'info',
        message: packet.message || '',
        rssi: packet.rssi ?? null
    }
    alertHistory.unshift(alert)
    if (alertHistory.length > MAX_ALERTS) alertHistory.length = MAX_ALERTS
    return alert
}

export function getRegistrySnapshot() {
    return Array.from(registry.values())
}

export function getAlertHistory() {
    return [...alertHistory]
}

export function startHeartbeatWatcher(win) {
    _win = win
    if (heartbeatInterval) clearInterval(heartbeatInterval)

    heartbeatInterval = setInterval(() => {
        const now = Date.now()
        let changed = false

        for (const [id, node] of registry.entries()) {
            if (node.status === 'online' && now - node.lastSeen > HEARTBEAT_TIMEOUT) {
                registry.set(id, { ...node, status: 'offline' })
                changed = true
            }
        }

        if (changed) {
            _win?.webContents.send('registry-update', getRegistrySnapshot())
        }
    }, 5_000)
}

export function stopHeartbeatWatcher() {
    if (heartbeatInterval) clearInterval(heartbeatInterval)
}
