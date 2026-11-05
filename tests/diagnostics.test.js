import test from 'node:test';
import assert from 'node:assert/strict';
import {usedPercent, uptime} from '../web/diagnostics.js';
test('Diagnosztika: ismeretlen és hibás kapacitás nem lehet 0%-os sáv', () => {
    for (const pair of [[null, 0], [0, 0], [100, null], [100, 101], [100, -1], [NaN, 0]]) {
        assert.equal(usedPercent(...pair), null);
    }
    assert.equal(usedPercent(100, 100), 0);
    assert.equal(usedPercent(100, 0), 100);
    assert.equal(usedPercent(34359738368, 17179869184), 50);
});
test('Diagnosztika: üzemidő napforduló után is monoton óraszám', () => {
    assert.equal(uptime(null), '—');
    assert.equal(uptime(-1), '—');
    assert.equal(uptime(0), '0:00:00');
    assert.equal(uptime(90061), '25:01:01');
});
