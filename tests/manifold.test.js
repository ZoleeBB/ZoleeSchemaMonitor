import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {newProject,newElement,newManifold,elementPorts,manifoldHeadState,manifoldParts,rotateElement,validateProject,exportPage} from '../web/model.js';
import {Editor} from '../web/editor.js';
const catalog=JSON.parse(readFileSync('web/catalog.json'));
const asset=catalog.elements.find(e=>e.id==='configurable-manifold');
const binding={uuid:'12345678-1234-5678-0102030405060708',scale:100,offset:0};
test('every circuit count and heating application has exactly one head and two distinct loop ports',()=>{
    for(let n=2;n<=16;n++) for(const variant of ['heating','floor','ceiling-heat','ceiling-cool']) {
        const p=newProject(), e=newElement('equipment',asset.id);
        e.manifold=newManifold(n);e.manifold.variant=variant;
        p.pages[0].elements.push(e);
        const pipe=newElement('pipe');pipe.source={element:e.id,port:1+2*n};p.pages[0].elements.push(pipe);
        assert.equal(elementPorts(e,asset).length,2+2*n);
        assert.equal(manifoldParts(e.manifold).filter(part=>part.head!==undefined).length,n);
        validateProject(exportPage(p,p.pages[0].id),catalog);
        pipe.source.port++;assert.throws(()=>validateProject(p,catalog));
        pipe.source.port--;e.manifold.connections=false;assert.equal(elementPorts(e,asset).length,0);
        assert.throws(()=>validateProject(p,catalog));pipe.source=null;validateProject(p,catalog);
    }
});
test('per-head manual and live signals distinguish zero, tiny positive, clamping and unknown',()=>{
    const head={percent:0,source:null};assert.deepEqual(manifoldHeadState(head,{},false),{known:true,percent:0,active:false});
    head.percent=.001;assert.equal(manifoldHeadState(head,{}).active,true);
    head.source=binding;assert.equal(manifoldHeadState(head,{}).known,false);
    assert.equal(manifoldHeadState(head,{[binding.uuid]:.5}).percent,50);
    assert.equal(manifoldHeadState(head,{[binding.uuid]:.5},false).known,false);
    assert.equal(manifoldHeadState(head,{[binding.uuid]:-1}).active,false);
    assert.equal(manifoldHeadState(head,{[binding.uuid]:2}).percent,100);
    assert.equal(manifoldHeadState(head,{[binding.uuid]:Infinity}).known,false);
});
test('count changes preserve independent bindings and validators reject malformed head data',()=>{
    const p=newProject(),e=newElement('equipment',asset.id);p.pages[0].elements.push(e);
    e.manifold.heads[1]={percent:12,source:binding};e.manifold=newManifold(16,e.manifold);
    assert.deepEqual(e.manifold.heads[1].source,binding);assert.notEqual(e.manifold.heads[0],e.manifold.heads[2]);
    validateProject(p,catalog);
    for(const change of [m=>m.circuits=17,m=>m.heads.pop(),m=>m.heads[2].percent=101,m=>m.variant='pool',m=>m.connections=1,m=>m.heads[0].source={...binding,scale:NaN},m=>m.heads[0].extra=1]) {
        const copy=structuredClone(p);change(copy.pages[0].elements[0].manifold);assert.throws(()=>validateProject(copy,catalog));
    }
});
test('quarter turns preserve pixel geometry for every element, pipes rotate once and detach endpoints',()=>{
    const size={width:1280,height:800};
    for(const type of ['equipment','text','value','pipe']) {
        let e=newElement(type,type==='equipment'?asset.id:''); e.points=[{x:.3,y:.3},{x:.4,y:.3},{x:.4,y:.5}];
        e.source={element:'other',port:0};
        const original=structuredClone(e);
        for(const angle of [90,180,270,0]) e=rotateElement(e,angle,size);
        assert.equal(e.rotation,0);
        if(type==='pipe') {assert.equal(e.source,null);e.points.forEach((p,i)=>{assert.ok(Math.abs(p.x-original.points[i].x)<1e-9);assert.ok(Math.abs(p.y-original.points[i].y)<1e-9);});}
    }
    const pipe=newElement('pipe');pipe.points=[{x:0,y:0},{x:1,y:0}];
    assert.throws(()=>rotateElement(pipe,90,size));
    assert.throws(()=>rotateElement(pipe,45,size));
});

test('rotated manifold ports follow the rectangular graphic; removed ports detach without moving the pipe',()=>{
    const e=newElement('equipment',asset.id),pipe=newElement('pipe');
    Object.assign(e,{x:.3,y:.42,w:.4,h:.16,rotation:90,manifold:newManifold(16)});
    pipe.source={element:e.id,port:33};pipe.target={element:e.id,port:2};
    const context={state:{catalog},size:{width:1280,height:800},page:{elements:[e,pipe]},portPoint:Editor.prototype.portPoint};
    Editor.prototype.updateConnections.call(context,e);
    const port=elementPorts(e,asset)[33];
    assert.ok(Math.abs(pipe.points[0].x-(e.x+e.w/2-(port.y-.5)*e.h*800/1280))<1e-9);
    assert.ok(Math.abs(pipe.points[0].y-(e.y+e.h/2+(port.x-.5)*e.w*1280/800))<1e-9);
    const endpoint=structuredClone(pipe.points[0]);
    e.manifold=newManifold(2,e.manifold);Editor.prototype.updateConnections.call(context,e);
    assert.equal(pipe.source,null);assert.deepEqual(pipe.points[0],endpoint);assert.equal(pipe.target.port,2);
    e.manifold.connections=false;Editor.prototype.updateConnections.call(context,e);assert.equal(pipe.target,null);
});
