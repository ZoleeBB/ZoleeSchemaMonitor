// A tesztadat az éles szerkesztő importján és letöltési gombjain halad át.
import assert from 'node:assert/strict';
import fs from 'node:fs/promises';
import path from 'node:path';
import {newElement, newProject, newManifold, LIMITS} from '../web/model.js';

const {chromium} = await import(process.env.SCHEMA_PLAYWRIGHT || 'playwright');
const scenarios = [
    ['kazan', 'Kazánház', 1, 20, ['gas-boiler', 'circulation-pump', 'motor-valve-3']],
    ['medence', 'Medence vízkezelése', 1, 40, ['pool-pump', 'pool-heat-pump', 'pool-bypass']],
    ['ontozes', 'Öntözési zónák', 2, 60, ['well-pump', 'motor-valve-2', 'rotor-sprinkler']],
    ['legtechnika', 'Légkezelés', 2, 60, ['duct-fan', 'supply-fan', 'extract-fan']],
    ['osztok', 'Padlófűtési körök', 4, 80, ['configurable-manifold', 'circulation-pump', 'motor-valve-3']],
    ['terheles', 'Vegyes gépészet', 10, 150, ['circulation-pump', 'heat-pump', 'spray-sprinkler', 'motor-valve-3']]
];
const source = {uuid: '12345678-1234-5678-0102030405060708', scale: 1, offset: 0};
const catalog = JSON.parse(await fs.readFile('web/catalog.json', 'utf8')).elements;
const output = process.argv[3] || 'tests/schemas';
const browser = await chromium.launch({headless: true, executablePath: process.env.SCHEMA_CHROME_PATH,
    args: ['--no-sandbox', '--disable-dev-shm-usage', ...(JSON.parse(process.env.SCHEMA_CHROME_ARGS || '[]'))]});

function buildPage(title, index, count, assets) {
    const rows = count / 5;
    const columns = Math.ceil(Math.sqrt(rows * 1.6));
    const lines = Math.ceil(rows / columns);
    const elements = [];
    for (let i = 0; i < rows; i++) {
        const x = (i % columns + .05) / columns, y = (Math.floor(i / columns) + .05) / lines;
        const w = .42 / columns, h = .5 / lines;
        const asset = assets[(i + index) % assets.length];
        const unit = Object.assign(newElement('equipment', asset), {id: 'unit-' + i, x, y, w, h,
            activity: source, power: {source, minimum: 0, maximum: 100}, showPower: true});
        if (unit.manifold) {
            unit.manifold = newManifold(2 + (i + index) % 15);
            for (const head of unit.manifold.heads) head.source = source;
        }
        if (asset.startsWith('motor-valve')) unit.position = {source, minimum: 0, maximum: 100, reverse: false};
        const cy = y + h / 2, end = x + .85 / columns;
        const pipe = Object.assign(newElement('pipe'), {id: 'supply-' + i, x, y, w, h,
            activity: source, width: 3, points: [{x: x+w/2, y: cy}, {x: end, y: cy}, {x: end, y}]});
        if (catalog.find(item => item.id === asset).ports.length || unit.manifold)
            pipe.source = {element: unit.id, port: 0};
        const back = Object.assign(newElement('pipe'), {id: 'return-' + i, x, y, w, h,
            activity: source, color: '#3991c7', width: 3, direction: -1,
            points: [{x: end, y: y+h}, {x: x+w/2, y: y+h}, {x: x+w/2, y: cy}]});
        const label = Object.assign(newElement('text'), {id: 'label-' + i, x, y: y+h, w: .9/columns,
            h: .18/lines, text: title + ' ' + (i+1), fontSize: 12});
        const value = Object.assign(newElement('value'), {id: 'value-' + i, x: x+w, y, w, h: .2/lines,
            value: source, unit: '%', decimals: 0, fontSize: 16});
        elements.push(pipe, back, unit, label, value);
    }
    return {id: 'lap-' + (index+1), name: title + ' ' + (index+1),
        background: ['cool','warm','plain'][index % 3], elements};
}

try {
    const page = await browser.newPage({viewport: {width: 1280, height: 800}});
    const errors = [];
    page.on('pageerror', error => errors.push(error.message));
    await page.goto((process.argv[2] || 'http://127.0.0.1:8080') + '/editor');
    await page.waitForSelector('#export-project');
    async function importFile(file) {
        const confirmed = page.waitForEvent('dialog').then(dialog => dialog.accept());
        await page.locator('#import-file').setInputFiles(file);
        await confirmed;
        await page.waitForFunction(() => document.querySelector('#import-file').files.length === 0);
    }
    async function download(button) {
        const pending = page.waitForEvent('download');
        await page.locator(button).click();
        return pending;
    }
    for (const [slug,title,pages,count,assets] of scenarios) {
        const dir = path.join(output, slug);
        await fs.mkdir(dir, {recursive: true});
        const project = {...newProject(), title, pages: Array.from({length:pages}, (_,i) => buildPage(title,i,count,assets))};
        await importFile({name:'input.json', mimeType:'application/json', buffer:Buffer.from(JSON.stringify(project))});
        const files = [];
        for (let i=0; i<pages; i++) {
            await page.locator('[data-page-index="'+i+'"]').click();
            assert.equal(await page.locator('#canvas [data-element]').count(), count);
            const file = path.join(dir, 'lap-' + (i+1) + '.json');
            await (await download('#export-page')).saveAs(file);
            files.push(file);
        }
        const full = path.join(dir, 'projekt.json');
        await (await download('#export-project')).saveAs(full);
        const saved = JSON.parse(await fs.readFile(full));
        assert.equal(saved.pages.length, pages);
        for (const [i,file] of files.entries()) assert.deepEqual(JSON.parse(await fs.readFile(file)).pages, [saved.pages[i]]);
        for (const file of [full,...files]) {
            assert.ok((await fs.stat(file)).size <= LIMITS.bytes);
            await importFile(file);
            const again = await download('#export-project');
            assert.deepEqual(JSON.parse(await fs.readFile(await again.path())), JSON.parse(await fs.readFile(file)));
        }
        console.log(slug, pages, count, 'OK');
    }
    assert.deepEqual(errors, []);
} finally {
    await browser.close();
}
