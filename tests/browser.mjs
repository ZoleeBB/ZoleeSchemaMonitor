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
    assert.deepEqual(errors, []);
    console.log('Chromium: editor, bindings, save/reload, export, portrait, HU/EN OK');
} finally {
    await browser?.close();
    server.kill();
    if (server.exitCode === null) await once(server, 'exit');
    await fs.rm(directory, {recursive: true, force: true});
}
