import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {Editor} from '../web/editor.js';
import {newElement} from '../web/model.js';
const catalog=JSON.parse(readFileSync('web/catalog.json'));
const calibration=JSON.parse(readFileSync('catalog/port-calibration.json')).elements;
test('every source port follows square image centering at all quarter turns on both panel orientations',()=>{
    for(const size of [{width:1280,height:800},{width:800,height:1280}]) {
        const context={state:{catalog},size};
        const sources=catalog.elements.filter(a=>calibration[a.id].sourceSha256);
        assert.equal(sources.length,322);
        for(const asset of sources) {
            const e=newElement('equipment',asset.id);
            Object.assign(e,{x:.2,y:.2,w:.25,h:.35});
            const side=Math.min(e.w*size.width,e.h*size.height);
            const left=(e.x+e.w/2)*size.width-side/2, top=(e.y+e.h/2)*size.height-side/2;
            for(const rotation of [0,90,180,270]) {
                e.rotation=rotation;
                asset.ports.forEach((port,index)=>{
                    let [u,v]=[port.x,port.y];
                    for(let t=0;t<rotation/90;t++) [u,v]=[1-v,u];
                    const actual=Editor.prototype.portPoint.call(context,e,index);
                    assert.ok(Math.abs(actual.x*size.width-(left+u*side))<1e-9,asset.id);
                    assert.ok(Math.abs(actual.y*size.height-(top+v*side))<1e-9,asset.id);
                });
            }
        }
    }
});
