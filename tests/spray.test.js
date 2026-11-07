import test from 'node:test';
import assert from 'node:assert/strict';
import {sprayPoint, SPRAY_DROPS} from '../web/model.js';

test('spray paths repeat, fan out and stay finite at extreme phase/direction values', () => {
    for (let drop = 0; drop < SPRAY_DROPS; drop++) {
        for (const phase of [0, .1, .75, 1, -1.25, 1e308]) {
            for (const direction of [-150, -90, 0, 270, 1e308]) {
                const p = sprayPoint(phase, drop, direction);
                assert.ok(Number.isFinite(p.x) && Number.isFinite(p.y));
                assert.ok(Math.hypot(p.x, p.y) <= 1.3);
            }
        }
        const a = sprayPoint(.2, drop), b = sprayPoint(1.2, drop, 270);
        assert.ok(Math.hypot(a.x-b.x, a.y-b.y) < 1e-12);
    }
    assert.ok(sprayPoint(.1, 0).x < 0 && sprayPoint(.1, 3).x > 0);
    for (const input of [[NaN,0,-90],[Infinity,0,-90],[0,-1,-90],[0,4,-90],[0,.5,-90],[0,0,Infinity]]) {
        assert.throws(() => sprayPoint(...input));
    }
});
