/* A helyi fejlesztői szerver ugyanazzal a modellel ellenőriz, mint a böngésző. */
import fs from 'node:fs';
import {validateProject} from '../web/model.js';

try {
    const catalog = JSON.parse(fs.readFileSync(new URL('../web/catalog.json', import.meta.url)));
    const project = validateProject(JSON.parse(fs.readFileSync(0, 'utf8')), catalog);
    process.stdout.write(JSON.stringify(project));
} catch (error) {
    process.stderr.write(error.message);
    process.exitCode = 1;
}
