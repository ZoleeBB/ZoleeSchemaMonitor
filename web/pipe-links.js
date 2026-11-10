/* A kapcsolatok az oldal csöveinek töréspontjait kötik össze. */
export function pipeLinks(page) {
    const groups = new Map();
    for (const pipe of page.elements) {
        if (pipe.type !== 'pipe') continue;
        pipe.points.forEach((point, index) => {
            if (!point.link) return;
            if (!groups.has(point.link)) groups.set(point.link, []);
            groups.get(point.link).push({pipe, point, index});
        });
    }
    return groups;
}

function sameBinding(a, b) {
    return a === b || (!!a && !!b && a.uuid === b.uuid && a.scale === b.scale && a.offset === b.offset);
}

export function sameFlow(a, b) {
    return a.direction === b.direction && sameBinding(a.activity, b.activity) &&
        (a.power === b.power || (!!a.power && !!b.power && sameBinding(a.power.source, b.power.source) &&
            a.power.minimum === b.power.minimum && a.power.maximum === b.power.maximum));
}

export function validatePipeLinks(page) {
    for (const members of pipeLinks(page).values()) {
        const first = members[0];
        if (members.length < 2 || new Set(members.map(m => m.pipe)).size !== members.length ||
            members.some(m => Math.abs(m.point.x - first.point.x) > 1e-9 ||
                Math.abs(m.point.y - first.point.y) > 1e-9)) throw Error('pipe.link');
        if (first.point.kind === 'node') {
            if (members.some(m => m.point.kind !== 'node' || !sameFlow(first.pipe, m.pipe))) throw Error('pipe.flow');
        } else if (members.length !== 2 || !members.some(m => m.point.kind === 'over') ||
            !members.some(m => m.point.kind === 'under') ||
            members.some(m => m.index === 0 || m.index === m.pipe.points.length - 1)) throw Error('pipe.bridge');
    }
}

export function shareFlow(page, source) {
    if (source.type !== 'pipe') return;
    const links = pipeLinks(page), seen = new Set([source]);
    for (const pipe of seen) {
        for (const point of pipe.points) {
            if (point.kind === 'node') for (const member of links.get(point.link)) seen.add(member.pipe);
        }
        if (pipe === source) continue;
        pipe.direction = source.direction;
        pipe.activity = structuredClone(source.activity);
        pipe.power = structuredClone(source.power);
    }
}

const cross = (a, b) => a.x * b.y - a.y * b.x;
const delta = (a, b) => ({x: a.x - b.x, y: a.y - b.y});
const mix = (a, b, t) => ({x: a.x + (b.x - a.x) * t, y: a.y + (b.y - a.y) * t});
const pixels = (p, size) => ({x: p.x * size.width, y: p.y * size.height});

// Csak a kattintás közelében lévő szakaszok párjait kell megvizsgálni.
export function nearbySegments(page, point, size, tolerance) {
    const hits = [], mouse = pixels(point, size);
    for (const pipe of page.elements) {
        if (pipe.type !== 'pipe') continue;
        for (let i = 1; i < pipe.points.length; i++) {
            const a = pixels(pipe.points[i - 1], size), b = pixels(pipe.points[i], size);
            const d = delta(b, a), length2 = d.x * d.x + d.y * d.y;
            if (!length2) continue;
            const t = Math.max(0, Math.min(1, ((mouse.x-a.x)*d.x + (mouse.y-a.y)*d.y) / length2));
            const p = mix(a,b,t), distance = Math.hypot(mouse.x-p.x, mouse.y-p.y);
            if (distance <= Math.max(tolerance, pipe.width / 2))
                hits.push({pipe, index:i-1, t, point:mix(pipe.points[i-1],pipe.points[i],t), distance});
        }
    }
    return hits.sort((a,b) => a.distance-b.distance);
}

export function crossingAt(page, point, size, tolerance) {
    const hits = nearbySegments(page, point, size, tolerance);
    let best = null, limit = tolerance;
    for (let i = 0; i < hits.length; i++) for (let j = i+1; j < hits.length; j++) {
        const a = hits[i], b = hits[j];
        if (a.pipe === b.pipe) continue;
        const p = a.pipe.points[a.index], q = b.pipe.points[b.index];
        const r = delta(a.pipe.points[a.index+1], p), s = delta(b.pipe.points[b.index+1], q);
        const det = cross(r,s);
        if (Math.abs(det) < 1e-12) continue;
        const diff = delta(q,p), t = cross(diff,s)/det, u = cross(diff,r)/det;
        if ([t,u].some(v => v < -1e-9 || v > 1+1e-9)) continue;
        const at = mix(p,a.pipe.points[a.index+1],Math.max(0,Math.min(1,t)));
        const distance = Math.hypot((at.x-point.x)*size.width, (at.y-point.y)*size.height);
        if (distance <= limit) {
            best = [{...a,t,point:at},{...b,t:u,point:at}];
            limit = distance;
        }
    }
    return best;
}

export function existingVertex(hit) {
    if (hit.t <= 1e-8) return hit.pipe.points[hit.index];
    if (hit.t >= 1-1e-8) return hit.pipe.points[hit.index+1];
    return null;
}

export function attachVertex(hit, link, kind, maxPoints) {
    let vertex = existingVertex(hit);
    if (!vertex) {
        if (hit.pipe.points.length >= maxPoints) throw Error('pointLimit');
        vertex = {...hit.point};
        hit.pipe.points.splice(hit.index+1,0,vertex);
    }
    if (vertex.link && vertex.link !== link) throw Error('occupiedCrossing');
    Object.assign(vertex,{x:hit.point.x,y:hit.point.y,link,kind});
}

export function pipeGeometry(pipe, size) {
    const base = pipe.points.map(p => pixels(p,size)), route = [], bridges = [];
    for (let i=0; i<base.length; i++) {
        const c = base[i];
        if (pipe.points[i].kind !== 'over') { route.push(c); continue; }
        const a = delta(base[i-1],c), b = delta(base[i+1],c);
        const la = Math.hypot(a.x,a.y), lb = Math.hypot(b.x,b.y);
        const radius = Math.min(Math.max(8, pipe.width*1.4), la*.45, lb*.45);
        if (radius < 1) { route.push(c); continue; }
        const start = {x:c.x+a.x*radius/la,y:c.y+a.y*radius/la};
        const end = {x:c.x+b.x*radius/lb,y:c.y+b.y*radius/lb};
        const d = delta(end,start), length = Math.hypot(d.x,d.y);
        if (length < 1) { route.push(c); continue; }
        const sign = d.x < 0 || (!d.x && d.y < 0) ? -1 : 1;
        const arc = [];
        for (let k=0; k<=8; k++) {
            const angle = Math.PI*k/8, p = mix(start,end,(1-Math.cos(angle))/2);
            p.x += sign*d.y/length*radius*Math.sin(angle);
            p.y -= sign*d.x/length*radius*Math.sin(angle);
            arc.push(p);
        }
        route.push(...arc);
        bridges.push({arc,point:pipe.points[i]});
    }
    return {route,bridges};
}

export function moveLinkedPoint(page, point, position) {
    Object.assign(point, position);
    if (point.link) {
        for (const {point: peer} of pipeLinks(page).get(point.link))
            Object.assign(peer, {x:point.x, y:point.y});
    }
}
