import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {Editor} from '../web/editor.js';
import {newElement, newManifold} from '../web/model.js';
const catalog=JSON.parse(readFileSync('web/catalog.json'));
const calibration=JSON.parse(readFileSync('catalog/port-calibration.json')).elements;
test('calibrated image pixels follow aspect fit and rotation in both panel orientations',()=>{
    for (const size of [{width:1280,height:800},{width:800,height:1280}]) {
        const context={state:{catalog},size};
        for (const asset of catalog.elements) {
            const audit=calibration[asset.id];
            assert.ok(audit.sourceSha256);
            const [iw,ih]=audit.canvas || [160,160];
            const e=newElement('equipment',asset.id);
            if (e.manifold) e.manifold=newManifold(16);
            Object.assign(e,{x:.2,y:.2,w:.25,h:.35});
            const scale=Math.min(e.w*size.width/iw,e.h*size.height/ih);
            for (const rotation of [0,90,180,270]) {
                e.rotation=rotation;
                audit.portsPx.forEach(([px,py],index)=>{
                    let dx=(px-iw/2)*scale, dy=(py-ih/2)*scale;
                    for (let t=0;t<rotation/90;t++) [dx,dy]=[-dy,dx];
                    const actual=Editor.prototype.portPoint.call(context,e,index);
                    assert.ok(Math.abs(actual.x*size.width-((e.x+e.w/2)*size.width+dx))<1e-8,asset.id);
                    assert.ok(Math.abs(actual.y*size.height-((e.y+e.h/2)*size.height+dy))<1e-8,asset.id);
                });
            }
        }
    }
});
