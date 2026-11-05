import test from 'node:test';
import assert from 'node:assert/strict';
import {validBackupPassword} from '../web/backup.js';

test('A mentési jelszó karakter- és UTF-8-bájtkorlátja egyezik a készülékkel', () => {
    assert.equal(validBackupPassword('12345678901'), false);
    assert.equal(validBackupPassword('123456789012'), true);
    assert.equal(validBackupPassword('árvíztűrő1234'), true);
    assert.equal(validBackupPassword('😀'.repeat(12)), true);
    assert.equal(validBackupPassword('😀'.repeat(32)), true);
    assert.equal(validBackupPassword('😀'.repeat(33)), false);
    assert.equal(validBackupPassword('a'.repeat(128)), true);
    assert.equal(validBackupPassword('a'.repeat(129)), false);
    assert.equal(validBackupPassword('123456789012\0'), false);
    assert.equal(validBackupPassword(null), false);
});
