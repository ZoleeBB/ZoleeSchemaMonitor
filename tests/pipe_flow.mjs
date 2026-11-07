// A próbaséma a normál importon, mentésen és szerkesztőeszközökön keresztül fut.
import assert from 'node:assert/strict';
import fs from 'node:fs/promises';
import {spawn} from 'node:child_process';
import {once} from 'node:events';
import os from 'node:os';
import path from 'node:path';
import {createInterface} from 'node:readline';

async function testPipeFlow(browser, url) {
    const page = await browser.newPage({viewport:{width:1600,height:1180},colorScheme:'light'});
    const fixture = 'tests/schemas/csoaramlas/aramlasi-proba.json';
    const expected = JSON.parse(await fs.readFile(fixture));
    const pipes = expected.pages[0].elements.filter(e=>e.type==='pipe');
    const signal = pipes[0].activity.uuid;
    let connected = true, value = 65;
    await page.route('**/api/live', route => route.fulfill({json:{connected,values:{[signal]:value}}}));
    try {
        await page.goto(url+'/settings');
        await page.locator('[name="rotation"]').selectOption('270');
        const settingsSaved = page.waitForResponse(r => r.url().endsWith('/api/settings') && r.request().method()==='POST');
        await page.locator('#settings-form button').last().click();
        assert.ok((await settingsSaved).ok());
        await page.goto(url+'/editor');
        page.once('dialog', d=>d.accept());
        await page.locator('#import-file').setInputFiles(fixture);
        const groups = page.locator('#canvas .pipe');
        await groups.last().waitFor();
        assert.equal(await groups.count(),pipes.length);
        const saved = page.waitForResponse(r=>r.url().endsWith('/api/project') && r.request().method()==='POST');
        await page.locator('#save-project').click();
        assert.ok((await saved).ok());
        await page.waitForFunction(()=>!document.querySelector('#save-project').classList.contains('dirty'));
        await page.reload();
        await page.waitForSelector('[data-element="pipe-5"] [data-dot="0"]:visible');
        for (const [i,pipe] of pipes.entries()) {
            const group=groups.nth(i), line=group.locator('[data-pipe-body]'), dot=group.locator('[data-dot="0"]');
            assert.equal(await line.getAttribute('stroke'),pipe.color);
            assert.equal(await line.getAttribute('stroke-width'),String(pipe.width));
            assert.equal(await dot.getAttribute('r'),String(Math.max(2,Math.round(.3*pipe.width))));
            assert.equal(await dot.getAttribute('fill'),i===3 || i===4 ? '#15202b' : '#ffffff');
            const firstX=Number(await dot.getAttribute('cx'));
            await page.waitForFunction(({id,firstX,dir})=>{
                const x=Number(document.querySelector(`[data-element="${id}"] [data-dot="0"]`).getAttribute('cx'));
                const dx=(x-firstX)*dir;
                return dx>1 && dx<100;
            },{id:pipe.id,firstX,dir:pipe.direction});
        }
        await page.locator('#night').click();
        assert.equal(await groups.last().locator('polyline').first().getAttribute('stroke'),'#9baab5');
        await page.locator('#night').click();
        assert.equal(await groups.nth(4).locator('polyline').first().getAttribute('stroke'),'#738391');
        for (const status of ['off','offline','active']) {
            connected=status!=='offline'; value=status==='off'?0:65;
            await page.waitForFunction(status=>[...document.querySelectorAll('#canvas .pipe')].every(group=>
                group.dataset.known===String(status!=='offline') &&
                [...group.querySelectorAll('[data-dot]')].every(dot=>(dot.style.display!=='none')===(status==='active'))
            ),status);
            if (status === 'off') {
                const rotor = page.locator('[data-element="pump-0"] [data-rotor]');
                const stopped = await rotor.getAttribute('transform');
                await page.locator('#night').click();
                assert.equal(await rotor.getAttribute('transform'), stopped);
                await page.locator('#night').click();
                assert.equal(await rotor.getAttribute('transform'), stopped);
            }
        }
        await page.locator('#asset-search').fill('keringeto szivattyu');
        assert.ok(await page.locator('#library button').count()>0);
        await page.locator('#asset-search').fill('no-such-schema-device');
        assert.equal(await page.locator('#library button').count(),0);
        assert.ok((await page.locator('#library-count').textContent()).length>0);
        await page.locator('#asset-search').fill('keringeto szivattyu');
        await page.locator('#preview').click();
        await page.locator('#preview').click();
        assert.equal(await page.locator('#asset-search').inputValue(),'keringeto szivattyu');
        await page.locator('[data-element="pump-2"] image').click();
        assert.equal(await page.locator('#save-project.dirty').count(),0);
        await page.locator('.element-caption img').evaluate(img=>img.decode());
        assert.equal(await page.locator('#inspector [data-property="w"]').inputValue(),'76');
        assert.equal(await page.locator('#inspector [data-binding="position"]').count(),0);
        await page.locator('#asset-search').fill('szivattyu');
        const fits = await page.locator('.element-caption .library-picture').evaluate(box => {
            const image=box.querySelector('img').getBoundingClientRect(), frame=box.getBoundingClientRect();
            return image.width<=frame.width && image.height<=frame.height;
        });
        assert.ok(fits);
        await page.evaluate(()=>window.scrollTo(0,0));
        await fs.mkdir('.browser-test', {recursive:true});
        await page.screenshot({path:'.browser-test/csoaramlas-szerkeszto.jpg',type:'jpeg',quality:88});
        await page.locator('#draw-pipe').click();
        assert.ok(await page.locator('#finish-pipe').isDisabled());
        await page.locator('#pipe-width').fill('12');
        await page.locator('#pipe-width').blur();
        await page.locator('#pipe-color').fill('#258653');
        const canvas=page.locator('#canvas');
        await canvas.scrollIntoViewIfNeeded();
        const bounds=await canvas.boundingBox();
        const point=(x,y)=>({x:bounds.width*x,y:bounds.height*y});
        await canvas.click({position:point(.55,.10)});
        await canvas.click({position:point(.74,.12)});
        assert.equal(await canvas.locator('[data-pipe-preview]').getAttribute('stroke-width'),'12');
        await page.locator('#finish-pipe').click();
        const line=groups.last().locator('[data-pipe-body]');
        assert.equal(await line.getAttribute('stroke'),'#258653');
        assert.equal(await line.getAttribute('stroke-width'),'12');
        await page.locator('#pipe-direction').selectOption('-1');
        const exported=page.waitForEvent('download');
        await page.locator('#export-page').click();
        const edited=JSON.parse(await fs.readFile(await (await exported).path()));
        assert.equal(edited.pages[0].elements.at(-1).direction,-1);
        await page.locator('#draw-pipe').click();
        await page.locator('#cancel-pipe').click();
        assert.equal(await canvas.locator('[data-pipe-preview]').count(),0);
        assert.equal(await groups.count(),pipes.length+1);
    } finally {
        await page.close();
    }

    const touch = await browser.newPage({viewport:{width:390,height:844},isMobile:true,hasTouch:true});
    try {
        await touch.goto(url+'/editor');
        await touch.locator('#draw-pipe').tap();
        const canvas = touch.locator('#canvas');
        await canvas.tap({position:{x:35,y:50}});
        assert.ok(await canvas.locator('[data-pipe-last]').isVisible());
        await canvas.tap({position:{x:120,y:80}});
        await touch.locator('#finish-pipe').tap();
        assert.equal(await touch.locator('#canvas .pipe').count(),pipes.length+1);
        assert.ok(await touch.evaluate(()=>document.documentElement.scrollWidth<=innerWidth+1));
    } finally {
        await touch.close();
    }
}

const {chromium} = await import(process.env.SCHEMA_PLAYWRIGHT || 'playwright');
const directory = await fs.mkdtemp(path.join(os.tmpdir(), 'schema-pipe-flow-'));
const server = spawn('python3', ['tools/dev_server.py', '--demo', '--port', '0', '--data', path.join(directory, 'state.json')],
    {stdio:['ignore','pipe','ignore']});
let browser;
try {
    const lines = createInterface({input:server.stdout});
    const [line] = await Promise.race([
        once(lines, 'line'),
        once(server, 'exit').then(([code])=>{throw Error(`Demo server exited: ${code}`);}),
        new Promise((_,reject)=>setTimeout(()=>reject(Error('Demo server startup timeout')),15000).unref())
    ]);
    const url = line.match(/http:\/\/127\.0\.0\.1:\d+/)?.[0];
    if (!url) throw Error('Missing demo server address');
    browser = await chromium.launch({executablePath:process.env.SCHEMA_CHROME_PATH || undefined,
        args:['--no-sandbox','--disable-dev-shm-usage',...JSON.parse(process.env.SCHEMA_CHROME_ARGS || '[]')]});
    await testPipeFlow(browser,url);
    console.log('Chromium: pipe colors and widths, both flow directions, stop/reconnect, equipment search, pixel sizes, mouse and touch routing OK');
} finally {
    await browser?.close();
    if (server.exitCode === null) {
        const stopped = once(server, 'exit');
        server.kill();
        await stopped;
    }
    await fs.rm(directory,{recursive:true,force:true});
}
