import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {readFileSync} from 'node:fs';
import {newProject, newElement, validateProject, elementPorts, sprayPoint, SPRAY_DROPS} from '../web/model.js';
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
// Az opcionális szelepállást a böngésző és a készülék azonosan fogadja el.
for (const position of [null,
    {source: {uuid:'12345678-1234-5678-0102030405060708', scale:1, offset:0}, minimum:2, maximum:10, reverse:true},
    {source:null, minimum:0, maximum:100, reverse:false},
    {source: {uuid:'12345678-1234-5678-0102030405060708', scale:1, offset:0}, minimum:10, maximum:10, reverse:false},
    {source: {uuid:'12345678-1234-5678-0102030405060708', scale:1, offset:0}, minimum:0, maximum:100, reverse:1}]) {
    const p = structuredClone(base);
    p.pages[0].elements[0].position = position;
    check(p);
}
const max = newProject();
max.pages = Array.from({length:10}, (_, n) => ({...max.pages[0], id:`p${n}`, elements:Array.from({length:150}, (_, i) => ({...pump,id:`e${i}`}))}));
check(max);
console.log(`Native/browser parity: ${count} cases OK`);

// Minden körszám és üzemmód, a 34. porttal és szigorú fej-validálással.
for (let n=2;n<=16;n++) for(const variant of ['heating','floor','ceiling-heat','ceiling-cool']) {
    const p=newProject(),e=newElement('equipment','configurable-manifold'),pipe=newElement('pipe');
    e.manifold={circuits:n,variant,connections:true,heads:Array.from({length:n},()=>({percent:0,source:null}))};
    pipe.source={element:e.id,port:1+2*n};p.pages[0].elements.push(e,pipe);check(p);
    pipe.source.port++;check(p);pipe.source.port--;e.manifold.connections=false;check(p);
}
for(const mutate of [m=>m.circuits=17,m=>m.heads.pop(),m=>m.heads[0].percent=101,m=>m.heads[0].source={},m=>m.extra=1,m=>m.variant='pool',m=>m.connections=0]) {
    const p=newProject(),e=newElement('equipment','configurable-manifold');p.pages[0].elements.push(e);mutate(e.manifold);check(p);
}
console.log(`Native/browser extended parity: ${count} cases OK`);

// Minden kész típus saját portszámával; a nem csatlakoztatható képek is ide tartoznak.
for(const asset of catalog.elements) {
    const p=newProject(),e=newElement('equipment',asset.id),pipe=newElement('pipe');
    p.pages[0].elements.push(e,pipe);
    const ports=elementPorts(e,asset);
    if(ports.length)pipe.source={element:e.id,port:ports.length-1};
    check(p);
    pipe.source={element:e.id,port:ports.length};check(p);
}
console.log(`Full catalogue native/browser parity: ${count} cases OK`);

// Az igazítás régi exportokban hiányozhat; az új mező csak szöveg/érték típusra érvényes.
for (const type of ['text', 'value', 'pipe', 'equipment']) {
    for (const align of [undefined, 'left', 'center', 'right', '', 'start', 'LEFT', null, false, 1, {}, []]) {
        const p = newProject(), e = newElement(type, type === 'equipment' ? pump.asset : '');
        if (align === undefined) delete e.align;
        else e.align = align;
        p.pages[0].elements.push(e);
        check(p);
    }
}
console.log(`Native/browser alignment parity: ${count} cases OK`);

// A permetpálya geometriáját ugyanazokon a bemeneteken számolja a C és a JS.
const sprayCases = [];
for (const phase of [0, .1, .75, 1, -1.25, 1e308])
    for (const direction of [-150, -90, 0, 270, 1e308])
        for (let drop=0; drop<SPRAY_DROPS; drop++) sprayCases.push([phase, drop, direction]);
const sprayResult = spawnSync(cli, ['spray'], {input: JSON.stringify(sprayCases), encoding:'utf8'});
assert.equal(sprayResult.status, 0, sprayResult.stderr);
const sprayPoints = JSON.parse(sprayResult.stdout);
assert.equal(sprayPoints.length, sprayCases.length);
sprayCases.forEach(([phase, drop, direction], index) => {
    const point = sprayPoint(phase, drop, direction), [valid, x, y] = sprayPoints[index];
    assert.equal(valid, true);
    assert.ok(Math.abs(point.x-x)<1e-12 && Math.abs(point.y-y)<1e-12);
});
console.log(`Native/browser spray geometry: ${sprayCases.length} cases OK`);
