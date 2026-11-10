import {refreshBridges, moveLinkedPoint, pipeLinks, shareFlow, sameFlow, nearbySegments, crossingAt, existingVertex, attachVertex, pipeGeometry} from './pipe-links.js';
import {labelLines} from './text-layout.js';
import {LIMITS, canvasSize, newElement, newId, validateProject, activityState, readBinding, exportPage, positionState, newManifold, elementPorts, manifoldHeadState, manifoldParts, manifoldCenter, manifoldWidth, equipmentImageSize, manifoldImage, rotateElement, SPRAY_DROPS, sprayPoint, valueText} from './model.js';

const searchText = value => value.normalize('NFD').replace(/\p{M}/gu, '').toLowerCase();
const NS = 'http://www.w3.org/2000/svg';
const clone = value => structuredClone(value);
const clamp = (value, min, max) => Math.max(min, Math.min(max, value));

function pipeLuminance(color) {
    const rgb = parseInt(color.slice(1), 16);
    return 299*(rgb >> 16) + 587*((rgb >> 8) & 255) + 114*(rgb & 255);
}

function svgNode(name, attributes = {}, text) {
    const node = document.createElementNS(NS, name);
    for (const [key, value] of Object.entries(attributes)) node.setAttribute(key, value);
    if (text !== undefined) node.textContent = text;
    return node;
}

function setFieldText(node, text) {
    if (node.dataset.fieldText === text) return;
    const size = Number(node.getAttribute('font-size'));
    const scale = Math.floor(size * 256 / 20) / 256;
    const spans = labelLines(text, Number(node.dataset.fieldWidth), size).map((line, index) =>
        svgNode('tspan', {x: node.getAttribute('x'), y: (21 + 28 * index) * scale,
            ...(line.width ? {textLength: line.width * scale, lengthAdjust: 'spacingAndGlyphs'} : {})}, line.text));
    node.replaceChildren(...spans);
    node.dataset.fieldText = text;
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
        this.librarySearch = '';
        this.libraryCategory = '';
        this.libraryManufacturer = '';
        this.boilerManufacturers = [...new Set(state.catalog.elements.map(asset => asset.manufacturer).filter(Boolean))]
            .sort((a, b) => a.localeCompare(b, this.language));
        this.pipePoints = null;
        this.pipeStyle = {width: 6, color: '#e34d4d'};
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

    remember(before = JSON.stringify(this.state.project)) {
        this.history.push(before);
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
        this.pipePoints = null;
        this.markDirty();
        this.render();
    }

    button(id, key) {
        return `<button type="button" id="${id}">${this.tr(key)}</button>`;
    }

    render() {
        const t = this.tr;
        this.root.innerHTML = `<div class="editor-heading"><h1>${t('editor')}</h1><span class="badge">${this.state.project.pages.length} / ${LIMITS.pages} ${t('pages')}</span></div><div class="page-tabs" role="group" aria-label="${t('pages')}">${this.state.project.pages.map((page, index) => `<button data-page-index="${index}" aria-current="${index === this.pageIndex}">${this.escape(page.name)}</button>`).join('')}</div>
            <div class="toolbar project-toolbar"><div class="tool-group" role="group" aria-label="${t('pages')}">${this.button('add-page', 'addPage')}${this.button('copy-page', 'copyPage')}${this.button('delete-page', 'deletePage')}</div>
            <div class="tool-group" role="group" aria-label="${t('projectFiles')}">${this.button('save-project', 'save')}${this.button('export-page', 'exportPage')}${this.button('export-project', 'exportProject')}${this.button('import-project', 'import')}</div>
            <input id="import-file" type="file" accept="application/json,.json" hidden></div>
            <datalist id="pipe-colors">${['#e34d4d','#287ac4','#258653','#f2cb3d','#ffffff','#26333d'].map(color => `<option value="${color}"></option>`).join('')}</datalist><div class="editor-layout"><section class="library-panel" aria-labelledby="library-title"><h2 id="library-title">${t('equipmentLibrary')}</h2><label>${t('search')}<input id="asset-search" type="search" placeholder="${t('search')}"></label>
            <label>${t('equipmentCategory')}<select id="asset-category"><option value="">${t('all')}</option>${Object.entries(this.state.catalog.categories).map(([id, names]) => `<option value="${id}">${this.escape(names[this.language])}</option>`).join('')}</select></label>
            <label id="manufacturer-label">${t('boilerManufacturer')}<select id="boiler-manufacturer"><option value="">${t('all')}</option>${this.boilerManufacturers.map(name => `<option>${this.escape(name)}</option>`).join('')}</select></label>
            <div class="toolbar" role="group" aria-label="${t('addElement')}">${this.button('add-text', 'text')}${this.button('add-value', 'value')}${this.button('draw-pipe', 'pipe')}</div><p id="library-count" class="hint" role="status"></p><div class="library" id="library"></div></section>
            <div class="editor-stage"><div class="toolbar canvas-toolbar"><div class="tool-group" role="group" aria-label="${t('editTools')}">${this.button('undo', 'undo')}${this.button('redo', 'redo')}${this.button('duplicate', 'copy')}${this.button('delete', 'delete')}</div>
            <div class="tool-group" role="group" aria-label="${t('preview')}">${this.button('preview', this.preview ? 'edit' : 'preview')}${this.button('night', 'night')}</div></div>
            <div class="workspace ${this.preview ? 'preview-mode' : ''}" id="workspace"><svg id="canvas" xmlns="${NS}" viewBox="0 0 ${this.size.width} ${this.size.height}" class="${this.size.width < this.size.height ? 'portrait' : ''}" role="img" aria-label="${t('schemaCanvas')}"></svg></div>
            <fieldset id="pipe-tools" hidden><legend>${t('drawPipe')}</legend>
            <div class="small-grid"><label>${t('color')}<input type="color" id="pipe-color" list="pipe-colors" value="${this.pipeStyle.color}"></label>
            <label>${t('lineWidth')}<input type="number" min="1" max="30" step="1" id="pipe-width" value="${this.pipeStyle.width}"></label></div>
            <p class="hint" id="pipe-hint">${t('pipeHint')}</p><div class="tool-group">${this.button('finish-pipe','finishPipe')}${this.button('cancel-pipe','cancel')}</div></fieldset><fieldset class="page-properties"><legend>${t('pageSettings')}</legend><div class="small-grid"><label>${t('name')}<input id="page-name" maxlength="80" value="${this.escape(this.page.name)}"></label>
            <label>${t('background')}<select id="page-background">${['plain', 'cool', 'warm'].map(value => `<option value="${value}" ${this.page.background === value ? 'selected' : ''}>${t(value)}</option>`).join('')}</select></label></div></fieldset></div>
            <section class="inspector" aria-labelledby="properties-title"><h2 id="properties-title">${t('properties')}</h2><div id="inspector"></div></section></div>`;
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
            this.pipePoints = null;
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
            if (this.pipePoints) return this.cancelPipe();
            if (this.page.elements.length >= LIMITS.elements) return this.feedback('150 / page', true);
            this.pipePoints = [];
            this.pipeSource = null;
            this.branchStart = null;
            if (this.preview) {
                this.preview = false;
                this.render();
            } else {
                this.syncPipeTools();
                this.draw();
            }
            if (window.matchMedia('(max-width: 759px)').matches) this.canvas.scrollIntoView({block:'start'});
        });
        on('finish-pipe', () => this.finishPipe());
        on('cancel-pipe', () => this.cancelPipe());
        this.root.querySelector('#pipe-color').oninput = event => {
            this.pipeStyle.color = event.target.value;
            this.drawPipePreview();
        };
        this.root.querySelector('#pipe-width').onchange = event => {
            const input = event.target;
            if (input.value && input.validity.valid) this.pipeStyle.width = Number(input.value);
            input.value = this.pipeStyle.width;
            this.drawPipePreview();
        };
        on('undo', () => this.restore(this.history, this.future));
        on('redo', () => this.restore(this.future, this.history));
        on('delete', () => this.remove());
        on('duplicate', () => this.duplicate());
        on('preview', () => { this.preview = !this.preview; this.pipePoints = null; this.render(); });
        on('night', () => { this.dark = !this.dark; this.draw(); this.renderLibrary(); this.inspect(); });
        const search = this.root.querySelector('#asset-search'), category = this.root.querySelector('#asset-category');
        search.value = this.librarySearch;
        category.value = this.libraryCategory;
        const manufacturer = this.root.querySelector('#boiler-manufacturer');
        manufacturer.value = this.libraryManufacturer;
        manufacturer.onchange = () => { this.libraryManufacturer = manufacturer.value; this.renderLibrary(); };
        search.oninput = () => { this.librarySearch = search.value; this.renderLibrary(); };
        category.onchange = () => { this.libraryCategory = category.value; this.renderLibrary(); };
        this.root.querySelector('#page-name').onchange = event => {
            this.remember(); this.page.name = event.target.value; this.render();
        };
        this.root.querySelector('#page-background').onchange = event => {
            this.remember(); this.page.background = event.target.value; this.draw();
        };
        this.canvas = this.root.querySelector('#canvas');
        this.canvas.onpointerdown = event => this.pointerDown(event);
        this.canvas.onpointermove = event => this.pointerMove(event);
        this.canvas.onpointerleave = () => { this.pipeCursor = null; this.drawPipePreview(); };
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
        this.pipeCursor = null;
        this.syncPipeTools();
        this.renderLibrary();
        this.draw();
        this.inspect();
        if (this.dirty) this.root.querySelector('#save-project').classList.add('dirty');
    }

    assetPicture(asset, manifold) {
        const image = new Image();
        image.decoding = 'async';
        image.src = manifold ? manifoldImage(manifold) : `/graphics/${asset.graphics[this.dark ? 'dark' : 'light']}`;
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
        return picture;
    }

    renderLibrary() {
        const terms = searchText(this.librarySearch).trim().split(/\s+/);
        const category = this.root.querySelector('#asset-category').value;
        const library = this.root.querySelector('#library');
        const showManufacturers = !category || category === 'heating';
        this.root.querySelector('#manufacturer-label').hidden = !showManufacturers;
        if (!showManufacturers) {
            this.libraryManufacturer = '';
            this.root.querySelector('#boiler-manufacturer').value = '';
        }
        library.replaceChildren();
        const selectedAsset = this.element?.asset;
        for (const asset of this.state.catalog.elements) {
            if (category && asset.category !== category) continue;
            if (this.libraryManufacturer && asset.manufacturer !== this.libraryManufacturer) continue;
            const keywords = searchText([asset.name.hu, asset.name.en, asset.id].join(' '));
            if (!terms.every(term => keywords.includes(term))) continue;
            const button = document.createElement('button');
            button.type = 'button';
            button.draggable = true;
            button.dataset.asset = asset.id;
            const picture = this.assetPicture(asset);
            button.append(picture, document.createTextNode(asset.name[this.language]));
            button.onclick = () => this.add('equipment', asset.id);
            button.ondragstart = event => event.dataTransfer.setData('text/schema-asset', asset.id);
            if (asset.id === selectedAsset) button.classList.add('chosen-asset');
            library.append(button);
        }
        this.root.querySelector('#library-count').textContent = library.childElementCount
            ? `${library.childElementCount} ${this.tr('assetCount')}` : this.tr('noAssets');
    }

    add(type, asset = '', point = {x: 0.15, y: 0.15}) {
        if (this.preview) return;
        if (this.pipePoints) this.cancelPipe();
        if (this.page.elements.length >= LIMITS.elements) return this.feedback('150 / page', true);
        this.remember();
        const element = newElement(type, asset);
        if (type === 'equipment') {
            element.w = 150 / this.size.width;
            element.h = 150 / this.size.height;
            const n = element.manifold?.circuits || this.state.catalog.elements.find(item => item.id === asset)?.proceduralManifold;
            if (n) {
                element.w = Math.min(.9, manifoldWidth(n) / this.size.width);
                element.h = 224 / this.size.height;
            }
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
        this.pipePoints = null;
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
        const size = equipmentImageSize(element.manifold?.circuits || asset.proceduralManifold,
            element.w * this.size.width, element.h * this.size.height);
        const dx = (p.x - .5) * size.width, dy = (p.y - .5) * size.height;
        return {x: element.x + element.w / 2 + (dx * Math.cos(radians) - dy * Math.sin(radians)) / this.size.width,
            y: element.y + element.h / 2 + (dx * Math.sin(radians) + dy * Math.cos(radians)) / this.size.height};
    }

    updateConnections(element) {
        for (const pipe of this.page.elements.filter(item => item.type === 'pipe')) {
            const asset = this.state.catalog.elements.find(item => item.id === element.asset);
            const count = asset ? elementPorts(element, asset).length : 0;
            for (const key of ['source', 'target']) if (pipe[key]?.element === element.id && pipe[key].port >= count) pipe[key] = null;
            if (pipe.source?.element === element.id) moveLinkedPoint(this.page, pipe.points[0], this.portPoint(element, pipe.source.port));
            if (pipe.target?.element === element.id) moveLinkedPoint(this.page, pipe.points.at(-1), this.portPoint(element, pipe.target.port));
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
                    moveLinkedPoint(this.page, previous, point);
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
        refreshBridges(this.page);
        const {width: w, height: h} = this.size;
        this.canvas.replaceChildren();
        const defs = svgNode('defs');
        const gradient = svgNode('linearGradient', {id: 'page-bg', x2: '1', y2: '1'});
        const colors = this.dark ? ['#17232f', '#25394a'] : this.page.background === 'warm' ? ['#faf8f1', '#e9e1d7'] : ['#f7fafc', '#dfeaf0'];
        gradient.append(svgNode('stop', {offset: '0', 'stop-color': colors[0]}), svgNode('stop', {offset: '1', 'stop-color': this.page.background === 'plain' ? colors[0] : colors[1]}));
        defs.append(gradient);
        this.canvas.append(defs, svgNode('rect', {width: w, height: h, fill: 'url(#page-bg)'}));
        const markers = new Map();
        const bridgeDots = [];
        for (const element of this.page.elements) {
            const group = svgNode('g', {'data-element': element.id, class: element.type + (element.id === this.selected ? ' selected' : '')});
            if (element.type === 'pipe') {
                const {route, bridges} = pipeGeometry(element,this.size);
                const coords = points => points.map(p => `${p.x},${p.y}`).join(' ');
                for (const {arc,point} of bridges) {
                    const mark = svgNode('g', {'data-crossing':point.link});
                    for (const [color,width] of [[colors[0],element.width+4],[element.color,element.width]])
                        mark.append(svgNode('polyline',{points:coords(arc), fill:'none',stroke:color,
                            'stroke-width':width,'stroke-linecap':'round','stroke-linejoin':'round'}));
                    markers.set(point.link, mark);
                }
                for (const p of element.points) if (p.kind === 'node') {
                    markers.set(p.link,svgNode('circle',{'data-crossing':p.link,cx:p.x*w,cy:p.y*h,
                        r:Math.max(4,element.width*.75+1),fill:element.color,stroke:colors[0],'stroke-width':1}));
                }
                const path = svgNode('polyline', {points: coords(route),
                    stroke: element.color, 'stroke-width': element.width, fill: 'none', 'stroke-linejoin': 'round', 'stroke-linecap': 'round'});
                const luminance = pipeLuminance(element.color);
                if (this.dark ? luminance < 70000 : luminance > 205000) {
                    const outline = path.cloneNode(false);
                    outline.setAttribute('stroke', this.dark ? '#9baab5' : '#738391');
                    outline.setAttribute('stroke-width', element.width + 2);
                    group.append(outline);
                }
                path.dataset.pipeBody = '';
                group.append(path);
                group.append(svgNode('polyline', {points: path.getAttribute('points'), stroke: 'transparent', 'stroke-width': Math.max(16, element.width), fill: 'none'}));
                if (element.activity || element.power) {
                    for (let index = 0; index < 3; index++) group.append(svgNode('circle', {
                        r: Math.max(2, Math.round(element.width * .3)), fill: luminance >= 145000 ? '#15202b' : '#ffffff',
                        stroke: element.width < 4 ? element.color : 'none', 'stroke-width': 1,
                        class: 'flow-dot', 'data-dot': index, style: 'display:none'}));
                }
                if (bridges.length) for (const dot of group.querySelectorAll('[data-dot]')) {
                    dot.id = 'bridge-dot-' + element.id + '-' + dot.dataset.dot;
                    bridgeDots.push(svgNode('use', {href:'#'+dot.id,'pointer-events':'none'}));
                }
                if (element.id === this.selected && !this.preview) element.points.forEach((point, index) => group.append(svgNode('circle', {
                    cx: point.x * w, cy: point.y * h, r: 6, class: 'handle', 'data-vertex': index})));
            } else {
                const x = element.x * w, y = element.y * h, width = element.w * w, height = element.h * h;
                const drawing = svgNode('g', {transform: `translate(${x},${y}) rotate(${element.rotation},${width / 2},${height / 2})`});
                if (element.type === 'equipment' && element.manifold) {
                    const m = element.manifold;
                    const {width: bodyW, height: bodyH, scale} = equipmentImageSize(m.circuits,width,height);
                    const body = svgNode('g', {transform: `translate(${(width-bodyW)/2},${(height-bodyH)/2})`});
                    drawing.append(body);
                    body.append(svgNode('image', {href: manifoldImage(m), width: bodyW, height: bodyH, preserveAspectRatio: 'none'}));
                    const tx = bodyW / 2, ty = .055 * bodyH;
                    body.append(svgNode('text', {x: tx, y: ty, 'text-anchor': 'middle',
                        fill: this.dark ? '#dfecf4' : '#354e60', 'font-size': 12*scale,
                        transform: `rotate(${-element.rotation},${tx},${ty})`}, this.tr(m.variant)));
                    for (const part of manifoldParts(m)) body.append(svgNode('rect', {
                        x: part.x * bodyW, y: part.y * bodyH, width: part.w * bodyW, height: part.h * bodyH,
                        rx: Math.min(3,part.w * bodyW * .15), fill: part.color,
                        stroke: '#67747d', 'stroke-width': .6,
                        ...(part.head === undefined ? {} : {'data-head-indicator': part.head})}));
                    m.heads.forEach((head, index) => {
                        const cx = manifoldCenter(m.circuits,index)*bodyW, cy = 155/224*bodyH;
                        body.append(svgNode('text', {x: cx, y: cy, 'text-anchor': 'middle',
                            'dominant-baseline': 'central', fill: '#26333d', 'font-size': 13*scale,
                            transform: `rotate(${-element.rotation},${cx},${cy})`, 'data-head-label': index}));
                        const ix = cx, iy = .12*bodyH;
                        body.append(svgNode('text', {x: ix, y: iy, 'text-anchor': 'middle',
                            fill: this.dark ? '#dfecf4' : '#354e60', 'font-size': 11*scale,
                            transform: `rotate(${-element.rotation},${ix},${iy})`}, index+1));
                    });
                    if (!this.preview) elementPorts(element, {}).forEach((p,index) => body.append(svgNode('circle', {
                        cx: p.x*bodyW,cy: p.y*bodyH,r:5,class:'port','data-port':index})));
                    if (element.label) group.append(svgNode('text', {x:x+width/2,y:y+height+17,
                        'text-anchor':'middle',fill:this.dark ? '#dfecf4' : '#354e60','font-size':element.fontSize},element.label));
                } else if (element.type === 'equipment') {
                    const asset = this.state.catalog.elements.find(item => item.id === element.asset);
                    const side = Math.min(width, height);
                    const {width: imageW, height: imageH} = equipmentImageSize(asset.proceduralManifold,width,height);
                    const offsetX = (width - imageW) / 2, offsetY = (height - imageH) / 2;
                    const equipment = svgNode('g', {transform: `translate(${offsetX},${offsetY})`});
                    equipment.append(svgNode('image', {href: `/graphics/${asset.graphics[this.dark ? 'dark' : 'light']}`, width: imageW, height: imageH, preserveAspectRatio: 'none'}));
                    if (asset.animation?.kind === 'rotor') for (const marker of asset.animation.rotors || [asset.animation]) {
                        const cx = side * marker.x, cy = side * marker.y;
                        const angle = this.motion.get(element.id)?.angle || 0;
                        const rotor = svgNode('g', {'data-rotor': '1', 'data-cx': cx, 'data-cy': cy,
                            transform: `rotate(${angle},${cx},${cy})`});
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
                    if (['status', 'spray'].includes(asset.animation?.kind)) {
                        equipment.append(svgNode('circle', {cx: side * asset.animation.x, cy: side * asset.animation.y,
                            r: Math.max(2, side * (asset.animation.kind === 'spray' ? .025 : asset.animation.radius)), fill: '#596674', stroke: '#e5eff4',
                            'stroke-width': 1, 'data-status-indicator': '1'}));
                    }
                    if (asset.animation?.kind === 'spray') {
                        for (let drop = 0; drop < SPRAY_DROPS; drop++) equipment.append(svgNode('circle', {
                            cx: side * asset.animation.x, cy: side * asset.animation.y,
                            r: Math.max(1.5, side * .0125), fill: '#249bd0', class: 'flow-dot',
                            'data-spray': drop, 'data-x': side * asset.animation.x, 'data-y': side * asset.animation.y,
                            'data-radius': side * asset.animation.radius, 'data-direction': asset.animation.direction,
                            style: 'display:none',
                        }));
                    }
                    if (!this.preview) asset.ports.forEach((port, index) => {
                        const socket = svgNode('circle', {cx: port.x * imageW, cy: port.y * imageH,
                            r: 5, class: 'port', 'data-port': index});
                        if (port.name) socket.append(svgNode('title', {}, port.name[this.language]));
                        equipment.append(socket);
                    });
                    drawing.append(equipment);
                    if (element.label) group.append(svgNode('text', {x: x + width / 2, y: y + height + 17, 'text-anchor': 'middle',
                        fill: this.dark ? '#dfecf4' : '#354e60', 'font-size': element.fontSize}, element.label));
                    if (element.showPower) group.append(svgNode('text', {x: x + width / 2, y: y + height + 36,
                        'text-anchor': 'middle', fill: this.dark ? '#dfecf4' : '#354e60', 'font-size': element.fontSize, 'data-live-power': '1'}));
                } else {
                    drawing.append(svgNode('rect', {width, height, fill: 'transparent'}));
                    const align = element.align || 'left';
                    const field = svgNode('text', {'data-field-width': width, 'xml:space': 'preserve', x: align === 'center' ? width / 2 : align === 'right' ? width : 0,
                        'text-anchor': align === 'center' ? 'middle' : align === 'right' ? 'end' : 'start',
                        y: element.fontSize, fill: this.dark && element.color === '#456172' ? '#d5e6f0' : element.color,
                        'font-size': element.fontSize, ...(element.type === 'value' ? {'data-live-value': '1'} : {})});
                    setFieldText(field, element.type === 'text' ? element.text : '—');
                    drawing.append(field);
                }
                group.append(drawing);
                if (element.id === this.selected && !this.preview) {
                    group.append(svgNode('rect', {x, y, width, height, class: 'selected-box'}));
                    group.append(svgNode('rect', {x: x + width - 6, y: y + height - 6, width: 12, height: 12, class: 'handle resize', 'data-resize': '1'}));
                }
            }
            this.canvas.append(group);
        }
        for (const mark of markers.values()) this.canvas.append(mark);
        this.canvas.append(...bridgeDots);
        this.drawPipePreview();
        this.updateValues();
    }

    syncPipeTools() {
        const drawing = this.pipePoints !== null;
        this.canvas.classList.toggle('drawing-pipe', drawing);
        this.root.querySelector('#draw-pipe').setAttribute('aria-pressed', String(drawing));
        this.root.querySelector('#pipe-tools').hidden = !drawing;
        this.root.querySelector('#finish-pipe').disabled = !drawing || this.pipePoints.length < 2;
    }

    cancelPipe() {
        this.pipePoints = this.pipeCursor = this.pipeSource = this.branchStart = null;
        this.syncPipeTools();
        this.drawPipePreview();
    }

    pipeRoute(points) {
        if (points.length !== 2 || points[0].x === points[1].x || points[0].y === points[1].y) return points;
        return [points[0], {x: points[1].x, y: points[0].y}, points[1]];
    }

    drawPipePreview() {
        this.canvas.querySelector('[data-pipe-preview]')?.remove();
        this.canvas.querySelector('[data-pipe-last]')?.remove();
        if (!this.pipePoints?.length) return;
        const points = this.pipeCursor ? [...this.pipePoints, this.pipeCursor] : this.pipePoints;
        this.canvas.append(svgNode('polyline', {points: this.pipeRoute(points).map(point => `${point.x * this.size.width},${point.y * this.size.height}`).join(' '),
            stroke: this.pipeStyle.color, 'stroke-width': this.pipeStyle.width, fill: 'none', 'stroke-dasharray': '8 4', 'pointer-events': 'none', 'data-pipe-preview': '1'}));
        const last = this.pipePoints.at(-1);
        this.canvas.append(svgNode('circle', {cx: last.x*this.size.width, cy: last.y*this.size.height,
            r: 4, fill: '#fff', stroke: '#237fab', 'stroke-width': 2, 'pointer-events': 'none', 'data-pipe-last': '1'}));
    }

    click(event) {
        if (this.preview || this.pipePoints === null || this.pipePoints === undefined) return;
        const port = event.target.closest('[data-port]');
        const equipment = port && this.page.elements.find(item => item.id === port.closest('[data-element]').dataset.element);
        const endpoint = equipment && {element: equipment.id, port: Number(port.dataset.port)};
        const mouse = this.point(event);
        let branch = !endpoint && nearbySegments(this.page,mouse,this.size,6)[0];
        const marker = event.target.closest('[data-crossing]')?.dataset.crossing;
        if (!endpoint && marker) {
            const members = pipeLinks(this.page).get(marker);
            const {pipe,point} = members.find(m=>m.point.kind==='over') || members[0];
            branch = nearbySegments({elements:[pipe]},point,this.size,1)[0];
        }
        if (branch) {
            // A már létező töréspontot érintéssel is pontosan el lehet találni.
            for (const t of [0,1]) {
                const vertex = branch.pipe.points[branch.index+t];
                if (Math.hypot((vertex.x-branch.point.x)*this.size.width,
                               (vertex.y-branch.point.y)*this.size.height) <= 4) {
                    branch = {...branch,t,point:{x:vertex.x,y:vertex.y}};
                    break;
                }
            }
            const kind = existingVertex(branch)?.kind;
            if ((kind === 'over' || kind === 'under') && !confirm(this.tr('joinAtBridge'))) return;
        }
        const point = endpoint ? this.portPoint(equipment, endpoint.port) : branch ? branch.point : mouse;
        const last = this.pipePoints.at(-1);
        if (last && Math.hypot(point.x-last.x, point.y-last.y) <= .001) return;
        if (this.pipePoints.length >= LIMITS.points) return this.feedback(this.tr('pointLimit'), true);
        this.pipePoints.push(point);
        this.pipeCursor = null;
        if (this.pipePoints.length === 1) {
            this.pipeSource = endpoint || null;
            this.branchStart = branch || null;
        } else if (endpoint || branch) return this.finishPipe(endpoint, branch);
        this.syncPipeTools();
        this.drawPipePreview();
    }

    finishPipe(target = null, branchEnd = null) {
        if (!this.pipePoints || this.pipePoints.length < 2 || this.page.elements.length >= LIMITS.elements) return;
        const before = JSON.stringify(this.state.project);
        const pipe = newElement('pipe');
        Object.assign(pipe, this.pipeStyle, {points: this.pipeRoute(this.pipePoints), source: this.pipeSource, target});
        try {
            this.page.elements.push(pipe);
            const attachments = [[this.branchStart,pipe.points[0]],[branchEnd,pipe.points.at(-1)]]
                .filter(([hit]) => hit).sort(([a],[b]) => b.index-a.index || b.t-a.t);
            for (const [hit, vertex] of attachments) {
                // Az első beszúrás után ugyanannak a csőnek a szakaszindexe megváltozhat.
                const current = nearbySegments({elements:[hit.pipe]},hit.point,this.size,1)[0];
                const previous = existingVertex(current);
                const link = previous?.link || newId('j');
                if (previous?.kind === 'over' || previous?.kind === 'under')
                    for (const member of pipeLinks(this.page).get(link)) member.point.kind = 'node';
                attachVertex(current,link,'node',LIMITS.points);
                Object.assign(vertex,{...current.point,link,kind:'node'});
            }
            shareFlow(this.page,this.branchStart?.pipe || branchEnd?.pipe || pipe);
            validateProject(this.state.project,this.state.catalog);
        } catch (error) {
            this.state.project = JSON.parse(before);
            this.cancelPipe();
            this.draw();
            return this.feedback(this.tr(error.message),true);
        }
        this.remember(before);
        this.selected = pipe.id;
        this.cancelPipe();
        this.draw();
        this.inspect();
    }

    inspectCrossing(hits, members) {
        this.selected = null;
        const panel = this.root.querySelector('#inspector'), t = this.tr;
        const endsHere = hit => (hit.index === 0 && hit.t <= 1e-8) ||
            (hit.index === hit.pipe.points.length-2 && hit.t >= 1-1e-8);
        const branch = members ? members.length !== 2 || members.some(m =>
            !m.index || m.index === m.pipe.points.length-1) : hits.some(endsHere);
        const pipes = members ? members.map(m => m.pipe) : hits.map(hit => hit.pipe);
        const isNode = members?.[0].point.kind === 'node';
        const current = members ? (isNode ? 'node' : 'bridge') : null;
        panel.innerHTML = `<fieldset><legend>${t('pipeCrossing')}</legend>
            <label>${t('crossingMode')}<select id="crossing-mode">
            ${current ? '' : '<option value="">—</option>'}
            <option value="node" ${isNode?'selected':''}>${t('pipeNode')}</option>
            ${branch?'':`<option value="bridge" ${current==='bridge'?'selected':''}>${t('pipeBridge')}</option>`}
            </select></label><p class="hint">${t(branch?'branchHint':'crossingHint')}</p></fieldset>`;
        if (!isNode && !sameFlow(pipes[0],pipes[1])) {
            const select = document.createElement('select');
            select.id = 'flow-owner';
            for (const [index,pipe] of pipes.entries()) {
                const option = document.createElement('option');
                option.value = index;
                option.textContent = (pipe.label || t('pipe')+' '+(this.page.elements.indexOf(pipe)+1)) +
                    ' · ' + this.bindingName(pipe.activity || pipe.power?.source);
                select.append(option);
            }
            const label = document.createElement('label');
            label.textContent=t('flowOwner');
            label.append(select);
            panel.querySelector('fieldset').append(label);
        }
        panel.querySelector('#crossing-mode').onchange = event => {
            const kind = event.target.value;
            if (!kind) return;
            const before = JSON.stringify(this.state.project);
            const link = members?.[0].point.link || newId('j');
            try {
                if (members) members.forEach((m,i) => {m.point.kind = kind==='node'?'node':i?'over':'under';});
                else hits.forEach((hit,i) => attachVertex(hit,link,kind==='node'?'node':i?'over':'under',LIMITS.points));
                if (kind === 'node') shareFlow(this.page,pipes[Number(panel.querySelector('#flow-owner')?.value || 0)]);
                validateProject(this.state.project,this.state.catalog);
                this.remember(before);
                this.draw();
                this.inspectCrossing(null,pipeLinks(this.page).get(link));
            } catch (error) {
                this.state.project = JSON.parse(before);
                this.draw(); this.inspect(); this.feedback(t(error.message),true);
            }
        };
    }

    pointerDown(event) {
        if (this.preview || this.pipePoints) return;
        const mouse = this.point(event);
        const marker = event.target.closest('[data-crossing]')?.dataset.crossing;
        const hits = crossingAt(this.page,mouse,this.size,6);
        const link = marker || hits?.map(existingVertex).find(p => p?.link)?.link;
        if ((hits || link) && !event.target.hasAttribute('data-vertex')) {
            this.drag = null;
            this.inspectCrossing(hits,link && pipeLinks(this.page).get(link));
            return;
        }
        const group = event.target.closest('[data-element]');
        this.selected = group?.dataset.element || null;
        this.inspect();
        if (!this.element) { this.draw(); return; }
        this.drag = {start: this.point(event), original: clone(this.element), vertex: event.target.dataset.vertex,
            resize: !!event.target.dataset.resize};
        this.canvas.setPointerCapture(event.pointerId);
        this.draw();
    }

    pointerMove(event) {
        if (this.pipePoints?.length && event.pointerType !== 'touch') {
            this.pipeCursor = this.point(event);
            this.drawPipePreview();
            return;
        }
        if (!this.drag || !this.element) return;
        const point = this.point(event), original = this.drag.original;
        const dx = point.x - this.drag.start.x, dy = point.y - this.drag.start.y;
        if (!this.drag.moved) {
            if (Math.hypot(dx*this.size.width, dy*this.size.height) < 2) return;
            this.remember();
            this.drag.moved = true;
        }
        if (this.element.type === 'pipe') {
            if (this.drag.vertex !== undefined) {
                const vertex = Number(this.drag.vertex);
                moveLinkedPoint(this.page, this.element.points[vertex], point);
                if (vertex === 0) this.element.source = null;
                if (vertex === this.element.points.length - 1) this.element.target = null;
            } else {
                const minX = Math.min(...original.points.map(p => p.x)), maxX = Math.max(...original.points.map(p => p.x));
                const minY = Math.min(...original.points.map(p => p.y)), maxY = Math.max(...original.points.map(p => p.y));
                original.points.forEach((p,i) => moveLinkedPoint(this.page, this.element.points[i], {
                    x:p.x+clamp(dx,-minX,1-maxX), y:p.y+clamp(dy,-minY,1-maxY)}));
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

    bindingName(binding, fallback = 'none') {
        if (!binding) return this.tr(fallback);
        const source = this.state.sources.find(item => item.uuid === binding.uuid);
        return source ? [source.room, source.name, source.state].filter(Boolean).join(' · ') : binding.uuid;
    }

    inspect() {
        const panel = this.root.querySelector('#inspector');
        const e = this.element, t = this.tr;
        this.root.querySelector('#library .chosen-asset')?.classList.remove('chosen-asset');
        if (e?.asset) this.root.querySelector(`#library [data-asset="${CSS.escape(e.asset)}"]`)?.classList.add('chosen-asset');
        if (!e) { panel.textContent = t('select'); return; }
        const dimensionField = (key, title, axis) => `<label>${t(title)} (px)<input data-property="${key}" data-axis="${axis}" type="number" min="${Math.ceil(.005*this.size[axis])}" max="${this.size[axis]}" step="1" value="${Math.round(e[key]*this.size[axis])}"></label>`;
        const field = (key, title, type = 'text') => `<label>${t(title)}<input data-property="${key}" type="${type}" ${e.type === 'pipe' && key === 'color' ? 'list="pipe-colors"' : ''} ${e.type === 'pipe' && key === 'width' ? 'min="1" max="30" step="1"' : ''} value="${this.escape(e[key])}"></label>`;
        const asset = this.state.catalog.elements.find(item => item.id === e.asset);
        panel.innerHTML = `<p class="element-caption">${this.escape(asset?.name[this.language] || t(e.type))}</p><fieldset><legend>${t('appearance')}</legend>${field('label', 'label')}
            ${e.type === 'text' ? `<label>${t('text')}<textarea data-property="text" maxlength="1000"></textarea></label>` : ''}${e.type === 'equipment' ? '' : field('color', 'color', 'color')}
            <div class="small-grid">${e.type === 'pipe' ? field('width', 'lineWidth', 'number') : field('fontSize', 'fontSize', 'number')}</div>
            ${e.type !== 'pipe' ? `<div class="small-grid">${dimensionField('w','width','width')}${dimensionField('h','height','height')}</div>` : `<label>${t('flowDirection')}<select id="pipe-direction"><option value="1">${t('forwardFlow')}</option><option value="-1" ${e.direction < 0 ? 'selected' : ''}>${t('backwardFlow')}</option></select></label>`}
            ${['text', 'value'].includes(e.type) ? `<label>${t('alignment')}<select data-property="align">${['left', 'center', 'right'].map(value => `<option value="${value}" ${(e.align || 'left') === value ? 'selected' : ''}>${t(value)}</option>`).join('')}</select></label>` : ''}
            <label>${t('rotate')}<select id="element-rotation">${[0,90,180,270].map(n => `<option value="${n}" ${n === e.rotation ? 'selected' : ''}>${n}°</option>`).join('')}</select></label></fieldset>
            ${e.type === 'value' ? `<fieldset><legend>${t('value')}</legend>` + `${field('unit', 'unit')}${field('decimals', 'decimals', 'number')}<button class="binding-button" data-binding="value">${t('source')}: ${this.escape(this.bindingName(e.value))}</button></fieldset>` : ''}
            ${['equipment', 'pipe'].includes(e.type) && !e.manifold ? `<fieldset><legend>${t('signals')}</legend><label>${t('activity')}<button class="binding-button" data-binding="activity">${this.escape(this.bindingName(e.activity))}</button></label>
            <label>${t('power')}<button class="binding-button" data-binding="power">${this.escape(this.bindingName(e.power?.source))}</button></label>
            ${e.power ? `<div class="small-grid"><label>${t('minimum')}<input id="power-min" type="number" step="any" value="${e.power.minimum}"></label><label>${t('maximum')}<input id="power-max" type="number" step="any" value="${e.power.maximum}"></label></div>
                <label><input id="show-power" type="checkbox" ${e.showPower ? 'checked' : ''}>${t('showPower')}</label>` : ''}<p class="hint">${t('sourceHint')}</p></fieldset>` : ''}`;
        if (e.type === 'pipe' && e.points.some(p=>p.kind==='node')) {
            const hint = document.createElement('p');
            hint.className = 'hint';
            hint.textContent = t('sharedFlowHint');
            panel.querySelector('[data-binding="activity"]').closest('fieldset').prepend(hint);
        }
        if (asset) panel.querySelector('.element-caption').prepend(this.assetPicture(asset, e.manifold));
        panel.querySelector('#element-rotation').onchange = event => {
            const before = clone(this.state.project);
            this.remember();
            try {
                Object.assign(e, rotateElement(e, Number(event.target.value), this.size));
                if (e.type === 'pipe') for (const p of e.points) if (p.link) moveLinkedPoint(this.page, p,p);
                this.updateConnections(e);
                refreshBridges(this.page);
                validateProject(this.state.project, this.state.catalog);
                this.draw(); this.inspect();
            } catch(error) { this.state.project = before; this.feedback(error.message, true); this.inspect(); }
        };
        if (e.manifold) this.inspectManifold(panel, e);
        if (e.position || asset?.animation?.kind === 'position') {
            panel.insertAdjacentHTML('beforeend', `<fieldset><legend>${t('valvePosition')}</legend><label>${t('position')}<button class="binding-button" data-binding="position">${this.escape(this.bindingName(e.position?.source))}</button></label>
                ${e.position ? `<div class="small-grid"><label>${t('minimum')}<input id="position-min" type="number" step="any" value="${e.position.minimum}"></label><label>${t('maximum')}<input id="position-max" type="number" step="any" value="${e.position.maximum}"></label></div>
                <label><input id="position-reverse" type="checkbox" ${e.position.reverse ? 'checked' : ''}>${t('positionReverse')}</label>` : ''}<p class="hint">${t('positionHint')}</p></fieldset>`);
        }
        if (e.type === 'text') panel.querySelector('[data-property="text"]').value = e.text;
        panel.querySelectorAll('[data-property]').forEach(input => input.onchange = () => {
            const key = input.dataset.property;
            if (input.type === 'number' && (!input.value || !input.validity.valid)) {
                input.reportValidity();
                this.inspect();
                return;
            }
            const value = input.dataset.axis ? Number(input.value) / this.size[input.dataset.axis]
                : input.type === 'number' ? Number(input.value) : input.value;
            if (e[key] === value) return;
            const before = clone(this.state.project);
            e[key] = value;
            try {
                if (e.type !== 'pipe') Object.assign(e, rotateElement(e, e.rotation, this.size));
                this.updateConnections(e);
                refreshBridges(this.page);
                validateProject(this.state.project, this.state.catalog);
                this.remember(JSON.stringify(before));
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
            shareFlow(this.page,e);
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
                shareFlow(this.page,e);
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
        if (e.type === 'pipe') panel.querySelector('#pipe-direction').onchange = event => {
            this.remember();
            e.direction = Number(event.target.value);
            shareFlow(this.page,e);
            this.draw();
        };
    }

    inspectManifold(panel, e) {
        const t = this.tr, m = e.manifold;
        panel.insertAdjacentHTML('beforeend', `<fieldset><legend>${t('manifold')}</legend>
            <label>${t('circuits')}<select id="manifold-circuits">${Array.from({length:15},(_,i)=>i+2).map(n=>`<option ${n===m.circuits?'selected':''}>${n}</option>`).join('')}</select></label>
            <label>${t('variant')}<select id="manifold-variant">${['heating','floor','ceiling-heat','ceiling-cool'].map(v=>`<option value="${v}" ${v===m.variant?'selected':''}>${t(v)}</option>`).join('')}</select></label>
            <label><input type="checkbox" id="manifold-connections" ${m.connections?'checked':''}>${t('connections')}</label>
            <p class="hint">${t('headHint')}</p>
            ${m.heads.map((head,i)=>`<label>${t('circuit')} ${i+1} (%)<input data-head-percent="${i}" type="number" min="0" max="100" step="any" ${head.source?'disabled':''} value="${head.percent}"></label><button type="button" data-head-source="${i}">${t('source')}: ${this.escape(this.bindingName(head.source,'manual'))}</button>`).join('')}</fieldset>`);
        const change = update => {
            const before = clone(this.state.project);
            this.remember();
            try {update(); this.updateConnections(e); validateProject(this.state.project,this.state.catalog);this.draw();this.inspect();}
            catch(error) {this.state.project=before;this.feedback(error.message,true);this.inspect();}
        };
        panel.querySelector('#manifold-circuits').onchange = event => change(()=>{e.manifold=newManifold(Number(event.target.value),m);
            e.w=Math.min(this.size.width*.9,manifoldWidth(e.manifold.circuits))/this.size.width;
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
            const field = group.querySelector('[data-live-value]');
            if (field) setFieldText(field, valueText(value, element.decimals, element.unit));
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
            const power = positionState(element.power?.source, this.state.values,
                element.power?.minimum, element.power?.maximum, false, this.state.device.connected);
            setText('[data-live-power]', valueText({valid: power.known, value: power.percent}, 0, '%'));
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
            const drops = group.querySelectorAll('[data-spray]');
            if (drops.length && live.active) motion.phase = (motion.phase + .05 * live.speed) % 1;
            drops.forEach((dot, index) => {
                const display = live.active ? '' : 'none';
                if (dot.style.display !== display) dot.style.display = display;
                if (!live.active) return;
                const point = sprayPoint(motion.phase, index, Number(dot.dataset.direction));
                dot.setAttribute('cx', Number(dot.dataset.x) + point.x * Number(dot.dataset.radius));
                dot.setAttribute('cy', Number(dot.dataset.y) + point.y * Number(dot.dataset.radius));
            });
            if (element.type === 'pipe' && (element.activity || element.power)) {
                const path = group.querySelector('[data-pipe-body]');
                const length = Number(path.dataset.length ?? (path.dataset.length = path.getTotalLength()));
                if (live.active && length > 0) motion.phase = (motion.phase + 2.5 * live.speed / length) % 1;
                group.querySelectorAll('[data-dot]').forEach((dot, index) => {
                    dot.style.display = live.active && length > 0 ? '' : 'none';
                    if (!live.active || !length) return;
                    const phase = (motion.phase + index / 3) % 1;
                    const p = path.getPointAtLength((element.direction < 0 ? 1-phase : phase) * length);
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
        for (const peers of pipeLinks(this.page).values()) if (peers.length === 1) {
            delete peers[0].point.link;
            delete peers[0].point.kind;
        }
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
        if (element.type === 'pipe') {
            element.source = element.target = null;
            element.points = element.points.map(({x,y}) => ({x,y}));
        }
        else { element.x = clamp(element.x + 0.03, 0, 1 - element.w); element.y = clamp(element.y + 0.03, 0, 1 - element.h); }
        this.page.elements.push(element);
        this.selected = element.id;
        this.draw();
        this.inspect();
    }

    keydown(event) {
        if (document.querySelector('dialog[open]') || ['INPUT', 'TEXTAREA', 'SELECT'].includes(document.activeElement.tagName)) return;
        if (event.key === 'Escape') { this.cancelPipe(); this.drag = null; }
        if (event.key === 'Enter' && this.pipePoints && document.activeElement.tagName !== 'BUTTON') this.finishPipe();
        if (this.preview || this.pipePoints) return;
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
            const url = URL.createObjectURL(new Blob([JSON.stringify(checked)], {type: 'application/json'}));
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
            this.pipePoints = null;
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
