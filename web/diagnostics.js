/* A nézet az egyszer felépített mezőket frissíti; nincs másodpercenkénti DOM-csere. */
const words = {
    hu: {
        title: 'Rendszerdiagnosztika', remount: 'SD újracsatolása újraindítással',
        insert: 'Helyezd be a kártyát, majd indítsd újra a kijelzőt.', restarting: 'Újraindítás folyamatban.', retryFailed: 'Az újraindítás nem indult el.', unknown: 'Nem mért', connected: 'Kapcsolódva', offline: 'Nincs kapcsolat',
        free: 'szabad', minimum: 'Mélypont', cpu: 'CPU-terhelés', internal: 'Belső RAM', psram: 'PSRAM', sd: 'SD-kártya',
        wifi: 'Wi-Fi / web', loxone: 'Loxone', numeric: 'Számszerű Loxone-adat', dma: 'DMA-memória',
        system: 'Rendszer', stack: 'Feladatok / verem', update: 'Kiadásfrissítés', recovery: 'Beállító Wi-Fi',
        tasks: 'Feladatok', largest: 'Legnagyobb blokk', last: 'Utolsó adat', known: 'Ismert értékek',
        noData: 'Még nincs érvényes adat', clock: 'Óra beállítva', noClock: 'Az óra nincs beállítva',
        active: 'Aktív', inactive: 'Inaktív', recovered: 'Korábbi érvényes mentés visszaállítva',
        privateKey: 'A jelszó csak a kijelzőn olvasható.', events: 'Legutóbbi események',
        eventHint: 'Üzemidő szerinti időbélyegek. A napló újraindításkor kiürül.', noEvents: 'Még nincs esemény.',
        stale: 'Az utolsó minta kora', disconnected: 'A diagnosztikai kapcsolat megszakadt. Az utolsó pillanatkép látható.',
        fps: 'A fizikai megjelenítési sebesség még nem mért.'
    },
    en: {
        title: 'System diagnostics', remount: 'Restart to remount SD card',
        insert: 'Insert the card, then restart the display.', restarting: 'Restarting the display.', retryFailed: 'Could not start the restart.', unknown: 'Not measured', connected: 'Connected', offline: 'Disconnected',
        free: 'free', minimum: 'Minimum', cpu: 'CPU load', internal: 'Internal RAM', psram: 'PSRAM', sd: 'SD card',
        wifi: 'Wi-Fi / web', loxone: 'Loxone', numeric: 'Numeric Loxone data', dma: 'DMA memory',
        system: 'System', stack: 'Tasks / stack', update: 'Release update', recovery: 'Setup Wi-Fi',
        tasks: 'Tasks', largest: 'Largest block', last: 'Last update', known: 'Known values',
        noData: 'No valid data yet', clock: 'Clock valid', noClock: 'Clock not set',
        active: 'Active', inactive: 'Inactive', recovered: 'Earlier valid state recovered',
        privateKey: 'The password is only shown on the device.', events: 'Recent events',
        eventHint: 'Uptime timestamps. The log is cleared on restart.', noEvents: 'No events yet.',
        stale: 'Last sample age', disconnected: 'Diagnostic connection lost. The last snapshot is shown.',
        fps: 'Physical display frame rate has not been measured yet.'
    }
};
const finite = value => typeof value === 'number' && Number.isFinite(value);
export function usedPercent(total, free) {
    return finite(total) && finite(free) && total > 0 && free >= 0 && free <= total
        ? Math.min(100, Math.max(0, (total - free) / total * 100)) : null;
}
export function uptime(seconds) {
    if (!finite(seconds) || seconds < 0) return '—';
    return `${Math.floor(seconds / 3600)}:${String(Math.floor(seconds / 60) % 60).padStart(2, '0')}:${String(Math.floor(seconds) % 60).padStart(2, '0')}`;
}
export class DiagnosticPanel {
    constructor(root, {api, language, escape}) {
        this.root = root;
        this.api = api;
        this.language = language;
        this.escape = escape;
        this.text = words[language];
        this.closed = false;
        this.busy = false;
        const t = this.text;
        const keys = ['cpu', 'internal', 'psram', 'sd', 'wifi', 'loxone', 'numeric', 'dma', 'system', 'stack', 'update', 'recovery'];
        root.innerHTML = `<h1>${t.title}</h1><p id="diagnostics-status" role="status"></p>
            <div class="diagnostic-grid">${keys.map(key => `<article data-diag-card="${key}"><h2>${t[key]}</h2>
                <p class="diagnostic-value" data-diag="${key}">${t.unknown}</p>
                ${['cpu', 'internal', 'psram', 'sd'].includes(key) ? `<progress data-meter="${key}" max="100" value="0" aria-label="${t[key]}" hidden></progress>` : ''}
                ${key === 'cpu' ? '<progress data-meter="cpu1" max="100" value="0" aria-label="CPU1" hidden></progress>' : ''}
                ${key === 'sd' ? '<div id="sd-recovery" hidden><p></p><button type="button"></button><span role="status"></span></div>' : ''}
                </article>`).join('')}</div>
            <section><h2>${t.events}</h2><p class="hint">${t.eventHint}</p><ol id="diagnostic-events" class="diagnostic-events"></ol></section>
            <p class="hint">${t.fps}</p>`;
        this.recovery = root.querySelector('#sd-recovery');
        this.recovery.querySelector('p').textContent = t.insert;
        const button = this.recovery.querySelector('button');
        button.textContent = t.remount;
        button.onclick = async () => {
            button.disabled = true;
            const message = this.recovery.querySelector('span');
            try {
                await this.api('/api/storage/remount', {});
                message.textContent = t.restarting;
            } catch {
                message.textContent = t.retryFailed;
                button.disabled = false;
            }
        };
    }
    value(key, text) {
        const node = this.root.querySelector(`[data-diag="${key}"]`);
        if (node.textContent !== text) node.textContent = text;
    }
    meter(key, value) {
        const node = this.root.querySelector(`[data-meter="${key}"]`);
        const hide = !finite(value);
        if (node.hidden !== hide) node.hidden = hide;
        if (hide) return;
        const percent = Math.min(100, Math.max(0, value));
        const className = value >= 90 ? 'danger' : value >= 75 ? 'warning' : '';
        if (node.value !== percent) node.value = percent;
        if (node.className !== className) node.className = className;
    }
    bytes(value, unit = 'KiB') {
        if (!finite(value) || value < 0) return this.text.unknown;
        const divisor = {KiB: 1024, MiB: 1048576, GiB: 1073741824}[unit];
        return `${(value / divisor).toFixed(unit === 'GiB' ? 2 : 1)} ${unit}`;
    }
    connected(value) {
        return value === true ? this.text.connected : value === false ? this.text.offline : this.text.unknown;
    }
    draw(data) {
        const t = this.text, measured = data.measured === true, wifi = data.wifi || {}, loxone = data.loxone || {}, update = data.update || {};
        const number = value => finite(value) ? value : t.unknown;
        const cpuValid = measured && data.cpuValid === true && finite(data.cpu0) && finite(data.cpu1);
        this.value('cpu', cpuValid ? `CPU0 ${data.cpu0}% / CPU1 ${data.cpu1}%` : t.unknown);
        this.meter('cpu', cpuValid ? data.cpu0 : null);
        this.meter('cpu1', cpuValid ? data.cpu1 : null);
        for (const key of ['internal', 'psram']) {
            const available = measured ? data[`${key}Free`] : null;
            const total = measured ? data[`${key}Total`] : null;
            const unit = key === 'internal' ? 'KiB' : 'MiB';
            this.value(key, `${this.bytes(available, unit)} / ${this.bytes(total, unit)} ${t.free}${key === 'internal' ? `\n${t.minimum}: ${this.bytes(measured ? data.internalMinimum : null)}` : ''}`);
            this.meter(key, usedPercent(total, available));
        }
        const sdValid = measured && data.sdSpaceValid === true;
        this.value('sd', `${sdValid ? `${this.bytes(data.sdFree, 'GiB')} / ${this.bytes(data.sdTotal, 'GiB')} ${t.free}` : (data.sdMounted === false ? t.offline : t.unknown)}${data.storageRecovery === true ? `\n${t.recovered}` : ''}`);
        this.recovery.hidden = !(measured && (data.sdMounted === false || data.sdSpaceValid === false));
        this.meter('sd', sdValid ? usedPercent(data.sdTotal, data.sdFree) : null);
        this.value('wifi', `${this.connected(measured ? wifi.connected : null)}${wifi.ssid ? `\n${wifi.ssid}` : ''}${wifi.ip ? `\nhttp://${wifi.ip}/` : ''}${wifi.connected === true && finite(wifi.rssi) ? `\n${wifi.rssi} dBm` : ''}`);
        this.value('loxone', this.connected(measured ? loxone.connected : null));
        this.value('numeric', measured && finite(loxone.numericAge) ? `${t.last}: ${loxone.numericAge} s\n${t.known}: ${number(loxone.knownValues)}` : t.noData);
        this.value('dma', `${t.free}: ${this.bytes(measured ? data.dmaFree : null)}\n${t.largest}: ${this.bytes(measured ? data.dmaLargest : null)}`);
        this.value('system', measured ? `${data.firmware || '—'}\n${uptime(data.uptime)}\n${data.clockValid === true ? t.clock : t.noClock}` : t.unknown);
        this.value('stack', measured ? `${t.tasks}: ${number(data.taskCount)}\napp_main: ${this.bytes(data.mainStackFree)} ${t.minimum}` : t.unknown);
        this.value('update', measured ? `${update[this.language === 'hu' ? 'phaseHu' : 'phaseEn'] || update.phase || t.unknown}${update.version ? `\n${update.version}` : ''}\n${this.bytes(update.receivedBytes, 'MiB')}` : t.unknown);
        this.value('recovery', measured ? `${wifi.recovery ? `${t.active}\n${wifi.recoverySsid || ''}\n${t.privateKey}` : t.inactive}` : t.unknown);
        const events = Array.isArray(data.events) ? data.events.slice(0, 15) : [];
        const html = events.length ? events.map(event => `<li class="${event.level === 2 ? 'error' : event.level === 1 ? 'warning' : ''}"><time>${uptime(event.uptime)}</time> ${this.escape(event[this.language] || event.code || '')}</li>`).join('') : `<li>${t.noEvents}</li>`;
        if (html !== this.eventHtml) { this.root.querySelector('#diagnostic-events').innerHTML = html; this.eventHtml = html; }
        const status = this.root.querySelector('#diagnostics-status');
        const message = finite(data.sampleAge) && data.sampleAge > 3 ? `${t.stale}: ${data.sampleAge} s` : '';
        if (status.textContent !== message) status.textContent = message;
        if (status.className !== 'warning') status.className = 'warning';
    }
    async refresh() {
        if (this.busy || this.closed) return;
        this.busy = true;
        try {
            const data = await this.api('/api/diagnostics');
            if (!this.closed) this.draw(data);
        } catch {
            if (!this.closed) {
                const status = this.root.querySelector('#diagnostics-status');
                status.textContent = this.text.disconnected; status.className = 'error';
            }
        } finally { this.busy = false; }
    }
    destroy() { this.closed = true; }
}
