import React, { useState, useEffect } from 'react'
import { useLora } from '../context/LoraContext'

export default function PortSelector() {
    const { serialStatus, setPort } = useLora()
    const [ports, setPorts] = useState(['MOCK', 'COM3', 'COM4', 'COM5'])
    const [selected, setSelected] = useState('MOCK')
    const [loading, setLoading] = useState(false)

    // Try to list real ports on mount
    useEffect(() => {
        if (window.lora.listPorts) {
            window.lora.listPorts().then(real => {
                if (real.length > 0) setPorts(['MOCK', ...real])
            })
        }
    }, [])

    const handleChange = async (e) => {
        const port = e.target.value
        setSelected(port)
        setLoading(true)
        await setPort(port)
        setLoading(false)
    }

    return (
        <div className="port-selector">
            <span className={`serial-indicator ${serialStatus.open ? 'si-open' : 'si-closed'}`} />
            <span className="serial-label">
                {serialStatus.mock ? 'MOCK MODE' : serialStatus.open ? `Connected: ${serialStatus.port}` : 'Disconnected'}
            </span>
            <select
                className="port-select"
                value={selected}
                onChange={handleChange}
                disabled={loading}
                title="Select COM port"
            >
                {ports.map(p => <option key={p} value={p}>{p}</option>)}
            </select>
        </div>
    )
}
