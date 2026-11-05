/* Valódi Chromium: rajzolás, párosítás, tartós mentés, export és tájolás. */
import assert from 'node:assert/strict';
import {spawn} from 'node:child_process';
import fs from 'node:fs/promises';
import os from 'node:os';
import path from 'node:path';
import {once} from 'node:events';

const {chromium} = await import(process.env.SCHEMA_PLAYWRIGHT || 'playwright');
const directory = await fs.mkdtemp(path.join(os.tmpdir(), 'schema-browser-'));
const server = spawn('python3', ['tools/dev_server.py', '--demo', '--port', '0', '--data', path.join(directory, 'state.json')]);
let browser;
try {
    const url = await new Promise((resolve, reject) => {
        let log = '';
        server.stdout.on('data', chunk => {
            log += chunk;
            const match = log.match(/http:\/\/127\.0\.0\.1:\d+/);
            if (match) resolve(match[0]);
        });
        server.once('exit', code => reject(Error(`Development server exited: ${code}`)));
        server.stderr.on('data', chunk => { if (String(chunk).includes('Traceback')) reject(Error(String(chunk))); });
        setTimeout(() => reject(Error('Server startup timeout')), 15000).unref();
    });
    let extraArgs = [];
    if (process.env.SCHEMA_CHROME_ARGS) extraArgs = JSON.parse(process.env.SCHEMA_CHROME_ARGS);
    browser = await chromium.launch({headless: true, executablePath: process.env.SCHEMA_CHROME_PATH || undefined,
        args: ['--no-sandbox', '--disable-dev-shm-usage', ...extraArgs]});
    const page = await browser.newPage({viewport: {width: 1600, height: 1100}});
    const errors = [];
    page.on('pageerror', error => errors.push(error.message));
    await page.goto(url + '/editor');
    await page.waitForSelector('[data-asset="circulation-pump"]');
    assert.equal(await page.locator('[data-asset]').count(), 138);
    await page.locator('[data-asset="circulation-pump"]').click();
    assert.equal(await page.locator('svg [data-element]').count(), 1);
    await page.locator('[data-property="label"]').fill('Szivattyú <teszt>');
    await page.locator('[data-property="label"]').blur();
    await page.locator('[data-binding="power"]').click();
    await page.locator('#binding-options').selectOption('12345678-1234-5678-0102030405060708');
    await page.locator('#binding-apply').click();
    await page.locator('#power-min').fill('10');
    await page.locator('#power-min').blur();
    await page.locator('#show-power').check();
    const bounds = await page.locator('#canvas').boundingBox();
    await page.locator('#draw-pipe').click();
    await page.mouse.click(bounds.x + bounds.width * 0.4, bounds.y + bounds.height * 0.4);
    await page.mouse.dblclick(bounds.x + bounds.width * 0.7, bounds.y + bounds.height * 0.4);
    await page.locator('#save-project').click();
    await page.waitForFunction(() => document.querySelector('#feedback').textContent === 'Tartósan elmentve.');
    const result = await page.request.get(url + '/api/project');
    const saved = await result.json();
    assert.equal(saved.project.pages[0].elements.length, 2);
    assert.equal(saved.project.pages[0].elements[0].power.minimum, 10);
    assert.equal(saved.project.pages[0].elements[0].label, 'Szivattyú <teszt>');
    const downloadEvent = page.waitForEvent('download');
    await page.locator('#export-page').click();
    const download = await downloadEvent;
    const exported = JSON.parse(await fs.readFile(await download.path(), 'utf8'));
    assert.equal(exported.pages[0].elements.length, 2);
    assert.equal(exported.password, undefined);
    await page.reload();
    await page.waitForSelector('svg [data-element]');
    assert.equal(await page.locator('svg [data-element]').count(), 2);
    await page.goto(url + '/settings');
    await page.waitForSelector('#settings-form');
    await page.locator('[name="rotation"]').selectOption('0');
    await page.locator('#settings-form button[type="submit"], #settings-form button:not([type])').click();
    await page.waitForFunction(() => document.querySelector('#feedback').textContent === 'Tartósan elmentve.');
    await page.goto(url + '/editor');
    await page.waitForSelector('#canvas');
    assert.equal(await page.locator('#canvas').getAttribute('viewBox'), '0 0 800 1280');
    await page.locator('#language').selectOption('en');
    await page.waitForFunction(() => document.documentElement.lang === 'en');
    assert.equal(await page.locator('h1').textContent(), 'Editor');
    await fs.mkdir('.browser-test', {recursive: true});
    await page.screenshot({path: '.browser-test/editor-portrait.png', fullPage: true});
    /* A részleges készülékes build képességei se jelenjenek meg kész funkcióként. */
    await page.route('**/api/device', async route => {
        const response = await route.fetch();
        const device = await response.json();
        Object.assign(device, {development: false, capabilities: {ota: false},
            integrationStatus: {hu: 'Készülékes MS1 próba', en: 'Device MS1 preview'}});
        await route.fulfill({response, json: device});
    });
    await page.goto(url + '/update');
    await page.waitForSelector('#firmware-upload:disabled');
    assert.equal(await page.locator('#mode-banner').textContent(), 'Device MS1 preview');
    await page.locator('#language').selectOption('hu');
    await page.waitForFunction(() => document.documentElement.lang === 'hu');
    assert.equal(await page.locator('#mode-banner').textContent(), 'Készülékes MS1 próba');
    assert.equal(await page.locator('#firmware-upload').isDisabled(), true);
    await page.unroute('**/api/device');
    await page.route('**/api/device', async route => {
        const response = await route.fetch();
        const device = await response.json();
        Object.assign(device, {development: false, capabilities: {ota: true}});
        await route.fulfill({response, json: device});
    });
    let update = {busy: false, success: false, phase: 'idle', version: '', receivedBytes: 0};
    await page.route('**/api/update/status', route => route.fulfill({json: update}));
    await page.route('**/api/update/release', async route => {
        assert.deepEqual(route.request().postDataJSON(), {});
        assert.equal(route.request().headers()['x-zolee-schema'], '1');
        update = {busy: true, success: false, phase: 'graphics', version: '0.1.0', receivedBytes: 2048};
        await route.fulfill({status: 202, json: update});
    });
    await page.goto(url + '/update');
    await page.waitForSelector('#release-update:enabled');
    await page.locator('#release-update').click();
    await page.waitForSelector('#release-update:disabled');
    await page.waitForFunction(() => document.querySelector('#release-status').textContent.includes('grafikák letöltése'));
    assert.equal(await page.locator('#firmware-upload').isDisabled(), true);
    update = {busy: false, success: false, phase: 'firmware.failed', version: '0.1.0', receivedBytes: 2048};
    await page.waitForSelector('#release-update:enabled');
    await page.waitForFunction(() => document.querySelector('#release-status').textContent.includes('futó verzió marad'));
    await page.locator('#language').selectOption('en');
    await page.waitForFunction(() => document.querySelector('#release-status').textContent.includes('running version remains'));
    await page.screenshot({path: '.browser-test/update-status.png', fullPage: true});
    let diagnostic = {
        measured: true, cpuValid: false, cpu0: null, cpu1: null, uptime: 90061, sampleAge: 0,
        firmware: '0.1.0-test', clockValid: true, internalFree: 65536, internalTotal: 262144,
        internalMinimum: 32768, psramFree: 30000000, psramTotal: 33554432,
        dmaFree: 16000, dmaLargest: 8000, taskCount: 23, mainStackFree: 4096,
        sdMounted: true, sdSpaceValid: false, sdFree: null, sdTotal: null,
        wifi: {connected: true, ssid: '<b>Home</b>', ip: '192.168.2.123', rssi: -52,
            recovery: true, recoverySsid: 'ZoleeSchema-Test', password: 'NeverRenderThis'},
        loxone: {connected: true, numericAge: 0, knownValues: 42},
        update: {phase: 'idle', phaseHu: 'Nincs frissítés', phaseEn: 'Idle', receivedBytes: 0},
        events: [{uptime: 90061, level: 1, code: 'test', hu: '<img src=x onerror=window.pwned=true>', en: '<img src=x onerror=window.pwned=true>'}]
    };
    await page.route('**/api/diagnostics', route => route.fulfill({json: diagnostic}));
    await page.goto(url + '/diagnostics');
    await page.waitForSelector('[data-diag="cpu"]');
    await page.waitForFunction(() => document.querySelector('[data-diag="wifi"]').textContent.includes('<b>Home</b>'));
    assert.equal(await page.locator('[data-meter="cpu"]').isHidden(), true);
    assert.equal(await page.locator('[data-meter="sd"]').isHidden(), true);
    assert.equal(await page.locator('[data-diag="system"]').textContent(), '0.1.0-test\n25:01:01\nClock valid');
    assert.equal(await page.locator('#diagnostic-events img').count(), 0);
    assert.equal(await page.evaluate(() => window.pwned), undefined);
    assert.equal((await page.locator('body').textContent()).includes('NeverRenderThis'), false);
    await page.evaluate(() => { window.diagCard = document.querySelector('[data-diag-card="cpu"]'); });
    diagnostic = {...diagnostic, cpuValid: true, cpu0: 75, cpu1: 25, sdSpaceValid: true,
        sdTotal: 34359738368, sdFree: 17179869184, sampleAge: 7};
    await page.waitForFunction(() => document.querySelector('[data-diag="cpu"]').textContent.includes('75%'));
    assert.equal(await page.locator('[data-meter="cpu"]').evaluate(node => node.value), 75);
    assert.equal(await page.locator('[data-meter="sd"]').evaluate(node => node.value), 50);
    assert.equal(await page.evaluate(() => window.diagCard === document.querySelector('[data-diag-card="cpu"]')), true);
    assert.equal((await page.locator('#diagnostics-status').textContent()).includes('7 s'), true);
    await page.locator('#language').selectOption('hu');
    await page.waitForFunction(() => document.querySelector('h1').textContent === 'Rendszerdiagnosztika');
    await page.waitForFunction(() => document.querySelector('[data-diag="update"]').textContent.includes('Nincs frissítés'));
    await page.screenshot({path: '.browser-test/diagnostics-hu.png', fullPage: true});
    {
        await page.route('**/api/backup/status', route => route.fulfill({json: {available: true, revision: 7}}));
        const envelope = {format: 'zolee-backup', version: 1, data: 'Ciphertext fixture'};
        let exports = 0, restores = 0;
        await page.route('**/api/backup/export', async route => {
            exports++;
            assert.deepEqual(route.request().postDataJSON(), {passphrase: 'Backup password 123!'});
            assert.equal(route.request().headers()['x-zolee-schema'], '1');
            await route.fulfill({json: envelope});
        });
        await page.route('**/api/backup/restore', async route => {
            restores++;
            const payload = route.request().postDataJSON();
            assert.equal(payload.revision, 7);
            assert.deepEqual(payload.backup, envelope);
            if (payload.passphrase !== 'Backup password 123!') {
                await route.fulfill({status: 400, json: {error: 'backup.invalid-or-password'}});
            } else await route.fulfill({json: {revision: 8, restart: true}});
        });
        await page.goto(url + '/backup');
        await page.waitForSelector('#device-backup-form button:enabled');
        const exportForm = page.locator('#device-backup-form');
        await exportForm.locator('[name=passphrase]').fill('Backup password 123!');
        await exportForm.locator('[name=repeat]').fill('Different password 123!');
        await exportForm.locator('button').click();
        assert.equal(exports, 0);
        await exportForm.locator('[name=repeat]').fill('Backup password 123!');
        const downloadEvent = page.waitForEvent('download');
        await exportForm.locator('button').click();
        const download = await downloadEvent;
        assert.deepEqual(JSON.parse(await fs.readFile(await download.path(), 'utf8')), envelope);
        assert.equal(await exportForm.locator('[name=passphrase]').inputValue(), '');
        const restoreForm = page.locator('#device-restore-form');
        await restoreForm.locator('[name=file]').setInputFiles({name: 'backup.json', mimeType: 'application/json', buffer: Buffer.from(JSON.stringify(envelope))});
        await restoreForm.locator('[name=passphrase]').fill('Wrong password 123!');
        await restoreForm.locator('button').click();
        await page.waitForFunction(() => document.querySelector('#device-restore-form button').disabled === false && document.querySelector('#device-restore-form [name=passphrase]').value === '');
        assert.equal(restores, 1);
        await restoreForm.locator('[name=passphrase]').fill('Backup password 123!');
        await restoreForm.locator('button').click();
        await page.waitForFunction(() => document.querySelector('#device-restore-form button').disabled && document.querySelector('#device-restore-form [name=passphrase]').value === '');
        assert.equal(restores, 2);
        await page.screenshot({path: '.browser-test/backup-hu.png', fullPage: true});
    }
    assert.deepEqual(errors, []);
    console.log('Chromium: editor, bindings, save/reload, export, portrait, HU/EN, device capabilities and update status/retry and diagnostic live refresh/HU-EN/escaping OK');
} finally {
    await browser?.close();
    server.kill();
    if (server.exitCode === null) await once(server, 'exit');
    await fs.rm(directory, {recursive: true, force: true});
}
