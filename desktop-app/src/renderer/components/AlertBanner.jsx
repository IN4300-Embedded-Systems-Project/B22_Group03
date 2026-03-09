import React, { useEffect, useState } from 'react'
import { useLora } from '../context/LoraContext'

const SEVERITY_ICON = { info: 'ℹ️', warn: '⚠️', critical: '🚨' }
const AUTO_DISMISS_MS = 10_000

export default function AlertBanner() {
    const { activeAlert, dismissAlert } = useLora()
    const [visible, setVisible] = useState(false)
    const [current, setCurrent] = useState(null)

    useEffect(() => {
        if (!activeAlert) return
        setCurrent(activeAlert)
        setVisible(true)

        const timer = setTimeout(() => {
            setVisible(false)
            dismissAlert()
        }, AUTO_DISMISS_MS)

        return () => clearTimeout(timer)
    }, [activeAlert, dismissAlert])

    const handleClose = () => {
        setVisible(false)
        dismissAlert()
    }

    if (!visible || !current) return null

    const sev = current.severity || 'info'

    return (
        <div className={`alert-banner alert-banner-${sev}`} role="alert">
            <span className="alert-banner-icon">{SEVERITY_ICON[sev] || 'ℹ️'}</span>
            <div className="alert-banner-body">
                <strong className="alert-banner-node">[{current.nodeId}]</strong>
                <span className="alert-banner-sev">{sev.toUpperCase()}</span>
                <span className="alert-banner-msg">{current.message || 'Alert received'}</span>
                {current.rssi != null && (
                    <span className="alert-banner-rssi">RSSI: {current.rssi} dBm</span>
                )}
            </div>
            <button className="alert-banner-close" onClick={handleClose} title="Dismiss">✕</button>
        </div>
    )
}
