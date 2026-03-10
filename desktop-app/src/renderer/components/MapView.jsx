import React, { useEffect, useRef } from 'react'
import { useLora } from '../context/LoraContext'
import {
    MapContainer, TileLayer, CircleMarker, Polygon,
    Tooltip as LeafletTooltip, useMap
} from 'react-leaflet'
import 'leaflet/dist/leaflet.css'

// Default center: Colombo, Sri Lanka
const DEFAULT_CENTER = [6.927, 79.861]
const DEFAULT_ZOOM = 14

// Example predefined zones (customize coordinates as needed)
const ZONES = [
    {
        id: 'zone-a',
        name: 'Zone A',
        color: '#60a5fa',
        positions: [[6.930, 79.855], [6.935, 79.855], [6.935, 79.865], [6.930, 79.865]]
    },
    {
        id: 'zone-b',
        name: 'Zone B',
        color: '#34d399',
        positions: [[6.920, 79.862], [6.925, 79.862], [6.925, 79.872], [6.920, 79.872]]
    }
]

function NodeMarker({ node }) {
    const isOnline = node.status === 'online'
    if (node.lat == null || node.lng == null) return null

    const color = isOnline
        ? (node.lastSeverity === 'critical' ? '#f87171' : '#34d399')
        : '#64748b'

    return (
        <CircleMarker
            center={[node.lat, node.lng]}
            radius={10}
            pathOptions={{ color, fillColor: color, fillOpacity: 0.85, weight: 2 }}
        >
            <LeafletTooltip>
                <strong>{node.nodeId}</strong><br />
                Status: {node.status}<br />
                RSSI: {node.rssi != null ? `${node.rssi} dBm` : '-'}<br />
                {node.lastMessage && <span>Msg: {node.lastMessage}</span>}
            </LeafletTooltip>
        </CircleMarker>
    )
}

export default function MapView() {
    const { nodes } = useLora()
    const nodesWithCoords = nodes.filter(n => n.lat != null && n.lng != null)

    return (
        <div className="map-container">
            <MapContainer
                center={DEFAULT_CENTER}
                zoom={DEFAULT_ZOOM}
                style={{ width: '100%', height: '100%', borderRadius: 8 }}
                attributionControl={false}
            >
                <TileLayer
                    url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
                    attribution='&copy; OpenStreetMap contributors'
                />

                {/* Draw predefined zones */}
                {ZONES.map(zone => (
                    <Polygon
                        key={zone.id}
                        positions={zone.positions}
                        pathOptions={{ color: zone.color, fillOpacity: 0.1, weight: 2 }}
                    >
                        <LeafletTooltip sticky>{zone.name}</LeafletTooltip>
                    </Polygon>
                ))}

                {/* Draw node markers */}
                {nodesWithCoords.map(node => (
                    <NodeMarker key={node.nodeId} node={node} />
                ))}
            </MapContainer>

            {/* Legend */}
            <div className="map-legend">
                <span className="legend-item"><span className="legend-dot" style={{ background: '#34d399' }} />Online</span>
                <span className="legend-item"><span className="legend-dot" style={{ background: '#f87171' }} />Alert</span>
                <span className="legend-item"><span className="legend-dot" style={{ background: '#64748b' }} />Offline</span>
            </div>
        </div>
    )
}
