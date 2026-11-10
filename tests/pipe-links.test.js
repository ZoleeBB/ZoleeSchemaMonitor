import test from 'node:test';
import assert from 'node:assert/strict';
import {newProject, newElement, validateProject} from '../web/model.js';
import {crossingAt, nearbySegments, attachVertex, shareFlow, pipeGeometry} from '../web/pipe-links.js';

const size = {width:1280,height:800}, catalog = {elements:[]};
const point = (x,y) => ({x,y});
function pipe(points) {
    return {...newElement('pipe'), points};
}

test('each intersection can be configured independently; the jump is on the animated route', () => {
    const project = newProject(), page = project.pages[0];
    const a = pipe([point(.1,.5),point(.9,.5)]);
    const b = pipe([point(.3,.2),point(.3,.8),point(.7,.8),point(.7,.2)]);
    page.elements.push(a,b);
    const hits = crossingAt(page,point(.701,.501),size,6);
    assert.equal(hits.length,2);
    for (const hit of hits) attachVertex(hit,'jump',hit.pipe===a?'over':'under',64);
    const {route,bridges} = pipeGeometry(a,size);
    assert.equal(bridges.length,1);
    assert.equal(route.length,11);
    assert.ok(Math.min(...route.map(p=>p.y))<.5*size.height-7);
    assert.equal(pipeGeometry(b,size).bridges.length,0);
    assert.ok(crossingAt(page,point(.3,.5),size,6));
    assert.deepEqual(validateProject(project,catalog),project);
});

test('junction flow is transitive; a branch endpoint cannot become a bridge', () => {
    const project = newProject(), page=project.pages[0];
    const a=pipe([point(.1,.5),point(.9,.5)]), b=pipe([point(.4,.5),point(.4,.9)]);
    const c=pipe([point(.4,.7),point(.7,.7)]);
    page.elements.push(a,b,c);
    for (const [host,branch,id] of [[a,b,'one'],[b,c,'two']]) {
        const hit=nearbySegments({elements:[host]},branch.points[0],size,3)[0];
        attachVertex(hit,id,'node',64);
        Object.assign(branch.points[0],{link:id,kind:'node'});
    }
    c.direction=-1;
    c.power={source:{uuid:'12345678-1234-5678-0102030405060708',scale:1,offset:0},minimum:5,maximum:100};
    assert.throws(()=>validateProject(project,catalog));
    shareFlow(page,c);
    assert.deepEqual(a.power,c.power);
    assert.equal(a.direction,-1);
    validateProject(project,catalog);
    b.points[0].kind='over';
    a.points[1].kind='under';
    assert.throws(()=>validateProject(project,catalog));
});
