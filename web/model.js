/* Közös, böngészőben és Node alatt is tesztelhető projektmodell. */
export const LIMITS = Object.freeze({pages: 10, elements: 150, points: 64, bytes: 1048576});
export const UUID = /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{16}$/i;
const ID = /^[a-zA-Z0-9_-]{1,64}$/;
const COLOR = /^#[0-9a-f]{6}$/i;
const clone = value => JSON.parse(JSON.stringify(value));

/* Helyi HTTP-n a randomUUID nem minden böngészőben érhető el. */
export function newId(prefix = 'e') {
    const random = crypto.getRandomValues(new Uint8Array(16));
    return `${prefix}-${Array.from(random, byte => byte.toString(16).padStart(2, '0')).join('')}`;
}

export function canvasSize(rotation) {
    if (![0, 90, 180, 270].includes(rotation)) throw Error('Érvénytelen tájolás / Invalid rotation');
    return rotation === 90 || rotation === 270 ? {width: 1280, height: 800} : {width: 800, height: 1280};
}

function object(value, keys, path) {
    if (!value || typeof value !== 'object' || Array.isArray(value)) throw Error(path);
    for (const key of Object.keys(value)) {
        if (!keys.includes(key)) throw Error(`${path}: ismeretlen mező / unknown field: ${key}`);
    }
}

function number(value, min, max, path) {
    if (typeof value !== 'number' || !Number.isFinite(value) || value < min || value > max) throw Error(path);
}

function text(value, max, path) {
    if (typeof value !== 'string' || value.length > max || /[\u0000-\u0008\u000b\u000c\u000e-\u001f]/.test(value)) throw Error(path);
}

export function validateBinding(binding, path = 'binding') {
    if (binding === null) return;
    object(binding, ['uuid', 'scale', 'offset'], path);
    if (!UUID.test(binding.uuid)) throw Error(`${path}.uuid`);
    number(binding.scale, -1000000, 1000000, `${path}.scale`);
    number(binding.offset, -1000000, 1000000, `${path}.offset`);
}

function validateElement(element, assets) {
    object(element, ['id', 'type', 'asset', 'x', 'y', 'w', 'h', 'rotation', 'label', 'text',
        'color', 'fontSize', 'unit', 'decimals', 'value', 'activity', 'power', 'showPower',
        'points', 'width', 'direction', 'source', 'target'], 'element');
    if (typeof element.id !== 'string' || !ID.test(element.id) || !['equipment', 'text', 'value', 'pipe'].includes(element.type)) throw Error('element.id/type');
    text(element.asset, 64, 'asset');
    if (element.type === 'equipment' && !assets.has(element.asset)) throw Error(`element.asset: ${element.asset}`);
    for (const key of ['x', 'y']) number(element[key], 0, 1, key);
    for (const key of ['w', 'h']) number(element[key], 0.005, 1, key);
    if (element.x + element.w > 1.000001 || element.y + element.h > 1.000001) throw Error('element.bounds');
    if (![0, 90, 180, 270].includes(element.rotation)) throw Error('element.rotation');
    text(element.label, 120, 'label');
    text(element.text, 1000, 'text');
    text(element.unit, 24, 'unit');
    if (!COLOR.test(element.color)) throw Error('color');
    number(element.fontSize, 10, 64, 'fontSize');
    if (!Number.isInteger(element.decimals) || element.decimals < 0 || element.decimals > 6) throw Error('decimals');
    if (typeof element.showPower !== 'boolean') throw Error('showPower');
    validateBinding(element.value, 'value');
    validateBinding(element.activity, 'activity');
    if (element.power !== null) {
        object(element.power, ['source', 'minimum', 'maximum'], 'power');
        validateBinding(element.power.source, 'power.source');
        if (!element.power.source) throw Error('power.source');
        number(element.power.minimum, -1e12, 1e12, 'power.minimum');
        number(element.power.maximum, -1e12, 1e12, 'power.maximum');
        if (element.power.maximum <= element.power.minimum) throw Error('power.range');
    }
    if (element.type === 'pipe') {
        if (!Array.isArray(element.points) || element.points.length < 2 || element.points.length > LIMITS.points) throw Error('points');
        for (const point of element.points) {
            object(point, ['x', 'y'], 'point');
            number(point.x, 0, 1, 'point.x');
            number(point.y, 0, 1, 'point.y');
        }
        number(element.width, 1, 30, 'width');
        if (![1, -1].includes(element.direction)) throw Error('direction');
        for (const key of ['source', 'target']) {
            if (element[key] === null) continue;
            object(element[key], ['element', 'port'], key);
            if (typeof element[key].element !== 'string' || !ID.test(element[key].element) || !Number.isInteger(element[key].port) || element[key].port < 0 || element[key].port > 31) throw Error(key);
        }
    }
}

export function validateProject(project, catalog) {
    object(project, ['format', 'version', 'title', 'pages'], 'project');
    if (project.format !== 'zolee-schema' || project.version !== 1) throw Error('project.format/version');
    text(project.title, 80, 'title');
    if (!Array.isArray(project.pages) || !project.pages.length || project.pages.length > LIMITS.pages) throw Error('pages');
    const assets = new Set(catalog.elements.map(asset => asset.id));
    const pageIds = new Set();
    for (const page of project.pages) {
        object(page, ['id', 'name', 'background', 'elements'], 'page');
        if (typeof page.id !== 'string' || !ID.test(page.id) || pageIds.has(page.id)) throw Error('page.id');
        pageIds.add(page.id);
        text(page.name, 80, 'page.name');
        if (!['plain', 'cool', 'warm'].includes(page.background)) throw Error('background');
        if (!Array.isArray(page.elements) || page.elements.length > LIMITS.elements) throw Error('elements');
        const ids = new Set();
        for (const element of page.elements) {
            validateElement(element, assets);
            if (ids.has(element.id)) throw Error('duplicate element.id');
            ids.add(element.id);
        }
        for (const pipe of page.elements.filter(element => element.type === 'pipe')) {
            for (const end of [pipe.source, pipe.target]) {
                if (!end) continue;
                const equipment = page.elements.find(element => element.id === end.element && element.type === 'equipment');
                const asset = equipment && catalog.elements.find(item => item.id === equipment.asset);
                if (!asset || end.port >= asset.ports.length) throw Error('pipe.endpoint');
            }
        }
    }
    if (new TextEncoder().encode(JSON.stringify(project)).length > LIMITS.bytes) throw Error('project.size');
    return clone(project);
}

export function newProject() {
    return {format: 'zolee-schema', version: 1, title: 'ZoleeSchemaMonitor',
        pages: [{id: 'page-1', name: 'Gépészet', background: 'cool', elements: []}]};
}

export function newElement(type, asset = '') {
    return {id: newId(), type, asset, x: 0.1, y: 0.1, w: 0.12, h: 0.16,
        rotation: 0, label: '', text: '', color: '#e34d4d', fontSize: 20, unit: '', decimals: 1,
        value: null, activity: null, power: null, showPower: false,
        points: [{x: 0.1, y: 0.1}, {x: 0.4, y: 0.1}], width: 6, direction: 1, source: null, target: null};
}

export function readBinding(binding, values, connected = true) {
    if (!binding || !connected) return {configured: !!binding, valid: false, value: 0};
    const raw = values[binding.uuid.toLowerCase()];
    const value = typeof raw === 'number' ? raw * binding.scale + binding.offset : NaN;
    return {configured: true, valid: Number.isFinite(value), value};
}

export function activityState(element, values, connected = true) {
    const activity = readBinding(element.activity, values, connected);
    const power = readBinding(element.power?.source, values, connected);
    const range = element.power;
    const known = activity.configured ? activity.valid : power.configured ? power.valid : true;
    const active = known && (activity.configured ? activity.value > 0 : power.configured && power.value > range.minimum);
    let speed = 0;
    if (active) speed = power.valid ? Math.max(0.15, Math.min(1, (power.value - range.minimum) / (range.maximum - range.minimum))) : 1;
    return {known, active: !!active, speed};
}

export function exportPage(project, pageId) {
    const page = project.pages.find(item => item.id === pageId);
    if (!page) throw Error('page.id');
    return clone({...project, pages: [page]});
}
