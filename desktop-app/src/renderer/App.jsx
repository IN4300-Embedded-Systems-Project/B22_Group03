import React from 'react'
import { LoraProvider } from './context/LoraContext'
import NodeStatusList from './components/NodeStatusList'
import AlertBanner from './components/AlertBanner'
import RssiChart from './components/RssiChart'
import MapView from './components/MapView'
import AlertLogTable from './components/AlertLogTable'
import PortSelector from './components/PortSelector'

export default function App() {
    return (
        <LoraProvider>
            <div className="app-shell">
                {/* ── Top Bar ── */}
                <header className="topbar">
                    <div className="topbar-left">
                        <span className="topbar-logo">📡</span>
                        <h1 className="topbar-title">LoRa Alert Dashboard</h1>
                    </div>
                    <div className="topbar-right">
                        <PortSelector />
                    </div>
                </header>

                {/* ── Alert Banner (sticky, conditionally rendered) ── */}
                <AlertBanner />

                {/* ── Main Layout ── */}
                <div className="main-layout">
                    {/* Left sidebar: Node status */}
                    <aside className="sidebar">
                        <NodeStatusList />
                    </aside>

                    {/* Center/Right: Charts + Map + Log */}
                    <main className="content-area">
                        <div className="top-panels">
                            <section className="panel panel-chart">
                                <h2 className="panel-title">RSSI Over Time</h2>
                                <RssiChart />
                            </section>
                            <section className="panel panel-map">
                                <h2 className="panel-title">Node Map</h2>
                                <MapView />
                            </section>
                        </div>
                        <section className="panel panel-log">
                            <h2 className="panel-title">Alert Log</h2>
                            <AlertLogTable />
                        </section>
                    </main>
                </div>
            </div>
        </LoraProvider>
    )
}
