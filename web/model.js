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
        'color', 'fontSize', 'unit', 'decimals', 'value', 'activity', 'power', 'showPower', 'align',
        'points', 'width', 'direction', 'source', 'target', 'position', 'manifold'], 'element');
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
    // Hiányzó mező a régi exportokban: balra igazítás.
    if (element.align !== undefined && (!['text', 'value'].includes(element.type) ||
        !['left', 'center', 'right'].includes(element.align))) throw Error('align');
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
    // Opcionális mező: a korábbi oldalexportok továbbra is érvényesek.
    if (element.position !== undefined && element.position !== null) {
        if (element.type !== 'equipment') throw Error('position.type');
        object(element.position, ['source', 'minimum', 'maximum', 'reverse'], 'position');
        validateBinding(element.position.source, 'position.source');
        if (!element.position.source) throw Error('position.source');
        number(element.position.minimum, -1e12, 1e12, 'position.minimum');
        number(element.position.maximum, -1e12, 1e12, 'position.maximum');
        if (element.position.maximum <= element.position.minimum ||
            typeof element.position.reverse !== 'boolean') throw Error('position.range');
    }
    if (element.asset === 'configurable-manifold' && element.type === 'equipment') {
        object(element.manifold, ['circuits', 'variant', 'connections', 'heads'], 'manifold');
        const m = element.manifold;
        if (!Number.isInteger(m.circuits) || m.circuits < 2 || m.circuits > 16 ||
            !['heating', 'floor', 'ceiling-heat', 'ceiling-cool'].includes(m.variant) ||
            typeof m.connections !== 'boolean' || !Array.isArray(m.heads) || m.heads.length !== m.circuits) throw Error('manifold');
        for (const head of m.heads) {
            object(head, ['percent', 'source'], 'manifold.head');
            number(head.percent, 0, 100, 'manifold.percent');
            validateBinding(head.source, 'manifold.source');
        }
    } else if (element.manifold !== undefined) throw Error('manifold.asset');
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
            if (typeof element[key].element !== 'string' || !ID.test(element[key].element) || !Number.isInteger(element[key].port) || element[key].port < 0 || element[key].port > 33) throw Error(key);
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
                if (!asset || end.port >= elementPorts(equipment, asset).length) throw Error('pipe.endpoint');
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
    return {...(['text', 'value'].includes(type) ? {align: 'left'} : {}),
        ...(type === 'equipment' && asset === 'configurable-manifold' ? {manifold: newManifold()} : {}), id: newId(), type, asset, x: 0.1, y: 0.1, w: 0.12, h: 0.16,
        rotation: 0, label: '', text: '', color: '#e34d4d', fontSize: 20, unit: '', decimals: 1,
        value: null, activity: null, power: null, showPower: false,
        points: [{x: 0.1, y: 0.1}, {x: 0.4, y: 0.1}], width: 6, direction: 1, source: null, target: null};
}

export const SPRAY_DROPS = 4;
// Ugyanaz a sugárarányos pálya, mint a hordozható C-magban.
export function sprayPoint(phase, drop, direction = -90) {
    if (!Number.isFinite(phase) || !Number.isFinite(direction) ||
        !Number.isInteger(drop) || drop < 0 || drop >= SPRAY_DROPS) throw Error('spray.point');
    const t = (phase - Math.floor(phase) + drop / SPRAY_DROPS) % 1;
    const angle = (direction % 360 + (drop - 1.5) * 18) * Math.PI / 180;
    const distance = .12 + .88 * t;
    return {x: Math.cos(angle) * distance, y: Math.sin(angle) * distance + .3 * t * t};
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

/* A szelepállás külön adat; ebből nem következtetünk szivattyúra vagy áramlásra. */
export function positionState(binding, values, minimum = 0, maximum = 100,
                              reverse = false, connected = true) {
    const signal = readBinding(binding, values, connected);
    if (!signal.valid || !Number.isFinite(minimum) || !Number.isFinite(maximum) ||
        maximum <= minimum || !Number.isFinite(maximum - minimum)) {
        return {known: false, percent: 0, angle: 0};
    }
    // Korlátozás a kivonás előtt, a firmware számításával azonos módon.
    const fraction = signal.value <= minimum ? 0 : signal.value >= maximum ? 1 :
        (signal.value - minimum) / (maximum - minimum);
    return {known: true, percent: fraction * 100,
        angle: (reverse ? 1 - fraction : fraction) * 90 - 45};
}

/* A körszám szerkesztésénél a megmaradó körök saját állapotforrása megmarad. */
export function newManifold(circuits = 4, previous) {
    return {circuits, variant: previous?.variant ?? 'floor', connections: previous?.connections ?? true,
        heads: Array.from({length: circuits}, (_, i) => previous?.heads[i] ? clone(previous.heads[i]) : {percent: 0, source: null})};
}

export function elementPorts(element, asset) {
    if (!element.manifold) return asset.ports;
    const m = element.manifold;
    if (!m.connections) return [];
    // Stabil páros indexelés: 2+2*i előremenő, 3+2*i visszatérő.
    return [{x: .04, y: .30}, {x: .04, y: .72}, ...m.heads.flatMap((_, i) =>
        [{x: .16 + .78 * (i + .5) / m.circuits, y: .46},
         {x: .16 + .78 * (i + .5) / m.circuits, y: .95}])];
}

export function manifoldHeadState(head, values, connected = true) {
    const signal = head.source ? readBinding(head.source, values, connected) : {valid: true, value: head.percent};
    const percent = signal.valid ? Math.max(0, Math.min(100, signal.value)) : 0;
    return {known: signal.valid, percent, active: signal.valid && percent > 0};
}

/* Csőnél a tárolt töréspontokat forgatjuk: a firmware pontosan ezt a vonalat rajzolja. */
export function rotateElement(element, rotation, size) {
    if (![0, 90, 180, 270].includes(rotation)) throw Error('element.rotation');
    const result = clone(element);
    if (element.type === 'pipe') {
        const angle = (rotation - element.rotation) * Math.PI / 180;
        const xs = element.points.map(p => p.x * size.width), ys = element.points.map(p => p.y * size.height);
        const cx = (Math.min(...xs) + Math.max(...xs)) / 2, cy = (Math.min(...ys) + Math.max(...ys)) / 2;
        result.points = xs.map((x, i) => ({x: (cx + (x-cx)*Math.cos(angle) - (ys[i]-cy)*Math.sin(angle))/size.width,
            y: (cy + (x-cx)*Math.sin(angle) + (ys[i]-cy)*Math.cos(angle))/size.height}));
        if (result.points.some(p => p.x < -1e-9 || p.x > 1+1e-9 || p.y < -1e-9 || p.y > 1+1e-9)) throw Error('element.bounds');
        result.points.forEach(p => {p.x = Math.max(0, Math.min(1,p.x)); p.y = Math.max(0, Math.min(1,p.y));});
        if (rotation !== element.rotation) result.source = result.target = null;
    }
    if (element.type !== 'pipe') {
        const width = element.w * size.width, height = element.h * size.height;
        const rw = rotation % 180 ? height : width, rh = rotation % 180 ? width : height;
        if (rw > size.width || rh > size.height) throw Error('element.bounds');
        const cx = Math.max(rw/2, Math.min(size.width-rw/2, (element.x+element.w/2)*size.width));
        const cy = Math.max(rh/2, Math.min(size.height-rh/2, (element.y+element.h/2)*size.height));
        // A tárolt befoglaló téglalap is az oldalon marad.
        result.x = Math.max(0,Math.min(1-element.w,cx/size.width-element.w/2));
        result.y = Math.max(0,Math.min(1-element.h,cy/size.height-element.h/2));
    }
    result.rotation = rotation;
    return result;
}

/* Pontos műszaki rajz: két külön sín, körönként egy átfolyásmérő és egy termofej. */
export function manifoldParts(m) {
    const parts = [];
    const rect = (x,y,w,h,color,head) => parts.push({x,y,w,h,color,...(head === undefined ? {} : {head})});
    const supply = m.variant === 'ceiling-cool' ? '#349bd0' : '#db5952';
    rect(.08,.265,.88,.07,'#c6cfd4'); rect(.08,.685,.88,.07,'#c6cfd4');
    rect(.08,.265,.88,.012,supply); rect(.08,.735,.88,.012,'#349bd0');
    if (m.connections) {rect(.04,.28,.04,.04,'#c9a455'); rect(.04,.70,.04,.04,'#c9a455');}
    for (let i=0;i<m.circuits;i++) {
        const x=.16+.78*(i+.5)/m.circuits, d=.55/m.circuits;
        rect(x-d*.28,.17,d*.56,.095,'#b9d9e5'); rect(x-d*.25,.235,d*.5,.035,supply);
        rect(x-d*.32,.335,d*.64,.06,'#c9a455');
        rect(x-d*.35,.65,d*.7,.035,'#c9a455');
        rect(x-d*.5,.50,d,.17,'#edf1f3'); rect(x-d*.4,.49,d*.8,.025,'#ffffff');
        rect(x-d*.23,.52,d*.46,.025,'#26333d',i);
        rect(x-d*.32,.755,d*.64,.06,'#c9a455');
        if(m.connections) {rect(x-d*.18,.395,d*.36,.065,supply);rect(x-d*.18,.815,d*.36,.135,'#349bd0');}
    }
    return parts;
}
