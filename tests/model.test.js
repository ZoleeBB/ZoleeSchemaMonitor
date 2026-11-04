import test from 'node:test';
import assert from 'node:assert/strict';
import {newProject, newElement, validateProject, canvasSize, activityState, exportPage} from '../web/model.js';

const uuid = '12345678-1234-5678-0102030405060708';
const binding = {uuid, scale: 1, offset: 0};
const catalog = {elements: [{id: 'pump', ports: [{x: 0, y: 0.5}, {x: 1, y: 0.5}]}]};

test('explicit activity wins, unknown differs from off, power threshold is strict', () => {
    const pump = newElement('equipment', 'pump');
    pump.power = {source: binding, minimum: 10, maximum: 100};
    assert.equal(activityState(pump, {[uuid]: 10}).active, false);
    assert.equal(activityState(pump, {[uuid]: 11}).active, true);
    assert.equal(activityState(pump, {[uuid]: 200}).speed, 1);
    pump.activity = {...binding, scale: 0};
    assert.equal(activityState(pump, {[uuid]: 50}).active, false);
    assert.equal(activityState(pump, {}).known, false);
    assert.equal(activityState(pump, {[uuid]: 50}, false).known, false);
});

test('all four orientations and ten-page limit', () => {
    assert.deepEqual(canvasSize(0), {width: 800, height: 1280});
    assert.deepEqual(canvasSize(180), canvasSize(0));
    assert.deepEqual(canvasSize(90), {width: 1280, height: 800});
    assert.deepEqual(canvasSize(270), canvasSize(90));
    assert.throws(() => canvasSize(45));
    const project = newProject();
    project.pages = Array.from({length: 10}, (_, n) => ({...project.pages[0], id: `page-${n}`}));
    validateProject(project, catalog);
    project.pages.push({...project.pages[0], id: 'eleven'});
    assert.throws(() => validateProject(project, catalog));
});

test('import rejects secrets, unknown equipment, duplicate ids, invalid power and dangling pipe ports', () => {
    const project = newProject();
    const pump = newElement('equipment', 'pump');
    project.pages[0].elements.push(pump);
    validateProject(project, catalog);
    for (const change of [
        p => p.password = 'secret',
        p => p.pages[0].elements[0].asset = 'untrusted.svg',
        p => p.pages[0].elements.push(p.pages[0].elements[0]),
        p => p.pages[0].elements[0].power = {source: binding, minimum: 100, maximum: 100},
        p => p.pages[0].elements[0].value = {...binding, scale: Infinity},
    ]) {
        const copy = structuredClone(project);
        change(copy);
        assert.throws(() => validateProject(copy, catalog));
    }
    const pipe = newElement('pipe');
    pipe.source = {element: pump.id, port: 20};
    project.pages[0].elements.push(pipe);
    assert.throws(() => validateProject(project, catalog));
});

test('single-page export is detached and round-trips', () => {
    const project = newProject();
    const exported = exportPage(project, 'page-1');
    validateProject(JSON.parse(JSON.stringify(exported)), catalog);
    exported.pages[0].name = 'Changed';
    assert.equal(project.pages[0].name, 'Gépészet');
});
