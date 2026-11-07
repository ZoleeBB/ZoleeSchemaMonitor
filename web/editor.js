import {LIMITS, canvasSize, newElement, newId, validateProject, activityState, readBinding, exportPage, positionState, newManifold, elementPorts, manifoldHeadState, manifoldParts, rotateElement} from './model.js';

const NS = 'http://www.w3.org/2000/svg';
const clone = value => structuredClone(value);
const clamp = (value, min, max) => Math.max(min, Math.min(max, value));

function svgNode(name, attributes = {}, text) {
    const node = document.createElementNS(NS, name);
    for (const [key, value] of Object.entries(attributes)) node.setAttribute(key, value);
    if (text !== undefined) node.textContent = text;
    return node;
}

export class Editor {
    constructor(root, state, helpers) {
        this.root = root;
        this.state = state;
        Object.assign(this, helpers);
        this.pageIndex = 0;
        this.selected = null;
        this.history = [];
        this.future = [];
        this.dirty = !!state.editorDirty;
        this.dark = false;
        this.preview = false;
        this.pipePoints = null;
        this.phase = 0;
        this.motion = new Map();
        this.size = canvasSize(state.settings.display.rotation);
        this.keyHandler = event => this.keydown(event);
        this.unloadHandler = event => { if (this.dirty) { event.preventDefault(); event.returnValue = ''; } };
        window.addEventListener('keydown', this.keyHandler);
        window.addEventListener('beforeunload', this.unloadHandler);
        this.render();
        this.animation = setInterval(() => this.animate(), 50);
    }

    get page() { return this.state.project.pages[this.pageIndex]; }
    get element() { return this.page.elements.find(element => element.id === this.selected); }

    remember() {
        this.history.push(JSON.stringify(this.state.project));
        if (this.history.length > 60) this.history.shift();
        this.future = [];
        this.markDirty();
    }

    markDirty() {
        this.dirty = true;
        this.state.editorDirty = true;
        this.root.querySelector('#save-project')?.classList.add('dirty');
    }

    restore(from, to) {
        if (!from.length) return;
        to.push(JSON.stringify(this.state.project));
        this.state.project = JSON.parse(from.pop());
        this.pageIndex = Math.min(this.pageIndex, this.state.project.pages.length - 1);
        this.selected = null;
        this.markDirty();
        this.render();
    }

    button(id, key) {
        return `<button type="button" id="${id}">${this.tr(key)}</button>`;
    }

    render() {
        const t = this.tr;
        this.root.innerHTML = `<h1>${t('editor')}</h1><div class="page-tabs">${this.state.project.pages.map((page, index) => `<button data-page-index="${index}" aria-current="${index === this.pageIndex}">${this.escape(page.name)}</button>`).join('')}</div>
            <div class="toolbar">${this.button('add-page', 'addPage')}${this.button('copy-page', 'copyPage')}${this.button('delete-page', 'deletePage')}
            ${this.button('save-project', 'save')}${this.button('export-page', 'exportPage')}${this.button('export-project', 'exportProject')}${this.button('import-project', 'import')}
            <input id="import-file" type="file" accept="application/json,.json" hidden></div>
            <div class="editor-layout"><section><h2>${t('available')}</h2><input id="asset-search" type="search" placeholder="${t('search')}">
            <select id="asset-category"><option value="">${t('all')}</option>${Object.entries(this.state.catalog.categories).map(([id, names]) => `<option value="${id}">${this.escape(names[this.language])}</option>`).join('')}</select>
            <div class="toolbar">${this.button('add-text', 'text')}${this.button('add-value', 'value')}${this.button('draw-pipe', 'pipe')}</div><div class="library" id="library"></div></section>
            <div><div class="toolbar">${this.button('undo', 'undo')}${this.button('redo', 'redo')}${this.button('duplicate', 'copy')}${this.button('delete', 'delete')}
            ${this.button('preview', this.preview ? 'edit' : 'preview')}${this.button('night', 'night')}</div>
            <div class="workspace ${this.preview ? 'preview-mode' : ''}" id="workspace"><svg id="canvas" xmlns="${NS}" viewBox="0 0 ${this.size.width} ${this.size.height}" class="${this.size.width < this.size.height ? 'portrait' : ''}" role="img" aria-label="Schema"></svg></div>
            <p class="hint" id="pipe-hint"></p><div class="small-grid"><label>${t('name')}<input id="page-name" maxlength="80" value="${this.escape(this.page.name)}"></label>
            <label>${t('background')}<select id="page-background">${['plain', 'cool', 'warm'].map(value => `<option value="${value}" ${this.page.background === value ? 'selected' : ''}>${t(value)}</option>`).join('')}</select></label></div></div>
            <section class="inspector"><h2>${t('properties')}</h2><div id="inspector"></div></section></div>`;
        const on = (id, handler) => this.root.querySelector(`#${id}`).onclick = handler;
        this.root.querySelectorAll('[data-page-index]').forEach(button => button.onclick = () => {
            this.pageIndex = Number(button.dataset.pageIndex);
            this.selected = null;
            this.pipePoints = null;
            this.render();
        });
        on('add-page', () => this.addPage());
        on('copy-page', () => this.addPage(true));
        on('delete-page', () => {
            if (this.state.project.pages.length < 2 || !confirm(t('confirmDelete'))) return;
            this.remember();
            this.state.project.pages.splice(this.pageIndex, 1);
            this.pageIndex = 0;
            this.selected = null;
            this.render();
        });
        on('save-project', () => this.save());
        on('export-page', () => this.download(exportPage(this.state.project, this.page.id), `${this.page.id}.json`));
        on('export-project', () => this.download(this.state.project, 'ZoleeSchemaMonitor.json'));
        on('import-project', () => this.root.querySelector('#import-file').click());
        this.root.querySelector('#import-file').onchange = event => this.import(event.target.files[0]);
        on('add-text', () => this.add('text'));
        on('add-value', () => this.add('value'));
        on('draw-pipe', () => {
            this.pipePoints = [];
            this.pipeSource = null;
            this.preview = false;
            this.root.querySelector('#pipe-hint').textContent = t('pipeHint');
        });
        on('undo', () => this.restore(this.history, this.future));
        on('redo', () => this.restore(this.future, this.history));
        on('delete', () => this.remove());
        on('duplicate', () => this.duplicate());
        on('preview', () => { this.preview = !this.preview; this.pipePoints = null; this.render(); });
        on('night', () => { this.dark = !this.dark; this.draw(); this.renderLibrary(); });
        this.root.querySelector('#asset-search').oninput = () => this.renderLibrary();
        this.root.querySelector('#asset-category').onchange = () => this.renderLibrary();
        this.root.querySelector('#page-name').onchange = event => {
            this.remember(); this.page.name = event.target.value; this.render();
        };
        this.root.querySelector('#page-background').onchange = event => {
            this.remember(); this.page.background = event.target.value; this.draw();
        };
        this.canvas = this.root.querySelector('#canvas');
        this.canvas.onpointerdown = event => this.pointerDown(event);
        this.canvas.onpointermove = event => this.pointerMove(event);
        this.canvas.onpointerup = () => this.pointerUp();
        this.canvas.onpointercancel = () => this.pointerUp();
        this.canvas.onclick = event => this.click(event);
        this.canvas.ondblclick = event => { if (this.pipePoints) { event.preventDefault(); this.finishPipe(); } };
        this.canvas.ondragover = event => event.preventDefault();
        this.canvas.ondrop = event => {
            event.preventDefault();
            const asset = event.dataTransfer.getData('text/schema-asset');
            if (this.state.catalog.elements.some(item => item.id === asset)) this.add('equipment', asset, this.point(event));
        };
        this.renderLibrary();
        this.draw();
        this.inspect();
        if (this.dirty) this.root.querySelector('#save-project').classList.add('dirty');
    }

    renderLibrary() {
        const search = this.root.querySelector('#asset-search').value.toLocaleLowerCase();
        const category = this.root.querySelector('#asset-category').value;
        const library = this.root.querySelector('#library');
        library.replaceChildren();
        for (const asset of this.state.catalog.elements) {
            if (category && asset.category !== category) continue;
            if (!asset.name[this.language].toLocaleLowerCase().includes(search)) continue;
            const button = document.createElement('button');
            button.type = 'button';
            button.draggable = true;
            button.dataset.asset = asset.id;
            const image = new Image();
            image.src = `/graphics/${asset.graphics[this.dark ? 'dark' : 'light']}`;
            image.alt = '';
            const picture = document.createElement('span');
            picture.className = 'library-picture';
            picture.append(image);
            if (asset.animation?.kind === 'rotor') {
                // A választóban is az összeszerelt kép látszik, nyugalmi rotorral.
                const overlay = svgNode('svg', {viewBox: '0 0 160 160', 'aria-hidden': 'true'});
                for (const marker of asset.animation.rotors || [asset.animation]) {
                    const cx = marker.x*160, cy = marker.y*160, radius = marker.radius*160;
                    const rotor = svgNode('g', {transform: `translate(${cx},${cy}) scale(${asset.animation.scaleX || 1},${asset.animation.scaleY || 1})`});
                    for (let i=0; i<3; i++) rotor.append(svgNode('path', {
                        d: `M0 0q${-radius} ${-radius} 0 ${-radius}q${radius} 0 0 ${radius}`,
                        fill: '#9cb0be', stroke: '#e5eff4', 'stroke-width': 1, transform: `rotate(${i*120})`}));
                    overlay.append(rotor);
                }
                picture.append(overlay);
            }
            button.append(picture, document.createTextNode(asset.name[this.language]));
            button.onclick = () => this.add('equipment', asset.id);
            button.ondragstart = event => event.dataTransfer.setData('text/schema-asset', asset.id);
            library.append(button);
        }
    }

    add(type, asset = '', point = {x: 0.15, y: 0.15}) {
        if (this.preview) return;
        if (this.page.elements.length >= LIMITS.elements) return this.feedback('150 / page', true);
        this.remember();
        const element = newElement(type, asset);
        if (type === 'equipment') {
            element.w = 150 / this.size.width;
            element.h = 150 / this.size.height;
            if (element.manifold) { element.w = 320 / this.size.width; element.h = 200 / this.size.height; }
        }
        element.x = clamp(point.x, 0, 1 - element.w);
        element.y = clamp(point.y, 0, 1 - element.h);
        if (type === 'text') { element.text = this.tr('text'); element.color = '#456172'; element.w = 0.2; element.h = 0.06; }
        if (type === 'value') { element.unit = '°C'; element.color = '#456172'; element.w = 0.15; element.h = 0.06; }
        element.x = clamp(element.x, 0, 1 - element.w);
        element.y = clamp(element.y, 0, 1 - element.h);
        this.page.elements.push(element);
        this.selected = element.id;
        this.draw();
        this.inspect();
    }

    addPage(copy = false) {
        if (this.state.project.pages.length >= LIMITS.pages) return this.feedback('10 / project', true);
        const name = prompt(this.tr('newPageName'), copy ? this.page.name : this.tr('pages'));
        if (name === null) return;
        this.remember();
        const page = copy ? this.copyPage(this.page) : {elements: [], background: 'cool'};
        Object.assign(page, {id: newId('p'), name: name.slice(0, 80)});
        this.state.project.pages.push(page);
        this.pageIndex = this.state.project.pages.length - 1;
        this.selected = null;
        this.render();
    }

    copyPage(page) {
        const copy = clone(page);
        const ids = new Map(copy.elements.map(element => [element.id, newId()]));
        for (const element of copy.elements) {
            element.id = ids.get(element.id);
            for (const endpoint of [element.source, element.target]) if (endpoint) endpoint.element = ids.get(endpoint.element);
        }
        return copy;
    }

    point(event) {
        const point = new DOMPoint(event.clientX, event.clientY).matrixTransform(this.canvas.getScreenCTM().inverse());
        return {x: clamp(point.x / this.size.width, 0, 1), y: clamp(point.y / this.size.height, 0, 1)};
    }

    portPoint(element, port) {
        const asset = this.state.catalog.elements.find(item => item.id === element.asset);
        const p = elementPorts(element, asset)[port];
        const radians = element.rotation * Math.PI / 180;
        const side = Math.min(element.w * this.size.width, element.h * this.size.height);
        const dx = (p.x - 0.5) * (element.manifold ? element.w * this.size.width : side);
        const dy = (p.y - 0.5) * (element.manifold ? element.h * this.size.height : side);
        return {x: element.x + element.w / 2 + (dx * Math.cos(radians) - dy * Math.sin(radians)) / this.size.width,
            y: element.y + element.h / 2 + (dx * Math.sin(radians) + dy * Math.cos(radians)) / this.size.height};
    }

    updateConnections(element) {
        for (const pipe of this.page.elements.filter(item => item.type === 'pipe')) {
            const asset = this.state.catalog.elements.find(item => item.id === element.asset);
            const count = asset ? elementPorts(element, asset).length : 0;
            for (const key of ['source', 'target']) if (pipe[key]?.element === element.id && pipe[key].port >= count) pipe[key] = null;
            if (pipe.source?.element === element.id) pipe.points[0] = this.portPoint(element, pipe.source.port);
            if (pipe.target?.element === element.id) pipe.points[pipe.points.length - 1] = this.portPoint(element, pipe.target.port);
        }
    }

    synchronizeConnections() {
        const equipment = new Map(this.page.elements.filter(e => e.type === 'equipment').map(e => [e.id, e]));
        let changed = false;
        for (const pipe of this.page.elements.filter(e => e.type === 'pipe')) {
            for (const key of ['source', 'target']) {
                const end = pipe[key];
                if (!end || !equipment.has(end.element)) continue;
                const point = this.portPoint(equipment.get(end.element), end.port);
                const index = key === 'source' ? 0 : pipe.points.length - 1;
                const previous = pipe.points[index];
                if (Math.abs(previous.x - point.x) > 1e-9 || Math.abs(previous.y - point.y) > 1e-9) {
                    pipe.points[index] = point;
                    changed = true;
                }
            }
        }
        // Frissített képhez új csonkhely tartozhat; a tartós mentés továbbra is explicit.
        if (changed) this.markDirty();
    }

    draw() {
        if (!this.canvas) return;
        this.synchronizeConnections();
        const {width: w, height: h} = this.size;
        this.canvas.replaceChildren();
        const defs = svgNode('defs');
        const gradient = svgNode('linearGradient', {id: 'page-bg', x2: '1', y2: '1'});
        const colors = this.dark ? ['#17232f', '#25394a'] : this.page.background === 'warm' ? ['#faf8f1', '#e9e1d7'] : ['#f7fafc', '#dfeaf0'];
        gradient.append(svgNode('stop', {offset: '0', 'stop-color': colors[0]}), svgNode('stop', {offset: '1', 'stop-color': this.page.background === 'plain' ? colors[0] : colors[1]}));
        defs.append(gradient);
        this.canvas.append(defs, svgNode('rect', {width: w, height: h, fill: 'url(#page-bg)'}));
        for (const element of this.page.elements) {
            const group = svgNode('g', {'data-element': element.id, class: element.type + (element.id === this.selected ? ' selected' : '')});
            if (element.type === 'pipe') {
                const path = svgNode('polyline', {points: element.points.map(point => `${point.x * w},${point.y * h}`).join(' '),
                    stroke: element.color, 'stroke-width': element.width, fill: 'none', 'stroke-linejoin': 'round', 'stroke-linecap': 'round'});
                group.append(path);
                group.append(svgNode('polyline', {points: path.getAttribute('points'), stroke: 'transparent', 'stroke-width': Math.max(16, element.width), fill: 'none'}));
                for (let index = 0; index < 5; index++) group.append(svgNode('circle', {r: Math.max(2, element.width * 0.3), fill: '#f5fbff', class: 'flow-dot', 'data-dot': index}));
                if (element.id === this.selected && !this.preview) element.points.forEach((point, index) => group.append(svgNode('circle', {
                    cx: point.x * w, cy: point.y * h, r: 6, class: 'handle', 'data-vertex': index})));
            } else {
                const x = element.x * w, y = element.y * h, width = element.w * w, height = element.h * h;
                const drawing = svgNode('g', {transform: `translate(${x},${y}) rotate(${element.rotation},${width / 2},${height / 2})`});
                if (element.type === 'equipment' && element.manifold) {
                    const m = element.manifold;
                    const tx = width / 2, ty = .055 * height;
                    drawing.append(svgNode('text', {x: tx, y: ty, 'text-anchor': 'middle',
                        fill: this.dark ? '#dfecf4' : '#354e60', 'font-size': 12,
                        transform: `rotate(${-element.rotation},${tx},${ty})`}, this.tr(m.variant)));
                    for (const part of manifoldParts(m)) drawing.append(svgNode('rect', {
                        x: part.x * width, y: part.y * height, width: part.w * width, height: part.h * height,
                        rx: Math.min(3,part.w * width * .15), fill: part.color,
                        stroke: '#67747d', 'stroke-width': .6,
                        ...(part.head === undefined ? {} : {'data-head-indicator': part.head})}));
                    m.heads.forEach((head, index) => {
                        const cx = (.16 + .78*(index+.5)/m.circuits)*width, cy = .65*height;
                        drawing.append(svgNode('text', {x: cx, y: cy, 'text-anchor': 'middle',
                            fill: '#26333d', 'font-size': Math.max(9, Math.min(16,width*.16/m.circuits)),
                            transform: `rotate(${-element.rotation},${cx},${cy})`, 'data-head-label': index}));
                        const ix = cx, iy = .12*height;
                        drawing.append(svgNode('text', {x: ix, y: iy, 'text-anchor': 'middle',
                            fill: this.dark ? '#dfecf4' : '#354e60', 'font-size': 11,
                            transform: `rotate(${-element.rotation},${ix},${iy})`}, index+1));
                    });
                    if (!this.preview) elementPorts(element, {}).forEach((p,index) => drawing.append(svgNode('circle', {
                        cx: p.x*width,cy: p.y*height,r:5,class:'port','data-port':index})));
                    if (element.label) group.append(svgNode('text', {x:x+width/2,y:y+height+17,
                        'text-anchor':'middle',fill:this.dark ? '#dfecf4' : '#354e60','font-size':element.fontSize},element.label));
                } else if (element.type === 'equipment') {
                    const asset = this.state.catalog.elements.find(item => item.id === element.asset);
                    const side = Math.min(width, height), offsetX = (width - side) / 2, offsetY = (height - side) / 2;
                    const equipment = svgNode('g', {transform: `translate(${offsetX},${offsetY})`});
                    equipment.append(svgNode('image', {href: `/graphics/${asset.graphics[this.dark ? 'dark' : 'light']}`, width: side, height: side}));
                    if (asset.animation?.kind === 'rotor') for (const marker of asset.animation.rotors || [asset.animation]) {
                        const cx = side * marker.x, cy = side * marker.y;
                        const rotor = svgNode('g', {'data-rotor': '1', 'data-cx': cx, 'data-cy': cy});
                        const radius = side * marker.radius;
                        for (let index = 0; index < 3; index++) rotor.append(svgNode('path', {
                            d: `M${cx} ${cy}q${-radius} ${-radius} 0 ${-radius}q${radius} 0 0 ${radius}`,
                            fill: '#9cb0be', stroke: '#e5eff4', 'stroke-width': 1, transform: `rotate(${index * 120},${cx},${cy})`}));
                        const projection = svgNode('g', {transform: `translate(${cx},${cy}) scale(${asset.animation.scaleX || 1},${asset.animation.scaleY || 1}) translate(${-cx},${-cy})`});
                        projection.append(rotor);
                        equipment.append(projection);
                    }
                    if (asset.animation?.kind === 'position' && element.position) {
                        const cx = side * asset.animation.x, cy = side * asset.animation.y;
                        const radius = side * asset.animation.radius;
                        equipment.append(svgNode('line', {x1: cx, y1: cy - radius * 0.2,
                            x2: cx, y2: cy - radius, stroke: '#ff3b30', 'stroke-width': Math.max(2, side * 0.018),
                            'stroke-linecap': 'round', 'data-position-needle': '1', 'data-cx': cx, 'data-cy': cy,
                            visibility: 'hidden'}));
                        // A százalék az elforgatott gomb alatt is vízszintesen olvasható.
                        const rotation = element.rotation * Math.PI / 180;
                        const tx = cx + Math.sin(rotation) * side * 0.025;
                        const ty = cy + Math.cos(rotation) * side * 0.025;
                        equipment.append(svgNode('text', {x: tx, y: ty, 'dominant-baseline': 'hanging',
                            fill: '#ffffff', 'text-anchor': 'middle', 'font-size': Math.max(8, side * 0.065),
                            'data-position-label': '1', transform: `rotate(${-element.rotation},${tx},${ty})`}, '—'));
                    }
                    if (asset.animation?.kind === 'status') {
                        equipment.append(svgNode('circle', {cx: side * asset.animation.x, cy: side * asset.animation.y,
                            r: Math.max(2, side * asset.animation.radius), fill: '#596674', stroke: '#e5eff4',
                            'stroke-width': 1, 'data-status-indicator': '1'}));
                    }
                    if (!this.preview) asset.ports.forEach((port, index) => equipment.append(svgNode('circle', {cx: port.x * side, cy: port.y * side,
                        r: 5, class: 'port', 'data-port': index})));
                    drawing.append(equipment);
                    if (element.label) group.append(svgNode('text', {x: x + width / 2, y: y + height + 17, 'text-anchor': 'middle',
                        fill: this.dark ? '#dfecf4' : '#354e60', 'font-size': element.fontSize}, element.label));
                    if (element.showPower) group.append(svgNode('text', {x: x + width / 2, y: y + height + 36,
                        'text-anchor': 'middle', fill: this.dark ? '#dfecf4' : '#354e60', 'font-size': element.fontSize, 'data-live-power': '1'}));
                } else {
                    drawing.append(svgNode('rect', {width, height, fill: 'transparent'}));
                    drawing.append(svgNode('text', {x: 0, y: element.fontSize, fill: this.dark && element.color === '#456172' ? '#d5e6f0' : element.color,
                        'font-size': element.fontSize, ...(element.type === 'value' ? {'data-live-value': '1'} : {})}, element.text || '—'));
                }
                group.append(drawing);
                if (element.id === this.selected && !this.preview) {
                    group.append(svgNode('rect', {x, y, width, height, class: 'selected-box'}));
                    group.append(svgNode('rect', {x: x + width - 6, y: y + height - 6, width: 12, height: 12, class: 'handle resize', 'data-resize': '1'}));
                }
            }
            this.canvas.append(group);
        }
        this.drawPipePreview();
        this.updateValues();
    }

    drawPipePreview() {
        this.canvas.querySelector('[data-pipe-preview]')?.remove();
        if (!this.pipePoints?.length) return;
        this.canvas.append(svgNode('polyline', {points: this.pipePoints.map(point => `${point.x * this.size.width},${point.y * this.size.height}`).join(' '),
            stroke: '#dc5656', 'stroke-width': 5, fill: 'none', 'stroke-dasharray': '8 4', 'pointer-events': 'none', 'data-pipe-preview': '1'}));
    }

    click(event) {
        if (this.preview || this.pipePoints === null || this.pipePoints === undefined) return;
        const port = event.target.closest('[data-port]');
        if (port) {
            const equipment = this.page.elements.find(item => item.id === port.closest('[data-element]').dataset.element);
            const endpoint = {element: equipment.id, port: Number(port.dataset.port)};
            this.pipePoints.push(this.portPoint(equipment, endpoint.port));
            if (this.pipePoints.length === 1) this.pipeSource = endpoint;
            else return this.finishPipe(endpoint);
        } else this.pipePoints.push(this.point(event));
        this.drawPipePreview();
    }

    finishPipe(target = null) {
        if (!this.pipePoints || this.pipePoints.length < 2 || this.page.elements.length >= LIMITS.elements) return;
        this.remember();
        const pipe = newElement('pipe');
        const points = this.pipePoints.filter((p, i, all) => !i || Math.hypot(p.x - all[i - 1].x, p.y - all[i - 1].y) > 0.001);
        if (points.length < 2) return;
        if (points.length === 2) points.splice(1, 0, {x: points[1].x, y: points[0].y});
        Object.assign(pipe, {points, source: this.pipeSource, target});
        this.page.elements.push(pipe);
        this.selected = pipe.id;
        this.pipePoints = null;
        this.root.querySelector('#pipe-hint').textContent = '';
        this.draw();
        this.inspect();
    }

    pointerDown(event) {
        if (this.preview || this.pipePoints) return;
        const group = event.target.closest('[data-element]');
        this.selected = group?.dataset.element || null;
        this.inspect();
        if (!this.element) { this.draw(); return; }
        this.remember();
        this.drag = {start: this.point(event), original: clone(this.element), vertex: event.target.dataset.vertex,
            resize: !!event.target.dataset.resize};
        this.canvas.setPointerCapture(event.pointerId);
        this.draw();
    }

    pointerMove(event) {
        if (!this.drag || !this.element) return;
        const point = this.point(event), original = this.drag.original;
        const dx = point.x - this.drag.start.x, dy = point.y - this.drag.start.y;
        if (this.element.type === 'pipe') {
            if (this.drag.vertex !== undefined) {
                const vertex = Number(this.drag.vertex);
                this.element.points[vertex] = point;
                if (vertex === 0) this.element.source = null;
                if (vertex === this.element.points.length - 1) this.element.target = null;
            } else {
                const minX = Math.min(...original.points.map(p => p.x)), maxX = Math.max(...original.points.map(p => p.x));
                const minY = Math.min(...original.points.map(p => p.y)), maxY = Math.max(...original.points.map(p => p.y));
                this.element.points = original.points.map(p => ({x: p.x + clamp(dx, -minX, 1 - maxX), y: p.y + clamp(dy, -minY, 1 - maxY)}));
                this.element.source = this.element.target = null;
            }
        } else if (this.drag.resize) {
            this.element.w = clamp(original.w + dx, 0.02, 1 - original.x);
            this.element.h = clamp(original.h + dy, 0.02, 1 - original.y);
            this.updateConnections(this.element);
        } else {
            this.element.x = clamp(original.x + dx, 0, 1 - original.w);
            this.element.y = clamp(original.y + dy, 0, 1 - original.h);
            this.updateConnections(this.element);
        }
        if (this.element.type !== 'pipe') {
            try { Object.assign(this.element, rotateElement(this.element, this.element.rotation, this.size)); }
            catch { Object.assign(this.element, original); }
            this.updateConnections(this.element);
        }
        this.draw();
    }

    pointerUp() { if (this.drag) { this.drag = null; this.inspect(); } }

    inspect() {
        const panel = this.root.querySelector('#inspector');
        const e = this.element, t = this.tr;
        if (!e) { panel.textContent = t('select'); return; }
        const field = (key, title, type = 'text') => `<label>${t(title)}<input data-property="${key}" type="${type}" value="${this.escape(e[key])}"></label>`;
        panel.innerHTML = `<p class="hint">${this.escape(e.asset || e.type)}</p>${field('label', 'label')}
            ${e.type === 'text' ? field('text', 'text') : ''}${field('color', 'color', 'color')}
            <div class="small-grid">${field('fontSize', 'fontSize', 'number')}${e.type === 'pipe' ? field('width', 'lineWidth', 'number') : ''}</div>
            ${e.type !== 'pipe' ? `<div class="small-grid">${field('w', 'width', 'number')}${field('h', 'height', 'number')}</div>` : this.button('reverse-pipe', 'reverse')}
            ${e.type === 'value' ? `${field('unit', 'unit')}${field('decimals', 'decimals', 'number')}<button class="binding-button" data-binding="value">${t('source')}: ${this.escape(e.value?.uuid || t('none'))}</button>` : ''}
            ${['equipment', 'pipe'].includes(e.type) && !e.manifold ? `<label>${t('activity')}<button class="binding-button" data-binding="activity">${this.escape(e.activity?.uuid || t('none'))}</button></label>
            <label>${t('power')}<button class="binding-button" data-binding="power">${this.escape(e.power?.source.uuid || t('none'))}</button></label>
            ${e.power ? `<div class="small-grid"><label>${t('minimum')}<input id="power-min" type="number" step="any" value="${e.power.minimum}"></label><label>${t('maximum')}<input id="power-max" type="number" step="any" value="${e.power.maximum}"></label></div>
                <label><input id="show-power" type="checkbox" ${e.showPower ? 'checked' : ''}>${t('showPower')}</label>` : ''}<p class="hint">${t('sourceHint')}</p>` : ''}`;
        panel.insertAdjacentHTML('beforeend', `<label>${t('rotate')}<select id="element-rotation">${[0,90,180,270].map(n => `<option value="${n}" ${n === e.rotation ? 'selected' : ''}>${n}°</option>`).join('')}</select></label>`);
        panel.querySelector('#element-rotation').onchange = event => {
            const before = clone(this.state.project);
            this.remember();
            try {
                Object.assign(e, rotateElement(e, Number(event.target.value), this.size));
                this.updateConnections(e);
                validateProject(this.state.project, this.state.catalog);
                this.draw(); this.inspect();
            } catch(error) { this.state.project = before; this.feedback(error.message, true); this.inspect(); }
        };
        if (e.manifold) this.inspectManifold(panel, e);
        if (e.type === 'equipment' && !e.manifold) {
            panel.insertAdjacentHTML('beforeend', `<label>${t('position')}<button class="binding-button" data-binding="position">${this.escape(e.position?.source.uuid || t('none'))}</button></label>
                ${e.position ? `<div class="small-grid"><label>${t('minimum')}<input id="position-min" type="number" step="any" value="${e.position.minimum}"></label><label>${t('maximum')}<input id="position-max" type="number" step="any" value="${e.position.maximum}"></label></div>
                <label><input id="position-reverse" type="checkbox" ${e.position.reverse ? 'checked' : ''}>${t('positionReverse')}</label>` : ''}<p class="hint">${t('positionHint')}</p>`);
        }
        panel.querySelectorAll('[data-property]').forEach(input => input.onchange = () => {
            const key = input.dataset.property;
            const value = input.type === 'number' ? Number(input.value) : input.value;
            const before = clone(this.state.project);
            this.remember();
            e[key] = value;
            try {
                if (e.type !== 'pipe') Object.assign(e, rotateElement(e, e.rotation, this.size));
                this.updateConnections(e);
                validateProject(this.state.project, this.state.catalog);
                this.draw();
            } catch (error) {
                this.state.project = before;
                this.feedback(error.message, true);
                this.inspect();
                this.draw();
            }
        });
        panel.querySelectorAll('[data-binding]').forEach(button => button.onclick = async () => {
            const key = button.dataset.binding;
            const binding = await this.chooseBinding(['power', 'position'].includes(key) ? e[key]?.source : e[key]);
            if (binding === undefined) return;
            this.remember();
            if (key === 'power') e.power = binding ? {source: binding, minimum: e.power?.minimum ?? 0, maximum: e.power?.maximum ?? 100} : null;
            else if (key === 'position') e.position = binding ? {source: binding, minimum: e.position?.minimum ?? 0, maximum: e.position?.maximum ?? 100, reverse: e.position?.reverse ?? false} : null;
            else e[key] = binding;
            this.inspect();
            this.draw();
        });
        for (const selector of ['#power-min', '#power-max']) {
            const input = panel.querySelector(selector);
            if (input) input.onchange = () => {
                const minimum = Number(panel.querySelector('#power-min').value), maximum = Number(panel.querySelector('#power-max').value);
                if (!Number.isFinite(minimum) || !Number.isFinite(maximum) || maximum <= minimum) {
                    this.feedback('power.range', true); this.inspect(); return;
                }
                this.remember();
                Object.assign(e.power, {minimum, maximum});
            };
        }
        for (const selector of ['#position-min', '#position-max']) {
            const input = panel.querySelector(selector);
            if (input) input.onchange = () => {
                const minimumInput = panel.querySelector('#position-min');
                const maximumInput = panel.querySelector('#position-max');
                const minimum = Number(minimumInput.value), maximum = Number(maximumInput.value);
                if (!minimumInput.value.trim() || !maximumInput.value.trim() ||
                    !Number.isFinite(minimum) || !Number.isFinite(maximum) ||
                    minimum < -1e12 || maximum > 1e12 || maximum <= minimum) {
                    this.feedback('position.range', true); this.inspect(); return;
                }
                this.remember(); Object.assign(e.position, {minimum, maximum}); this.draw();
            };
        }
        const positionReverse = panel.querySelector('#position-reverse');
        if (positionReverse) positionReverse.onchange = event => {
            this.remember(); e.position.reverse = event.target.checked; this.draw();
        };
        if (panel.querySelector('#show-power')) panel.querySelector('#show-power').onchange = event => {
            this.remember(); e.showPower = event.target.checked; this.draw();
        };
        if (e.type === 'pipe') panel.querySelector('#reverse-pipe').onclick = () => { this.remember(); e.direction *= -1; this.draw(); };
    }

    inspectManifold(panel, e) {
        const t = this.tr, m = e.manifold;
        panel.insertAdjacentHTML('beforeend', `<fieldset><legend>${t('manifold')}</legend>
            <label>${t('circuits')}<select id="manifold-circuits">${Array.from({length:15},(_,i)=>i+2).map(n=>`<option ${n===m.circuits?'selected':''}>${n}</option>`).join('')}</select></label>
            <label>${t('variant')}<select id="manifold-variant">${['heating','floor','ceiling-heat','ceiling-cool'].map(v=>`<option value="${v}" ${v===m.variant?'selected':''}>${t(v)}</option>`).join('')}</select></label>
            <label><input type="checkbox" id="manifold-connections" ${m.connections?'checked':''}>${t('connections')}</label>
            <p class="hint">${t('headHint')}</p>
            ${m.heads.map((head,i)=>`<label>${t('circuit')} ${i+1} (%)<input data-head-percent="${i}" type="number" min="0" max="100" step="any" ${head.source?'disabled':''} value="${head.percent}"></label><button type="button" data-head-source="${i}">${t('source')}: ${this.escape(head.source?.uuid || t('manual'))}</button>`).join('')}</fieldset>`);
        const change = update => {
            const before = clone(this.state.project);
            this.remember();
            try {update(); this.updateConnections(e); validateProject(this.state.project,this.state.catalog);this.draw();this.inspect();}
            catch(error) {this.state.project=before;this.feedback(error.message,true);this.inspect();}
        };
        panel.querySelector('#manifold-circuits').onchange = event => change(()=>{e.manifold=newManifold(Number(event.target.value),m);
            e.w=Math.min(this.size.width*.9,80+40*e.manifold.circuits)/this.size.width;
            Object.assign(e,rotateElement(e,e.rotation,this.size));});
        panel.querySelector('#manifold-variant').onchange = event => change(()=>{m.variant=event.target.value;});
        panel.querySelector('#manifold-connections').onchange = event => change(()=>{m.connections=event.target.checked;});
        panel.querySelectorAll('[data-head-percent]').forEach(input => input.onchange = () => change(()=>{
            if (!input.value.trim()) throw Error('manifold.percent');
            m.heads[Number(input.dataset.headPercent)].percent=Number(input.value);
        }));
        panel.querySelectorAll('[data-head-source]').forEach(button => button.onclick = async () => {
            const head=m.heads[Number(button.dataset.headSource)], source=await this.chooseBinding(head.source);
            if(source!==undefined) change(()=>{head.source=source;});
        });
    }

    updateValues() {
        if (!this.canvas) return;
        for (const element of this.page.elements) {
            const group = this.canvas.querySelector(`[data-element="${element.id}"]`);
            if (!group) continue;
            const live = activityState(element, this.state.values, this.state.device.connected);
            group.dataset.active = live.active;
            group.dataset.known = live.known;
            const setText = (selector, text) => {
                const node = group.querySelector(selector);
                if (node && node.textContent !== text) node.textContent = text;
            };
            const value = readBinding(element.value, this.state.values, this.state.device.connected);
            setText('[data-live-value]', value.valid ? `${value.value.toFixed(element.decimals)} ${element.unit}` : `— ${element.unit}`);
            const position = positionState(element.position?.source, this.state.values,
                element.position?.minimum, element.position?.maximum, element.position?.reverse,
                this.state.device.connected);
            setText('[data-position-label]', position.known ? `${Math.round(position.percent)}%` : '—');
            const needle = group.querySelector('[data-position-needle]');
            if (needle) {
                needle.setAttribute('visibility', position.known ? 'visible' : 'hidden');
                const transform = `rotate(${position.angle},${needle.dataset.cx},${needle.dataset.cy})`;
                if (needle.getAttribute('transform') !== transform) needle.setAttribute('transform', transform);
            }
            element.manifold?.heads.forEach((head,index) => {
                const state = manifoldHeadState(head, this.state.values, this.state.device.connected);
                setText(`[data-head-label="${index}"]`, state.known ? (state.percent > 0 && state.percent < .5 ? '<1%' : `${Math.round(state.percent)}%`) : '—');
                const indicator = group.querySelector(`[data-head-indicator="${index}"]`);
                const color = !state.known ? '#d9a23a' : state.active ? '#36c678' : '#26333d';
                if (indicator && indicator.getAttribute('fill') !== color) indicator.setAttribute('fill',color);
            });
            const power = readBinding(element.power?.source, this.state.values, this.state.device.connected);
            setText('[data-live-power]', power.valid ? `${power.value.toFixed(element.decimals)} ${element.unit || '%'}` : '—');
            group.style.opacity = element.manifold || live.known ? '1' : '0.55';
        }
    }

    animate() {
        this.phase = (this.phase + 0.05) % 10000;
        for (const element of this.page.elements) {
            const group = this.canvas?.querySelector(`[data-element="${element.id}"]`);
            if (!group) continue;
            const live = activityState(element, this.state.values, this.state.device.connected);
            const motion = this.motion.get(element.id) || {angle: 0, phase: 0};
            this.motion.set(element.id, motion);
            const rotors = group.querySelectorAll('[data-rotor]');
            if (rotors.length && live.active) {
                motion.angle = (motion.angle + 18 * live.speed) % 360;
                for (const rotor of rotors) rotor.setAttribute('transform', `rotate(${motion.angle},${rotor.dataset.cx},${rotor.dataset.cy})`);
            }
            const indicator = group.querySelector('[data-status-indicator]');
            if (indicator) {
                if (live.active) motion.angle = (motion.angle + 18 * live.speed) % 360;
                const color = !live.known ? '#d9a23a' : live.active ? '#36c678' : '#596674';
                const opacity = live.active ? 0.5 + 0.5 * (Math.sin(motion.angle * Math.PI / 180) + 1) / 2 : 1;
                if (indicator.getAttribute('fill') !== color) indicator.setAttribute('fill', color);
                if (indicator.getAttribute('opacity') !== String(opacity)) indicator.setAttribute('opacity', opacity);
            }
            if (element.type === 'pipe') {
                const path = group.querySelector('polyline'), length = path.getTotalLength();
                if (live.active && length > 0) motion.phase = (motion.phase + 2.5 * live.speed * element.direction / length + 1) % 1;
                group.querySelectorAll('[data-dot]').forEach((dot, index) => {
                    dot.style.display = live.active && length > 0 ? '' : 'none';
                    if (!live.active || !length) return;
                    const phase = (motion.phase + index / 5) % 1;
                    const p = path.getPointAtLength(((phase + 1) % 1) * length);
                    dot.setAttribute('cx', p.x);
                    dot.setAttribute('cy', p.y);
                });
            }
        }
    }

    remove() {
        if (!this.element || this.preview) return;
        this.remember();
        const deleted = this.selected;
        this.page.elements = this.page.elements.filter(element => element.id !== deleted);
        for (const element of this.page.elements) {
            if (element.source?.element === deleted) element.source = null;
            if (element.target?.element === deleted) element.target = null;
        }
        this.selected = null;
        this.draw();
        this.inspect();
    }

    duplicate() {
        if (!this.element || this.preview || this.page.elements.length >= LIMITS.elements) return;
        this.remember();
        const element = clone(this.element);
        element.id = newId();
        if (element.type === 'pipe') element.source = element.target = null;
        else { element.x = clamp(element.x + 0.03, 0, 1 - element.w); element.y = clamp(element.y + 0.03, 0, 1 - element.h); }
        this.page.elements.push(element);
        this.selected = element.id;
        this.draw();
        this.inspect();
    }

    keydown(event) {
        if (document.querySelector('dialog[open]') || ['INPUT', 'TEXTAREA', 'SELECT'].includes(document.activeElement.tagName)) return;
        if (event.key === 'Escape') { this.pipePoints = null; this.drag = null; this.draw(); }
        if (this.preview) return;
        if (event.key === 'Delete') this.remove();
        if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === 'z') {
            event.preventDefault(); this.restore(event.shiftKey ? this.future : this.history, event.shiftKey ? this.history : this.future);
        }
        if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === 'd') { event.preventDefault(); this.duplicate(); }
        if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === 's') { event.preventDefault(); this.save(); }
    }

    async save() {
        try {
            const project = validateProject(this.state.project, this.state.catalog);
            const result = await this.api('/api/project', {project, revision: this.state.revision});
            this.state.revision = result.revision;
            this.dirty = false;
            this.state.editorDirty = false;
            this.root.querySelector('#save-project').classList.remove('dirty');
            this.feedback(this.tr('saved'));
        } catch (error) { this.feedback(error.message, true); }
    }

    download(project, name) {
        try {
            const checked = validateProject(project, this.state.catalog);
            const url = URL.createObjectURL(new Blob([JSON.stringify(checked, null, 2)], {type: 'application/json'}));
            const link = document.createElement('a');
            link.href = url; link.download = name; link.click();
            setTimeout(() => URL.revokeObjectURL(url), 1000);
        } catch (error) { this.feedback(error.message, true); }
    }

    async import(file) {
        if (!file) return;
        try {
            if (file.size > LIMITS.bytes) throw Error('project.size');
            const imported = validateProject(JSON.parse(await file.text()), this.state.catalog);
            const replace = confirm(this.tr('importMode'));
            const next = clone(this.state.project);
            if (replace) Object.assign(next, imported);
            else {
                for (const page of imported.pages) next.pages.push({...this.copyPage(page), id: newId('p')});
            }
            validateProject(next, this.state.catalog);
            this.remember();
            this.state.project = next;
            this.pageIndex = 0;
            this.selected = null;
            this.render();
        } catch (error) { this.feedback(error.message, true); }
    }

    destroy() {
        clearInterval(this.animation);
        window.removeEventListener('keydown', this.keyHandler);
        window.removeEventListener('beforeunload', this.unloadHandler);
    }
}
