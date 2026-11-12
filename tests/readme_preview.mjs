/* README-bemutató: az éles modellből generált és a webes szerkesztőn
 * keresztül ténylegesen exportált, valósághű fűtési kötés. */
import assert from 'node:assert/strict';
import fs from 'node:fs/promises';
import os from 'node:os';
import path from 'node:path';
import {spawn} from 'node:child_process';
import {newElement, newManifold, newProject, elementPorts, equipmentImageSize, validateProject} from '../web/model.js';

const {chromium} = await import(process.env.SCHEMA_PLAYWRIGHT || 'playwright');
const output = path.resolve('tests/schemas/readme');
const temp = await fs.mkdtemp(path.join(os.tmpdir(), 'schema-readme-'));
const catalog = JSON.parse(await fs.readFile('web/catalog.json','utf8'));
const assets = new Map(catalog.elements.map(asset => [asset.id, asset]));
const resolution = {width:1280,height:800};
const signal = {uuid:'12345678-1234-5678-0102030405060708',scale:1,offset:0};
const hot = '#e34d4d';
const cold = '#287ac4';
const pipeWidth = 14;

function equipment(id,asset,x,y,w,h) {
    const result = newElement('equipment', asset);
    Object.assign(result,{id,x,y,w,h});
    result.activity = signal;
    return result;
}
function port(element,index) {
    const asset = assets.get(element.asset);
    const local = elementPorts(element,asset)[index];
    assert(local, 'Ismeretlen csonk: '+element.id+'/'+index);
    const image = equipmentImageSize(element.manifold?.circuits || asset.proceduralManifold,
        element.w*resolution.width, element.h*resolution.height);
    const angle = element.rotation*Math.PI/180;
    const dx=(local.x-.5)*image.width,dy=(local.y-.5)*image.height;
    return {
        x:element.x+element.w/2+(dx*Math.cos(angle)-dy*Math.sin(angle))/resolution.width,
        y:element.y+element.h/2+(dx*Math.sin(angle)+dy*Math.cos(angle))/resolution.height
    };
}
function point(x,y) {return {x,y};}
function pipe(id,points,color,source=null,target=null) {
    const item = newElement('pipe');
    Object.assign(item,{id,points,width:pipeWidth,color,activity:signal,direction:1,source,target});
    return item;
}
function textField(id,content,x,y,w,fontSize=17,color='#29445b') {
    const el = newElement('text');
    Object.assign(el,{id,text:content,x,y,w,h:.07,fontSize,color});
    return el;
}
const boiler = equipment('gazkazan','gas-boiler',.07,.22,.14,.23);
const pump = equipment('keringeto','circulation-pump',.28,.23,.13,.23);
const valve = equipment('keveroszelep','motor-valve-3',.46,.23,.13,.23);
valve.position = {source:signal,minimum:0,maximum:100,reverse:false};
const manifold = equipment('padlofutesi-oszto','configurable-manifold',.69,.23,.26,.28);
manifold.manifold = newManifold(6);
manifold.manifold.variant='floor';
[0,30,45,65,80,0].forEach((value,i)=>{manifold.manifold.heads[i].percent=value;});
const bHot = port(boiler,1), bCold = port(boiler,0);
const pIn = port(pump,0), pOut = port(pump,1);
const vHot = port(valve,0), vMixed = port(valve,1), vCold = port(valve,2);
const mHot = port(manifold,0), mCold = port(manifold,1);
const junction = {x:.525,y:.72,link:'visszatero-t-csomopont',kind:'node'};

const pipes = [
    pipe('eloremeno-kazan-szivattyu', [
        bHot,point(.205,bHot.y),point(.205,pIn.y),pIn
    ],hot,{element:boiler.id,port:1},{element:pump.id,port:0}),
    pipe('eloremeno-szivattyu-szelep', [
        pOut,point(.44,pOut.y),point(.44,vHot.y),vHot
    ],hot,{element:pump.id,port:1},{element:valve.id,port:0}),
    pipe('eloremeno-szelep-oszto', [
        vMixed,point(.625,vMixed.y),point(.625,mHot.y),mHot
    ],hot,{element:valve.id,port:1},{element:manifold.id,port:0}),
    pipe('visszatero-oszto-kazan', [
        mCold,point(.66,mCold.y),point(.66,.72),{...junction},
        point(.18,.72),point(.18,.54),point(bCold.x,.54),bCold
    ],cold,{element:manifold.id,port:1},{element:boiler.id,port:0}),
    pipe('keveroszelep-hideg-visszatero',[
        {...junction},point(vCold.x,.72),vCold
    ],cold,null,{element:valve.id,port:2})
];

const project = newProject();
project.title='Fűtés – hatkörös osztó, motoros keverőszelep';
project.pages[0] = {
    id:'lap-1',name:'Fűtési rendszer – 6 kör',background:'cool',
    elements:[
        ...pipes,boiler,pump,valve,manifold,
        textField('cim','FŰTÉS / PADLÓFŰTÉSI OSZTÓ-GYŰJTŐ',.065,.055,.74,27),
        textField('kazanfelirat','KAZÁN',.09,.165,.14,16),
        textField('szivattyufelirat','KERINGETŐ',.287,.165,.14,16),
        textField('szelep-felirat','3-JÁRATÚ KEVERŐ',.455,.165,.18,16),
        textField('oszto-felirat','6 KÖRÖS OSZTÓ',.75,.165,.17,16),
        textField('info','Előremenő (piros) / visszatérő (kék) - 14 px',.25,.82,.59,19),
        textField('ertek-cimke','SZELEPÁLLÁS',.475,.535,.16,14),
        Object.assign(newElement('value'),{id:'szelep-ertek',x:.478,y:.59,w:.1,h:.06,
            value:signal,unit:'%',decimals:0,fontSize:22,color:'#cf5149'})
    ]
};
validateProject(project,catalog);
assert.equal(pipes.length,5);
assert(pipes.every(item=>item.width===14));
const manifoldEnds = pipes.flatMap(p=>[p.source,p.target]).filter(ref=>ref?.element===manifold.id);
assert.deepEqual(manifoldEnds.map(ref=>ref.port).sort(),[0,1],
    'Csak a fő előremenő és visszatérő csonk lehet bekötve!');
for(const item of pipes) {
    for(const [key,which] of [['source',0],['target',item.points.length-1]]) {
        const ref=item[key];
        if(!ref)continue;
        const el=[boiler,pump,valve,manifold].find(e=>e.id===ref.element);
        const expected=port(el,ref.port),actual=item.points[which];
        assert(Math.hypot((expected.x-actual.x)*resolution.width,
            (expected.y-actual.y)*resolution.height)<.01,'Eltérő csonk: '+item.id);
    }
}
await fs.mkdir(output,{recursive:true});

const server=spawn('python3',['tools/dev_server.py','--demo','--port','0','--data',path.join(temp,'state.json')],{stdio:['ignore','pipe','pipe']});
let browser;
try {
    const url=await new Promise((resolve,reject)=>{
        let log='';
        server.stdout.on('data',buf=>{
            log+=buf;
            const match=log.match(/http:\/\/127\.0\.0\.1:\d+/);
            if(match)resolve(match[0]);
        });
        server.on('exit',code=>reject(Error('Fejlesztői szerver leállt: '+code)));
        server.stderr.on('data',buf=>{if(String(buf).includes('Traceback'))reject(Error(String(buf)));});
        setTimeout(()=>reject(Error('Fejlesztői szerver nem indult el')),25000).unref();
    });
    browser=await chromium.launch({headless:true,executablePath:process.env.SCHEMA_CHROME_PATH||undefined,
        args:['--no-sandbox','--disable-dev-shm-usage',...JSON.parse(process.env.SCHEMA_CHROME_ARGS||'[]')]});
    const page=await browser.newPage({viewport:{width:1600,height:1040},deviceScaleFactor:1,acceptDownloads:true});
    // A böngésző és az LVGL pontosan ugyanazt az állandó demójelet mutatja.
    await page.route('**/api/live',async route=>{
        const response=await route.fetch();
        const data=await response.json();
        data.connected=true;
        data.values={...data.values,[signal.uuid]:65};
        await route.fulfill({response,json:data});
    });
    const errors=[];
    page.on('pageerror',e=>errors.push(e.message));
    await page.goto(url+'/editor');
    await page.waitForSelector('#import-file', {state:'attached'});
    const confirm=page.waitForEvent('dialog').then(dialog=>dialog.accept());
    await page.locator('#import-file').setInputFiles({name:'futes.json',
        mimeType:'application/json',buffer:Buffer.from(JSON.stringify(project))});
    await confirm;
    await page.waitForFunction(()=>document.querySelector('#import-file').files.length===0);
    await page.waitForFunction(n=>document.querySelectorAll('#canvas [data-element]').length===n,
        project.pages[0].elements.length);
    assert.equal(await page.locator('#canvas g.pipe').count(),5);
    assert.equal(await page.locator('#canvas [data-port]').count(),2+2+3+14);
    const saveResponse=page.waitForResponse(r=>r.url()===url+'/api/project'&&r.request().method()==='POST');
    await page.locator('#save-project').click();
    assert.equal((await saveResponse).status(),200);
    async function exportFile(button,file){
        const pending=page.waitForEvent('download');
        await page.locator(button).click();
        await (await pending).saveAs(path.join(output,file));
    }
    await exportFile('#export-project','projekt.json');
    await exportFile('#export-page','lap-1.json');
    const exported=JSON.parse(await fs.readFile(path.join(output,'projekt.json'),'utf8'));
    validateProject(exported,catalog);
    assert.equal(exported.pages[0].elements.length,project.pages[0].elements.length);
    const endpoints=exported.pages[0].elements.filter(e=>e.type==='pipe')
        .flatMap(e=>[e.source,e.target]).filter(r=>r?.element===manifold.id)
        .map(r=>r.port).sort();
    assert.deepEqual(endpoints,[0,1]);
    assert(exported.pages[0].elements.filter(e=>e.type==='pipe').every(e=>e.width===14));
    await page.locator('#preview').click();
    const images=await page.locator('#canvas image').evaluateAll(nodes=>nodes.map(n=>n.getAttribute('href')));
    for(const name of images) {
        const response=await page.request.get(url+name);
        assert(response.ok(),'Nem érhető el grafika: '+name);
    }
    await page.waitForFunction(()=>document.querySelector('[data-live-value]')?.textContent.trim()==='65 %');
    await page.screenshot({path:path.join(output,'szerkeszto.jpg'),type:'jpeg',quality:86,fullPage:false});
    assert.deepEqual(errors,[]);
    console.log('VALÓDI WEBSZERKESZTŐ: ',exported.pages[0].elements.length,
        'elem, 5 db 14px-es cső, 2 osztó főport, 0 fűtésköri leágazás; export és kép OK');
} finally {
    await browser?.close();
    server.kill('SIGTERM');
    await fs.rm(temp,{recursive:true,force:true});
}
