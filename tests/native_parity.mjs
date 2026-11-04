import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {readFileSync} from 'node:fs';
import {newProject, newElement, validateProject} from '../web/model.js';
const catalog = JSON.parse(readFileSync('web/catalog.json'));
const cli = process.argv[2];
let count = 0;
function check(project, raw) {
    let valid = true;
    try { validateProject(project, catalog); } catch { valid = false; }
    const result = spawnSync(cli, {input: raw ?? JSON.stringify(project), encoding: 'utf8'});
    assert.equal(result.status === 0, valid, result.stdout + result.stderr);
    count++;
}
const base = newProject();
const pump = newElement('equipment', catalog.elements.find(e => e.family === 'pump').id);
const pipe = newElement('pipe');
pipe.source = {element: pump.id, port: 0};
base.pages[0].elements.push(pump, pipe);
check(base);
for (const mutate of [
    p => p.password = 'secret',
    p => p.pages = [],
    p => p.pages = Array.from({length: 11}, (_, n) => ({...p.pages[0], id: `p${n}`})),
    p => p.pages[0].elements[0].asset = 'unknown.svg',
    p => p.pages[0].elements.push(p.pages[0].elements[0]),
    p => p.pages[0].elements[0].power = {source: {uuid:'12345678-1234-5678-0102030405060708', scale:1, offset:0}, minimum:10, maximum:10},
    p => p.pages[0].elements[1].source.port = 31,
    p => p.pages[0].elements[0].x = .99,
    p => p.pages[0].elements[0].rotation = 45,
    p => p.pages[0].name = '😀'.repeat(41),
    p => p.pages[0].elements[0].label = 'árvíztűrő tükörfúrógép',
    p => p.pages[0].elements[0].unit = '°C',
    p => p.pages[0].elements[0].value = {uuid:'12345678-1234-5678-0102030405060708', scale:1, offset:0},
    p => p.pages[0].elements[0].value = {uuid:'12345678-1234-5678-0102-030405060708', scale:1, offset:0},
    p => p.pages[0].elements[0]['id|type'] = 'wrong',
    p => p.pages[0].elements[0].text = '\u0000hidden',
]) { const p = structuredClone(base); mutate(p); check(p); }
for (const raw of ['{} {}', JSON.stringify(base) + 'garbage', JSON.stringify(base).replace('"version":1', '"version":1,"version":1'), Buffer.from([123,34,120,34,58,34,0xc0,0x80,34,125])]) {
    assert.notEqual(spawnSync(cli, {input:raw}).status, 0); count++;
}
const max = newProject();
max.pages = Array.from({length:10}, (_, n) => ({...max.pages[0], id:`p${n}`, elements:Array.from({length:150}, (_, i) => ({...pump,id:`e${i}`}))}));
check(max);
console.log(`Native/browser parity: ${count} cases OK`);
