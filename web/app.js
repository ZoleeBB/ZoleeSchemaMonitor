import {messages} from './i18n.js';
import {canvasSize, newProject, validateBinding} from './model.js';
import {Editor} from './editor.js';
import {DiagnosticPanel} from './diagnostics.js';
import {BackupPanel} from './backup.js';

export const $ = selector => document.querySelector(selector);
export const escape = value => String(value ?? '').replace(/[&<>"']/g, character => ({'&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;'}[character]));
let language = localStorage.getItem('schema-language') || 'hu';
export const tr = key => messages[language][key] || key;
export const state = {settings: {}, device: {}, catalog: {elements: []}, project: newProject(), revision: 0, values: {}, sources: []};
let editor;
let diagnostics;
let backup;
let timer;
let firmwareUploading = false;
let releasePolling = false;

export function feedback(message, error = false) {
    $('#feedback').textContent = message;
    $('#feedback').className = error ? 'error' : 'status-ok';
}

export async function api(path, data, method = 'POST', timeoutMs = 15000) {
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), timeoutMs);
    try {
        const response = await fetch(path, {signal: controller.signal, cache: 'no-store',
            ...(data === undefined ? {} : {method, headers: {'Content-Type': 'application/json', 'X-Zolee-Schema': '1'}, body: JSON.stringify(data)})});
        const result = await response.json();
        if (!response.ok) throw Error(response.status === 409 ? tr('conflict') : result.error || response.statusText);
        return result;
    } finally {
        clearTimeout(timeout);
    }
}

function label(key, input) {
    return `<label>${tr(key)}${input}</label>`;
}

function input(name, value, type = 'text', extra = '') {
    return `<input name="${name}" type="${type}" value="${escape(value)}" ${extra}>`;
}

export async function chooseBinding(current) {
    const dialog = $('#binding-dialog');
    $('#binding-title').textContent = tr('binding');
    $('#binding-cancel').textContent = tr('cancel');
    $('#binding-clear').textContent = tr('clear');
    $('#binding-apply').textContent = tr('apply');
    $('#room-label').firstChild.textContent = tr('room');
    $('#category-label').firstChild.textContent = tr('category');
    $('#binding-scale').value = current?.scale ?? 1;
    $('#binding-offset').value = current?.offset ?? 0;
    $('#binding-search').value = '';
    $('#binding-error').textContent = '';
    for (const [selector, property] of [['#room-filter', 'room'], ['#category-filter', 'category']]) {
        const select = $(selector);
        select.replaceChildren(new Option(tr('all'), ''));
        for (const name of [...new Set(state.sources.map(source => source[property] || ''))].sort()) {
            if (name) select.add(new Option(name, name));
        }
    }
    const refresh = () => {
        const options = $('#binding-options');
        const selected = options.value || current?.uuid;
        options.replaceChildren();
        const search = $('#binding-search').value.toLocaleLowerCase();
        for (const source of state.sources) {
            if ($('#room-filter').value && source.room !== $('#room-filter').value) continue;
            if ($('#category-filter').value && source.category !== $('#category-filter').value) continue;
            const title = `${source.name} · ${source.room || '—'} / ${source.category || '—'} · ${source.state || ''}`;
            if (!title.toLocaleLowerCase().includes(search)) continue;
            options.add(new Option(title, source.uuid, false, source.uuid === selected));
        }
        if (current && !state.sources.some(source => source.uuid === current.uuid)) {
            options.add(new Option(`${tr('unknown')}: ${current.uuid}`, current.uuid, false, true));
        }
    };
    for (const selector of ['#room-filter', '#category-filter', '#binding-search']) $(selector).oninput = refresh;
    refresh();
    return new Promise(resolve => {
        let resolved = false;
        const close = value => {
            if (resolved) return;
            resolved = true;
            dialog.close();
            resolve(value);
        };
        dialog.onclose = () => close(undefined);
        $('#binding-clear').onclick = () => close(null);
        $('#binding-apply').onclick = () => {
            try {
                const binding = {uuid: $('#binding-options').value, scale: Number($('#binding-scale').value), offset: Number($('#binding-offset').value)};
                if (!$('#binding-scale').value.trim() || !$('#binding-offset').value.trim()) throw Error(tr('unknown'));
                validateBinding(binding);
                close(binding);
            } catch (error) {
                $('#binding-error').textContent = error.message;
            }
        };
        dialog.showModal();
    });
}

function renderHome() {
    const device = state.device;
    const content = $('#content');
    content.innerHTML = `<h1>${escape(state.settings.project?.title || 'ZoleeSchemaMonitor')}</h1>
        <p class="lead">Loxone · ESP32-P4 · 10.1″</p><section><h2>${tr('home')}</h2><dl class="kv">
        <div><dt>${tr('firmware')}</dt><dd>${escape(device.firmware)}</dd></div>
        <div><dt>IP</dt><dd>${escape(device.ip)}</dd></div>
        <div><dt>Wi-Fi</dt><dd>${escape(device.wifi?.rssi ?? '—')} dBm</dd></div>
        <div><dt>Loxone</dt><dd>${tr(device.connected ? 'connected' : 'disconnected')}</dd></div>
        <div><dt>${tr('pages')}</dt><dd>${state.project.pages.length} / 10</dd></div>
        <div><dt>${tr('uptime')}</dt><dd>${escape(device.uptime ?? 0)} s</dd></div></dl>
        <div class="actions"><a class="button" href="/editor">${tr('editor')}</a><a class="button secondary" href="/diagnostics">${tr('diagnostics')}</a></div></section>
        <section><h2>${tr('events')}</h2><div id="event-list"></div></section>`;
    for (const event of device.events || []) {
        const line = document.createElement('div');
        line.className = 'event';
        line.textContent = `${event.time || ''} ${event[language] || event.message || ''}`;
        $('#event-list').append(line);
    }
}

function renderSettings() {
    const cfg = state.settings;
    $('#content').innerHTML = `<h1>${tr('settings')}</h1><form id="settings-form"><section><div class="grid">
        ${label('title', input('title', cfg.project.title))}
        ${label('rotation', `<select name="rotation">${[0, 90, 180, 270].map(value => `<option ${cfg.display.rotation === value ? 'selected' : ''}>${value}</option>`).join('')}</select>`)}
        ${label('dayBrightness', input('day', cfg.display.dayBrightness, 'number', 'min="1" max="100"'))}
        ${label('nightBrightness', input('night', cfg.display.nightBrightness, 'number', 'min="0" max="100"'))}
        ${label('ssid', input('ssid', cfg.wifi.ssid))}
        ${label('password', input('password', '', 'password', `placeholder="${tr('passwordKeep')}" autocomplete="new-password"`))}
        ${label('language', `<select name="language"><option value="hu" ${cfg.project.language === 'hu' ? 'selected' : ''}>Magyar</option><option value="en" ${cfg.project.language === 'en' ? 'selected' : ''}>English</option></select>`)}
        </div><p id="canvas-size"></p><div class="actions"><button type="button" id="night-binding">${tr('nightSource')}</button><button>${tr('saveRestart')}</button></div></section></form>`;
    const form = $('#settings-form');
    const displaySize = () => {
        const size = canvasSize(Number(form.elements.rotation.value));
        $('#canvas-size').textContent = `${size.width} × ${size.height} px`;
    };
    form.elements.rotation.onchange = displaySize;
    displaySize();
    let nightSource = cfg.display.nightSource;
    $('#night-binding').onclick = async () => {
        const selected = await chooseBinding(nightSource);
        if (selected !== undefined) nightSource = selected;
    };
    form.onsubmit = async event => {
        event.preventDefault();
        try {
            const next = {project: {title: form.elements.title.value, language: form.elements.language.value},
                display: {rotation: Number(form.elements.rotation.value), dayBrightness: Number(form.elements.day.value), nightBrightness: Number(form.elements.night.value), nightSource},
                wifi: {ssid: form.elements.ssid.value, password: form.elements.password.value || null}};
            state.settings = await api('/api/settings', next);
            feedback(tr('saved'));
        } catch (error) { feedback(error.message, true); }
    };
}

function renderLoxone() {
    const cfg = state.settings.loxone;
    $('#content').innerHTML = `<h1>Loxone</h1><form id="loxone-form"><section><div class="grid">
        ${label('host', input('host', cfg.host))}${label('port', input('port', cfg.port, 'number', 'min="1" max="65535"'))}
        ${label('username', input('username', cfg.username))}${label('password', input('password', '', 'password', `placeholder="${tr('passwordKeep')}"`))}
        <label>WSS / TLS<input name="tls" type="checkbox" ${cfg.tls ? 'checked' : ''}></label>
        <label>CA PEM<textarea name="ca" rows="4">${escape(cfg.ca_pem || '')}</textarea></label>
        </div><div class="actions"><button>${tr('connect')}</button></div><p id="loxone-status"></p></section></form><section><h2>${tr('available')}</h2><p>${state.sources.length} ${tr('value')}</p></section>`;
    const form = $('#loxone-form');
    form.onsubmit = async event => {
        event.preventDefault();
        try {
            await api('/api/loxone', {host: form.elements.host.value.trim(), port: Number(form.elements.port.value),
                username: form.elements.username.value, password: form.elements.password.value || null,
                tls: form.elements.tls.checked, ca_pem: form.elements.ca.value});
            feedback(tr('saved'));
        } catch (error) { feedback(error.message, true); }
    };
}

async function renderGraphics() {
    const manifest = await api('/api/graphics');
    $('#content').innerHTML = `<h1>${tr('graphics')}</h1><section><p>${tr('catalogInfo')}</p><dl class="kv">
        <div><dt>${tr('current')}</dt><dd>${escape(manifest.version)}</dd></div><div><dt>${tr('available')}</dt><dd>${state.catalog.elements.length}</dd></div>
        <div><dt>${language === 'hu' ? 'Egyedi grafika kész' : 'Final artwork ready'}</dt><dd>${state.catalog.elements.filter(item => item.graphicsStatus === 'ready').length} / ${state.catalog.elements.length}</dd></div><div><dt>SHA-256</dt><dd>${manifest.files?.length || 0}</dd></div></dl><div class="actions"><button id="graphics-refresh" ${state.device.development ? 'disabled' : ''}>${tr('refresh')}</button></div><p id="release-status" role="status"></p></section>
        <section><div class="device-list">${Object.entries(state.catalog.categories).map(([id, name]) => `<article><h3>${escape(name[language])}</h3><p>${state.catalog.elements.filter(item => item.category === id).length}</p></article>`).join('')}</div></section>`;
    await refreshReleaseStatus();
    $('#graphics-refresh').onclick = async () => {
        try { await api('/api/graphics/refresh', {}); await refreshReleaseStatus(); }
        catch (error) { feedback(error.message, true); }
    };
}

async function renderBackup() {
    backup = new BackupPanel($('#content'), {api, language, feedback});
    await backup.init();
}

const releaseMessages = {
    "hu": {
        "wifi.unavailable": "Nincs Wi-Fi adatkapcsolat.",
        "clock": "Várakozás az időszinkronra…",
        "clock.unavailable": "Az óra még nincs szinkronizálva. Ellenőrizd az internetelérést, majd próbáld újra.",
        "idle": "Még nem indult frissítés.",
        "release": "Kiadás keresése…",
        "manifest": "Grafikai készlet ellenőrzése…",
        "graphics": "Hiányzó vagy sérült grafikák letöltése…",
        "firmware": "Firmware letöltése és ellenőrzése…",
        "restarting": "Frissítés ellenőrizve, a készülék újraindul. Ezután töltsd újra az oldalt.",
        "complete": "A grafikai készlet naprakész.",
        "release.unavailable": "Nincs elérhető, megfelelő kiadás, vagy nem sikerült kapcsolódni a GitHubhoz.",
        "firmware.required": "Az új grafika új firmware-t igényel. Indítsd a közös frissítést a Frissítés oldalon.",
        "manifest.invalid": "A kiadáshoz tartozó grafikai manifest hibás.",
        "graphics.failed": "A grafikafrissítés megszakadt. Az ellenőrzött fájlok megmaradnak, újrapróbálható.",
        "firmware.failed": "A firmware frissítése nem sikerült. A futó verzió marad aktív.",
        "memory": "Nincs elegendő memória a frissítés indításához."
    },
    "en": {
        "wifi.unavailable": "No Wi-Fi connection.",
        "clock": "Waiting for clock synchronization…",
        "clock.unavailable": "The clock is not synchronized. Check internet connectivity and retry.",
        "idle": "No update started yet.",
        "release": "Checking release…",
        "manifest": "Checking graphics manifest…",
        "graphics": "Downloading missing or damaged graphics…",
        "firmware": "Downloading and validating firmware…",
        "restarting": "Update verified; the device is restarting. Reload this page afterwards.",
        "complete": "Graphics are up to date.",
        "release.unavailable": "No compatible release is available, or GitHub could not be reached.",
        "firmware.required": "New graphics require new firmware. Start the combined update on the Update page.",
        "manifest.invalid": "The release graphics manifest is invalid.",
        "graphics.failed": "Graphics update interrupted. Verified files are retained; you can retry.",
        "firmware.failed": "Firmware update failed. The running version remains active.",
        "memory": "Insufficient memory to start the update."
    }
};

async function refreshReleaseStatus() {
    const target = $('#release-status');
    if (!target || state.device.development || releasePolling || firmwareUploading) return;
    releasePolling = true;
    try {
        const update = await api('/api/update/status');
        if (target !== $('#release-status')) return;
        const details = [releaseMessages[language][update.phase] || update.phase];
        if (update.version) details.push(update.version);
        if (update.busy && update.receivedBytes) details.push(`${(update.receivedBytes / 1048576).toFixed(2)} MB`);
        target.textContent = details.join(' · ');
        for (const id of ['graphics-refresh', 'release-update', 'firmware-upload']) {
            const button = $(`#${id}`);
            if (button) button.disabled = update.busy || firmwareUploading || state.device.capabilities?.ota === false;
        }
    } catch { target.textContent = tr('disconnected'); }
    finally { releasePolling = false; }
}

function renderUpdate() {
    const disabled = state.device.development || state.device.capabilities?.ota === false;
    $('#content').innerHTML = `
        <h1>${tr('update')}</h1>
        <section>
            <h2>${language === 'hu' ? 'Firmware és grafika közös frissítése' : 'Combined firmware and graphics update'}</h2>
            <p>${language === 'hu' ? 'A projekt legújabb GitHub-kiadását telepíti. Csak a hiányzó vagy hibás grafikai fájlokat tölti le.' : 'Installs the latest GitHub release of this project. Downloads only missing or damaged graphics files.'}</p>
            <button id="release-update" ${disabled ? 'disabled' : ''}>
                ${language === 'hu' ? 'Legújabb kiadás telepítése' : 'Install latest release'}
            </button>
            <p id="release-status" role="status"></p>
        </section>
        <section>
            <h2>${tr('firmwareUpload')}</h2>
            <p>${language === 'hu' ? 'Kézi feltöltésnél a hozzá tartozó grafikai készletnek már az SD-n kell lennie. A közös frissítés ezt automatikusan előkészíti.' : 'Manual uploads require the matching graphics set on the SD card. The combined update prepares it automatically.'}</p>
            <input type="file" accept=".bin" id="firmware-file">
            <div class="actions"><button id="firmware-upload" ${disabled ? 'disabled' : ''}>${tr('upload')}</button></div>
            <progress id="upload-progress" value="0" max="100"></progress>
            <p>${escape(state.device.firmware)}</p>
        </section>`;
    void refreshReleaseStatus();
    $('#release-update').onclick = async () => {
        try {
            await api('/api/update/release', {});
            await refreshReleaseStatus();
        } catch (error) {
            feedback(error.message, true);
        }
    };
    $('#firmware-upload').onclick = () => {
        const file = $('#firmware-file').files[0];
        if (!file || firmwareUploading) return;
        firmwareUploading = true;
        $('#firmware-upload').disabled = true;
        $('#release-update').disabled = true;
        const request = new XMLHttpRequest();
        request.open('POST', '/api/update');
        request.setRequestHeader('X-Zolee-Schema', '1');
        request.setRequestHeader('Content-Type', 'application/octet-stream');
        request.upload.onprogress = event => {
            if (event.lengthComputable && $('#upload-progress')) {
                $('#upload-progress').value = event.loaded / event.total * 100;
            }
        };
        request.onload = () => {
            firmwareUploading = false;
            void refreshReleaseStatus();
            const success = language === 'hu'
                ? 'Firmware ellenőrizve, a készülék újraindul.'
                : 'Firmware validated; the device is restarting.';
            feedback(request.status === 200 ? success : request.responseText, request.status !== 200);
        };
        request.onerror = () => {
            firmwareUploading = false;
            void refreshReleaseStatus();
            feedback(tr('disconnected'), true);
        };
        request.send(file);
    };
}

async function renderDiagnostics() {
    diagnostics = new DiagnosticPanel($('#content'), {api, language, escape});
    await diagnostics.refresh();
}

async function route() {
    backup?.destroy();
    backup = null;
    diagnostics?.destroy();
    diagnostics = null;
    editor?.destroy();
    editor = null;
    clearInterval(timer);
    const route = location.pathname.split('/')[1] || 'home';
    document.body.dataset.page = route;
    $('#language').value = language;
    document.documentElement.lang = language;
    $('#navigation').innerHTML = ['home', 'editor', 'settings', 'loxone', 'graphics', 'backup', 'update', 'diagnostics']
        .map(page => `<a data-nav="${page}" href="/${page === 'home' ? '' : page}">${tr(page)}</a>`).join('');
    $('#support-title').textContent = tr('support');
    $('#support-copy').textContent = tr('supportCopy');
    $('#mode-banner').hidden = !state.device.development && !state.device.integrationStatus;
    $('#mode-banner').textContent = state.device.development ? tr('dev') : state.device.integrationStatus?.[language] || state.device.integrationStatus;
    $('#revolut').href = state.device.revolut;
    if (!state.device.revolut) $('#revolut').hidden = true;
    if (route === 'editor') editor = new Editor($('#content'), state, {tr, api, escape, feedback, chooseBinding, language});
    else if (route === 'settings') renderSettings();
    else if (route === 'loxone') renderLoxone();
    else if (route === 'graphics') await renderGraphics();
    else if (route === 'backup') await renderBackup();
    else if (route === 'update') renderUpdate();
    else if (route === 'diagnostics') await renderDiagnostics();
    else renderHome();
    timer = setInterval(async () => {
        // A jelszavas mentés alatt ne torlódjanak a háttér HTTP-kérések.
        if (backup?.busy) return;
        if (route === 'diagnostics') await diagnostics?.refresh();
        if (route === 'graphics' || route === 'update') await refreshReleaseStatus();
        try {
            const live = await api('/api/live');
            const wasConnected = state.device.connected;
            if (live.connected && !wasConnected) state.sources = (await api('/api/loxone/catalog')).catalog;
            state.values = live.values;
            state.device.connected = live.connected;
            editor?.updateValues();
            if (route === 'loxone' && $('#loxone-status')) $('#loxone-status').textContent = live.status || tr(live.connected ? 'connected' : 'disconnected');
        } catch {
            state.device.connected = false;
            editor?.updateValues();
        }
    }, 1000);
}

$('#language').onchange = async event => {
    language = event.target.value;
    localStorage.setItem('schema-language', language);
    await route();
};

try {
    const [device, settings, catalog, project, sources] = await Promise.all([
        api('/api/device'), api('/api/settings'), api('/catalog.json'), api('/api/project'), api('/api/loxone/catalog'),
    ]);
    Object.assign(state, {device, settings, catalog, sources: sources.catalog, project: project.project, revision: project.revision});
    await route();
} catch (error) { feedback(error.message, true); }
