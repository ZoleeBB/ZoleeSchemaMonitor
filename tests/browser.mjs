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
    const saveProjectAndWait = async () => {
        const response = page.waitForResponse(r => r.url() === url + '/api/project' && r.request().method() === 'POST');
        await page.locator('#save-project').click();
        assert.equal((await response).status(), 200);
        await page.waitForFunction(() => !document.querySelector('#save-project').classList.contains('dirty'));
    };
    const errors = [];
    page.on('pageerror', error => errors.push(error.message));
    await page.goto(url + '/editor');
    await page.waitForSelector('[data-asset="circulation-pump"]');
    assert.equal((await page.locator('[data-asset]').count()) >= 300, true);
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
    const thickness = page.locator('[data-property="width"]');
    assert.equal(await thickness.getAttribute('min'), '1');
    assert.equal(await thickness.getAttribute('max'), '30');
    for (const width of [1, 12, 30]) {
        await thickness.fill(String(width));
        await thickness.blur();
        assert.equal(await page.locator('svg .pipe polyline').first().getAttribute('stroke-width'), String(width));
        await saveProjectAndWait();
        const persisted = await (await page.request.get(url + '/api/project')).json();
        assert.equal(persisted.project.pages[0].elements.find(e => e.type === 'pipe').width, width);
    }
    await saveProjectAndWait();
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
        displayFps: 19.8, displayMaxRenderMs: 41.2,
        sdTotal: 34359738368, sdFree: 17179869184, sampleAge: 7};
    await page.waitForFunction(() => document.querySelector('[data-diag="cpu"]').textContent.includes('75%'));
    assert.equal(await page.locator('[data-meter="cpu"]').evaluate(node => node.value), 75);
    assert.equal(await page.locator('[data-meter="sd"]').evaluate(node => node.value), 50);
    assert.equal(await page.locator('[data-diag="fps"]').textContent(), '19.8 FPS\nLongest render: 41.2 ms');
    assert.equal(await page.evaluate(() => window.diagCard === document.querySelector('[data-diag-card="cpu"]')), true);
    assert.equal((await page.locator('#diagnostics-status').textContent()).includes('7 s'), true);
    await page.locator('#language').selectOption('hu');
    await page.waitForFunction(() => document.querySelector('h1').textContent === 'Rendszerdiagnosztika');
    await page.waitForFunction(() => document.querySelector('[data-diag="update"]').textContent.includes('Nincs frissítés'));
    await page.screenshot({path: '.browser-test/diagnostics-hu.png', fullPage: true});
    {
        await page.route('**/api/backup/status', route => route.fulfill({json: {available: true, revision: 7}}));
        const envelope = {format: 'zolee-backup', version: 1, data: 'Ciphertext fixture'};
        let exports = 0, restores = 0, duringExport = false, pollsDuringExport = 0;
        await page.route('**/api/live', async route => {
            if (duringExport) pollsDuringExport++;
            await route.continue();
        });
        await page.route('**/api/backup/export', async route => {
            exports++;
            assert.deepEqual(route.request().postDataJSON(), {passphrase: 'Backup password 123!'});
            assert.equal(route.request().headers()['x-zolee-schema'], '1');
            duringExport = true;
            await new Promise(resolve => setTimeout(resolve, 1600));
            duringExport = false;
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
        assert.equal(pollsDuringExport, 0);
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
    await page.goto(url + '/editor');
    await page.waitForSelector('[data-asset="well-pump"]');
    await page.locator('[data-asset="well-pump"]').click();
    await page.waitForSelector('[data-status-indicator]');
    await page.locator('[data-binding="activity"]').click();
    await page.locator('#binding-options').selectOption('12345678-1234-5678-0102030405060708');
    await page.locator('#binding-apply').click();
    let pumpLive = {connected: true, values: {'12345678-1234-5678-0102030405060708': 1}, status: 'test'};
    await page.route('**/api/live', route => route.fulfill({json: pumpLive}));
    await page.waitForFunction(() => document.querySelector('[data-status-indicator]').getAttribute('fill') === '#36c678');
    const pulse = await page.locator('[data-status-indicator]').getAttribute('opacity');
    await page.waitForFunction(value => document.querySelector('[data-status-indicator]').getAttribute('opacity') !== value, pulse);
    pumpLive.values['12345678-1234-5678-0102030405060708'] = 0;
    await page.waitForFunction(() => document.querySelector('[data-status-indicator]').getAttribute('fill') === '#596674');
    pumpLive.connected = false;
    await page.waitForFunction(() => document.querySelector('[data-status-indicator]').getAttribute('fill') === '#d9a23a');
    await page.screenshot({path: '.browser-test/editor-pumps.png', fullPage: true});
    await page.locator('[data-asset="configurable-manifold"] img').evaluate(image => image.decode());
    await page.locator('[data-asset="configurable-manifold"]').click();
    const manifold = page.locator('svg [data-element]').last();
    for (let n=2; n<=16; n++) {
        await page.locator('#manifold-circuits').selectOption(String(n));
        const imageSize = await manifold.locator('image').evaluate(async node => {
            const img = new Image(); img.src = node.href.baseVal; await img.decode();
            return [img.naturalWidth, img.naturalHeight];
        });
        assert.deepEqual(imageSize, [96 + 56*n, 224]);
        assert.equal(await manifold.locator('[data-head-indicator]').count(),n);
        assert.equal(await manifold.locator('[data-port]').count(),2+2*n);
    }
    for (const variant of ['heating','floor','ceiling-heat','ceiling-cool']) await page.locator('#manifold-variant').selectOption(variant);
    await page.locator('[data-head-percent="1"]').fill('25');
    await page.locator('[data-head-percent="1"]').blur();
    assert.equal(await manifold.locator('[data-head-indicator="0"]').getAttribute('fill'),'#26333d');
    assert.equal(await manifold.locator('[data-head-indicator="1"]').getAttribute('fill'),'#36c678');
    await page.locator('[data-head-source="0"]').click();
    await page.locator('#binding-options').selectOption('12345678-1234-5678-0102030405060708');
    await page.locator('#binding-apply').click();
    await page.waitForFunction(()=>document.querySelector('[data-head-indicator="0"]').getAttribute('fill')==='#d9a23a');
    pumpLive.connected=true; pumpLive.values['12345678-1234-5678-0102030405060708']=.001;
    await page.waitForFunction(()=>document.querySelector('[data-head-indicator="0"]').getAttribute('fill')==='#36c678');
    pumpLive.values['12345678-1234-5678-0102030405060708']=0;
    await page.waitForFunction(()=>document.querySelector('[data-head-indicator="0"]').getAttribute('fill')==='#26333d');
    // A legutolsó, 16. kör visszatérő portja is ténylegesen csőhöz köthető.
    await page.locator('#draw-pipe').click();
    await manifold.locator('[data-port="33"]').click();
    const pipeBounds = await page.locator('#canvas').boundingBox();
    await page.mouse.dblclick(pipeBounds.x+pipeBounds.width*.85,pipeBounds.y+pipeBounds.height*.65);
    await saveProjectAndWait();
    await page.waitForFunction(()=>document.querySelector('#feedback').textContent==='Tartósan elmentve.');
    let manifoldSaved=await (await page.request.get(url+'/api/project')).json();
    const savedPipe=manifoldSaved.project.pages[0].elements.at(-1);
    assert.equal(savedPipe.source.port,33);
    // Kijelölés csőkészítés után; kikapcsolás leválasztja a csövet, megőrzi a rajzot.
    await page.locator('svg [data-head-indicator="1"]').click();
    await page.locator('#element-rotation').selectOption('90');
    await saveProjectAndWait();
    await page.waitForFunction(()=>document.querySelector('#feedback').textContent==='Tartósan elmentve.');
    const rotatedProject=await (await page.request.get(url+'/api/project')).json();
    const beforeRejectedResize=rotatedProject.project.pages[0].elements.at(-1).points;
    const originalHeight=await page.locator('[data-property="h"]').inputValue();
    await page.locator('[data-property="h"]').fill('0.9');
    await page.locator('[data-property="h"]').blur();
    assert.equal(await page.locator('[data-property="h"]').inputValue(),originalHeight);
    await saveProjectAndWait();
    await page.waitForFunction(()=>document.querySelector('#feedback').textContent==='Tartósan elmentve.');
    const afterRejectedResize=await (await page.request.get(url+'/api/project')).json();
    assert.deepEqual(afterRejectedResize.project.pages[0].elements.at(-1).points,beforeRejectedResize);
    await page.locator('#manifold-connections').uncheck();
    assert.equal(await page.locator('svg [data-head-indicator="1"]').count(),1);
    assert.equal(await page.locator('svg [data-head-indicator="1"]').evaluate(n=>n.closest('[data-element]').querySelectorAll('[data-port]').length),0);
    for (const angle of ['90','180','270','0']) await page.locator('#element-rotation').selectOption(angle);
    await page.locator('#night').click();
    await page.screenshot({path:'.browser-test/manifold-dark.png',fullPage:true});
    await page.locator('#night').click();
    await page.screenshot({path:'.browser-test/manifold-light.png',fullPage:true});
    await saveProjectAndWait();
    await page.waitForFunction(()=>document.querySelector('#feedback').textContent==='Tartósan elmentve.');
    manifoldSaved=await (await page.request.get(url+'/api/project')).json();
    assert.equal(manifoldSaved.project.pages[0].elements.at(-1).source,null);
    await page.reload();
    await page.waitForSelector('[data-head-indicator="15"]');
    assert.equal(await page.locator('[data-head-label="1"]').textContent(),'25%');
    // A teljes készlet valódi böngészőben is dekódolható, nem csupán létező fájl.
    const decoded=await page.locator('#library img').evaluateAll(async images=>{
        await Promise.all(images.map(image=>image.decode()));
        return images.map(image=>[image.closest("[data-asset]").dataset.asset,image.naturalWidth,image.naturalHeight]);
    });
    const catalog = JSON.parse(await fs.readFile("web/catalog.json", "utf8"));
    assert.deepEqual(decoded.map(([id]) => id).sort(), catalog.elements.map(asset => asset.id).sort());
    for (const [id,w,h] of decoded) {
        const n = id === 'configurable-manifold' ? 4 : Number(/^manifold-(\d+)$/.exec(id)?.[1]);
        assert.deepEqual([w,h], n ? [96 + 56*n,224] : [160,160], id);
    }
    await page.locator('[data-asset="dry-cooler"]').click();
    const cooler=page.locator('svg [data-element]').last();
    assert.equal(await cooler.locator('[data-rotor]').count(),3);
    await page.locator('[data-binding="activity"]').click();
    await page.locator('#binding-options').selectOption('12345678-1234-5678-0102030405060708');
    await page.locator('#binding-apply').click();
    pumpLive.connected=true;pumpLive.values['12345678-1234-5678-0102030405060708']=1;
    await page.waitForFunction(()=>[...document.querySelectorAll('svg [data-element]')].at(-1).dataset.active==='true');
    await page.waitForFunction(()=>[...document.querySelectorAll('svg [data-element]')].at(-1).querySelectorAll('[data-rotor][transform]').length===3);
    const transforms=await cooler.locator('[data-rotor]').evaluateAll(nodes=>nodes.map(n=>n.getAttribute('transform')));
    assert.equal(transforms.length,3);
    await page.locator('[data-asset="pool-light"]').click();
    assert.equal(await page.locator('svg [data-element]').last().locator('[data-port]').count(),0);
    // Az igazítás a mező szélességéhez kötött, forgatáskor is a helyi koordinátáiban marad.
    for (const type of ['text', 'value']) {
        await page.locator(type === 'text' ? '#add-text' : '#add-value').click();
        if (type === 'text') {
            await page.locator('[data-property="text"]').fill('Igazítás <teszt>\nÁrvíztűrő tükörfúrógép');
            await page.locator('[data-property="text"]').blur();
            const spans = page.locator('svg .selected text tspan');
            assert.ok(await spans.count() >= 2);
            assert.ok(await spans.evaluateAll(nodes => nodes.every(node => Number(node.getAttribute('textLength')) >= 0)));
            assert.equal(await page.locator('svg .selected text').getAttribute('data-field-text'),
                'Igazítás <teszt>\nÁrvíztűrő tükörfúrógép');
        } else {
            await page.locator('[data-binding="value"]').click();
            await page.locator('#binding-options').selectOption('12345678-1234-5678-0102030405060708');
            await page.locator('#binding-apply').click();
        }
        for (const [align, anchor, fraction] of [['left', 'start', 0], ['center', 'middle', .5], ['right', 'end', 1]]) {
            await page.locator('[data-property="align"]').selectOption(align);
            for (const rotation of ['0', '90', '180', '270']) {
                await page.locator('#element-rotation').selectOption(rotation);
                const geometry = await page.locator('svg .selected text').evaluate(node => ({
                    anchor: node.getAttribute('text-anchor'), x: Number(node.getAttribute('x')),
                    width: Number(node.closest('[data-element]').querySelector('.selected-box').getAttribute('width')),
                }));
                assert.equal(geometry.anchor, anchor);
                assert.ok(Math.abs(geometry.x - geometry.width * fraction) < 1e-8);
            }
        }
        await saveProjectAndWait();
        const savedAlignment = await (await page.request.get(url + '/api/project')).json();
        assert.equal(savedAlignment.project.pages[0].elements.at(-1).align, 'right');
        await page.reload();
        await page.waitForSelector('svg [data-element]');
        const text = page.locator('svg [data-element]').last().locator('text');
        assert.equal(await text.getAttribute('text-anchor'), 'end');
    }
    // Minden permetező fej: ténylegesen mozgó cseppek, nulla és adathiány külön állapot.
    for (const asset of ['rotor-sprinkler', 'spray-sprinkler', 'micro-sprinkler', 'rotary-nozzle', 'impact-sprinkler']) {
        await page.locator(`[data-asset="${asset}"]`).click();
        await page.locator('[data-binding="activity"]').click();
        await page.locator('#binding-options').selectOption('12345678-1234-5678-0102030405060708');
        await page.locator('#binding-apply').click();
        const spray = page.locator('svg [data-element]').last();
        assert.equal(await spray.locator('[data-spray]').count(), 4);
        pumpLive.connected=true;pumpLive.values['12345678-1234-5678-0102030405060708']=1;
        await page.waitForFunction(()=>[...document.querySelectorAll('svg [data-element]')].at(-1).querySelector('[data-spray]').style.display==='');
        const first = await spray.locator('[data-spray="0"]').getAttribute('cx');
        await page.waitForFunction(old=>[...document.querySelectorAll('svg [data-element]')].at(-1).querySelector('[data-spray="0"]').getAttribute('cx')!==old,first);
        for (const rotation of ['90','180','270','0']) await page.locator('#element-rotation').selectOption(rotation);
        pumpLive.values['12345678-1234-5678-0102030405060708']=0;
        await page.waitForFunction(()=>{const group=[...document.querySelectorAll('svg [data-element]')].at(-1);return group.dataset.active==='false' && group.dataset.known==='true' && group.querySelector('[data-status-indicator]').getAttribute('fill')==='#596674';});
        assert.ok(await spray.locator('[data-spray]').evaluateAll(nodes=>nodes.every(n=>n.style.display==='none')));
        delete pumpLive.values['12345678-1234-5678-0102030405060708'];
        await page.waitForFunction(()=>{const group=[...document.querySelectorAll('svg [data-element]')].at(-1);return group.dataset.known==='false' && group.querySelector('[data-status-indicator]').getAttribute('fill')==='#d9a23a';});
        assert.ok(await spray.locator('[data-spray]').evaluateAll(nodes=>nodes.every(n=>n.style.display==='none')));
    }
    // A teljesítményfelirat a megadott tartományhoz képesti százalék, nem a nyers forrás.
    await page.locator('[data-asset="circulation-pump"]').click();
    await page.locator('[data-binding="power"]').click();
    await page.locator('#binding-options').selectOption('12345678-1234-5678-0102030405060708');
    await page.locator('#binding-apply').click();
    await page.locator('#power-min').fill('10');await page.locator('#power-min').blur();
    await page.locator('#power-max').fill('100');await page.locator('#power-max').blur();
    await page.locator('#show-power').check();
    for (const [raw, expected] of [[45,'39 %'],[150,'100 %'],[0,'0 %'],[undefined,'— %']]) {
        if (raw === undefined) delete pumpLive.values['12345678-1234-5678-0102030405060708'];
        else pumpLive.values['12345678-1234-5678-0102030405060708']=raw;
        await page.waitForFunction(expected=>[...document.querySelectorAll('svg [data-element]')].at(-1).querySelector('[data-live-power]').textContent===expected,expected);
    }
    await page.locator('#add-value').click();
    await page.locator('[data-property="unit"]').fill('°C');await page.locator('[data-property="unit"]').blur();
    await page.locator('[data-binding="value"]').click();
    await page.locator('#binding-options').selectOption('12345678-1234-5678-0102030405060708');
    await page.locator('#binding-apply').click();
    for (const [raw, expected] of [[-.0001,'0.0 °C'],[2.55,'2.6 °C'],[1e12,'1e+12 °C']]) {
        pumpLive.values['12345678-1234-5678-0102030405060708']=raw;
        await page.waitForFunction(expected=>[...document.querySelectorAll('svg [data-element]')].at(-1).querySelector('[data-live-value]').textContent===expected,expected);
    }
    // Valós elrendezés hat szélességen, minden oldalon, külső erőforrások nélkül.
    const responsive = await browser.newPage();
    const externalRequests = [];
    await responsive.route('**/*', route => {
        if (new URL(route.request().url()).origin !== url) {
            externalRequests.push(route.request().url());
            return route.abort();
        }
        return route.continue();
    });
    responsive.on('pageerror', error => errors.push(error.message));
    for (const width of [360, 390, 768, 1024, 1280, 1920]) {
        await responsive.setViewportSize({width, height: 1000});
        for (const route of ['home', 'editor', 'settings', 'loxone', 'graphics', 'backup', 'update', 'diagnostics']) {
            await responsive.goto(url + '/' + (route === 'home' ? '' : route));
            await responsive.waitForSelector('#content h1');
            assert.equal(await responsive.locator('#navigation [aria-current="page"]').getAttribute('data-nav'), route);
            const overflow = await responsive.evaluate(() => document.documentElement.scrollWidth - innerWidth);
            assert.ok(overflow <= 1, `${route} overflows at ${width}px by ${overflow}px`);
            if (route === 'settings') assert.equal(await responsive.locator('#settings-form fieldset').count(), 3);
        }
    }
    await responsive.setViewportSize({width: 390, height: 844});
    await responsive.emulateMedia({colorScheme: 'dark'});
    await responsive.goto(url + '/editor');
    await responsive.waitForSelector('[data-asset="configurable-manifold"]');
    await responsive.locator('#language').selectOption('en');
    await responsive.waitForFunction(() => document.documentElement.lang === 'en');
    await responsive.locator('[data-asset="configurable-manifold"]').click();
    await responsive.locator('#manifold-circuits').selectOption('16');
    await responsive.locator('[data-head-percent="15"]').fill('50');
    await responsive.locator('[data-head-percent="15"]').blur();
    assert.equal(await responsive.locator('[data-head-label="15"]').last().textContent(), '50%');
    assert.ok(await responsive.evaluate(() => document.documentElement.scrollWidth <= innerWidth + 1));
    await responsive.locator('[data-head-source="15"]').click();
    await responsive.waitForSelector('#binding-dialog[open]');
    assert.ok(await responsive.locator('#binding-dialog').evaluate(el => el.getBoundingClientRect().right <= innerWidth));
    await responsive.locator('#binding-cancel').click();
    assert.deepEqual(externalRequests, []);
    await responsive.route('**/api/diagnostics', route => route.fulfill({json: {
        measured: true, sdMounted: false, sdSpaceValid: false
    }}));
    await responsive.route('**/api/storage/remount', route => {
        assert.equal(route.request().method(), 'POST');
        return route.fulfill({status: 202, json: {restart: true}});
    });
    await responsive.goto(url + '/diagnostics');
    await responsive.locator('#sd-recovery button').click();
    await responsive.waitForFunction(() => document.querySelector('#sd-recovery span').textContent === 'Restarting the display.');
    assert.equal(await responsive.locator('#sd-recovery button').isDisabled(), true);
    await responsive.close();
    assert.deepEqual(errors, []);
    console.log('Chromium: 2–16 circuit manifolds, ports, bindings, save/reload, export, rotation, five animated spray heads with off/unknown states, 48 responsive route/viewport cases, dark mobile HU/EN, no external resources, device capabilities, updates and diagnostics OK');
} finally {
    await browser?.close();
    server.kill();
    if (server.exitCode === null) await once(server, 'exit');
    await fs.rm(directory, {recursive: true, force: true});
}
