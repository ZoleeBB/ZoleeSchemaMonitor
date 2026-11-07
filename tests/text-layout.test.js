import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {glyphWidth, labelLines} from '../web/text-layout.js';

test('A szerkesztő mind a 200 natív karakter lépésközét követi', () => {
    const font = fs.readFileSync('main/fonts/app_font_20.c', 'utf8');
    const metrics = [...font.matchAll(/\.adv_w=(\d+),[^\n]*U\+([A-F0-9]+)/g)];
    assert.equal(metrics.length, 200);
    for (const [,advance,code] of metrics)
        assert.equal(glyphWidth(String.fromCodePoint(parseInt(code, 16))), (Number(advance) + 8) >> 4);
});

test('LVGL 9.5 tördelési minták magyar szavakkal és különböző nagyítással', () => {
    const text = 'Fűtési kör előremenő hőmérséklete';
    const expected = ['Fűtési ', 'kör ', 'előrem', 'enő ', 'hőmérs', 'éklete'];
    for (const size of [10,20,40,60])
        assert.deepEqual(labelLines(text, 72 * size/20, size).map(line => line.text), expected);
    assert.deepEqual(labelLines(text, 112, 20).map(line => line.width), [96,106,110,18]);
    assert.deepEqual(labelLines('Árvíz\r\n\nŰr', 200, 20).map(line => line.text), ['Árvíz','','Űr']);
    assert.deepEqual(labelLines('őű', 1, 64).map(line => line.text), ['ő','ű']);
});
