/* A sémaexport marad titokmentes; az eszközmentés kriptográfiáját a készülék
 * végzi, így a helyi HTTP-felületen sem kell Web Crypto secure context. */
const words = {
    hu: {
        title: 'Mentés és visszaállítás', schema: 'Sémaexport',
        schemaHelp: 'Az összes oldal és Loxone-párosítás exportálható, jelszó és token nélkül.',
        schemaExport: 'Séma letöltése', schemaImport: 'Séma importálása a szerkesztőben',
        device: 'Titkosított eszközmentés',
        deviceHelp: 'Az oldalakat és a beállításokat – a Wi-Fi és a Loxone jelszavával együtt – külön mentési jelszó védi. A fájl másik kijelzőn is visszaállítható. Grafika, firmware, NVS-kulcs és munkamenettoken nincs benne.',
        pass: 'Mentési jelszó', repeat: 'Mentési jelszó ismét', download: 'Titkosított mentés letöltése',
        passHelp: 'Legalább 12 karakter, legfeljebb 128 UTF-8 bájt. A mentési jelszó nélkül a fájl nem állítható vissza.',
        restore: 'Eszköz visszaállítása', file: 'Titkosított mentés fájlja', restoreButton: 'Visszaállítás és újraindítás',
        restoreHelp: 'Az összes jelenlegi oldalt és beállítást lecseréli. Sikeres tartós mentés után a kijelző újraindul; a visszaállított hálózati beállítások lépnek életbe.',
        waiting: 'Ellenőrzés és titkosítás folyamatban. Ez eltarthat néhány másodpercig.',
        restored: 'Visszaállítva. A kijelző újraindul; az új hálózati beállítások lépnek életbe.',
        exported: 'A titkosított mentés letöltése elindult.', disabled: 'Az eszközmentés működő SD-állapottárral, a készüléken használható.',
        mismatch: 'A két mentési jelszó nem egyezik.', password: 'Legalább 12 karakteres, legfeljebb 128 UTF-8 bájtos mentési jelszó szükséges.',
        invalid: 'Hibás mentési jelszó, sérült vagy nem támogatott mentés.', size: 'A mentési fájl túl nagy.',
        busy: 'Frissítés vagy újraindítás van folyamatban. Próbáld később.',
        storage: 'Az SD-állapottár nem írható. A korábbi állapot megmaradt.',
        exportError: 'A mentés nem készíthető el. Nincs elegendő erőforrás, vagy a titkosítás nem sikerült.',
        incompatible: 'A mentés nem állítható vissza ezzel a firmware-rel. A korábbi állapot megmaradt.'
    },
    en: {
        title: 'Backup and restore', schema: 'Schema export',
        schemaHelp: 'Export all pages and Loxone bindings without passwords or tokens.',
        schemaExport: 'Download schema', schemaImport: 'Import schema in the editor',
        device: 'Encrypted device backup',
        deviceHelp: 'Pages and settings, including Wi-Fi and Loxone passwords, are protected by a separate backup password. The file can be restored to another display. Graphics, firmware, NVS keys and session tokens are excluded.',
        pass: 'Backup password', repeat: 'Repeat backup password', download: 'Download encrypted backup',
        passHelp: 'At least 12 characters, at most 128 UTF-8 bytes. The file cannot be recovered without its backup password.',
        restore: 'Restore device', file: 'Encrypted backup file', restoreButton: 'Restore and restart',
        restoreHelp: 'Replaces all current pages and settings. The display restarts after durable storage succeeds and applies the restored network settings.',
        waiting: 'Validation and encryption in progress. This may take a few seconds.',
        restored: 'Restored. The display is restarting with the restored network settings.',
        exported: 'Encrypted backup download started.', disabled: 'Device backup requires a display with working SD state storage.',
        mismatch: 'The backup passwords do not match.', password: 'Use at least 12 characters and at most 128 UTF-8 bytes.',
        invalid: 'Incorrect backup password, damaged or unsupported backup.', size: 'The backup file is too large.',
        busy: 'An update or restart is pending. Try again later.',
        storage: 'SD state storage is not writable. The previous state was retained.',
        exportError: 'Backup could not be created. Resources are insufficient or encryption failed.',
        incompatible: 'This firmware cannot restore the backup. The previous state was retained.'
    }
};
export const MAX_BACKUP_FILE = 2 * (1048576 + 32768) + 4096;
export function validBackupPassword(value) {
    return typeof value === 'string' && !value.includes('\0') && Array.from(value).length >= 12 && new TextEncoder().encode(value).length <= 128;
}
export class BackupPanel {
    constructor(root, {api, language, feedback}) {
        this.root = root; this.api = api; this.text = words[language]; this.feedback = feedback;
        this.closed = false; this.busy = false; this.available = false;
        const t = this.text;
        root.innerHTML = `<h1>${t.title}</h1><section><h2>${t.schema}</h2><p>${t.schemaHelp}</p>
            <a class="button" href="/api/project/export" download="ZoleeSchemaMonitor.json">${t.schemaExport}</a>
            <p><a href="/editor">${t.schemaImport}</a></p></section>
            <section><h2>${t.device}</h2><p>${t.deviceHelp}</p><p class="hint">${t.passHelp}</p>
            <p id="backup-availability" role="status"></p>
            <form id="device-backup-form"><div class="grid">
                <label>${t.pass}<input name="passphrase" type="password" minlength="12" maxlength="128" autocomplete="new-password" required></label>
                <label>${t.repeat}<input name="repeat" type="password" minlength="12" maxlength="128" autocomplete="new-password" required></label>
            </div><button type="submit" disabled>${t.download}</button></form></section>
            <section><h2>${t.restore}</h2><p>${t.restoreHelp}</p>
            <form id="device-restore-form"><div class="grid">
                <label>${t.file}<input name="file" type="file" accept=".json" required></label>
                <label>${t.pass}<input name="passphrase" type="password" minlength="12" maxlength="128" autocomplete="off" required></label>
            </div><button type="submit" disabled>${t.restoreButton}</button></form></section>`;
        this.exportForm = root.querySelector('#device-backup-form');
        this.restoreForm = root.querySelector('#device-restore-form');
        this.exportForm.onsubmit = event => { event.preventDefault(); void this.export(); };
        this.restoreForm.onsubmit = event => { event.preventDefault(); void this.restore(); };
    }
    async init() {
        try {
            const status = await this.api('/api/backup/status');
            if (this.closed) return;
            this.available = status.available === true;
            this.revision = status.revision;
            this.root.querySelector('#backup-availability').textContent = this.available ? '' : this.text.disabled;
            this.controls();
        } catch (error) { if (!this.closed) this.feedback(error.message, true); }
    }
    controls() {
        for (const form of [this.exportForm, this.restoreForm]) {
            for (const control of form.elements) control.disabled = this.busy || !this.available;
        }
    }
    error(error) {
        const code = error.message;
        if (code === 'backup.passphrase') return this.text.password;
        if (['backup.invalid-or-password', 'backup.fields', 'backup.format'].includes(code)) return this.text.invalid;
        if (code === 'backup.size') return this.text.size;
        if (code === 'backup.busy') return this.text.busy;
        if (['storage.write', 'backup.storage'].includes(code)) return this.text.storage;
        if (code === 'backup.export') return this.text.exportError;
        if (/^(project|settings|binding|element|page)\./.test(code)) return this.text.incompatible;
        return code;
    }
    async export() {
        if (this.busy || !this.available || this.closed) return;
        const passphrase = this.exportForm.elements.passphrase.value;
        if (!validBackupPassword(passphrase)) { this.feedback(this.text.password, true); return; }
        if (passphrase !== this.exportForm.elements.repeat.value) { this.feedback(this.text.mismatch, true); return; }
        this.busy = true; this.controls(); this.feedback(this.text.waiting);
        try {
            const envelope = await this.api('/api/backup/export', {passphrase}, 'POST', 120000);
            if (this.closed) return;
            const url = URL.createObjectURL(new Blob([JSON.stringify(envelope, null, 2) + '\n'], {type: 'application/json'}));
            const link = document.createElement('a'); link.href = url; link.download = 'ZoleeSchemaMonitor-backup.json';
            document.body.append(link); link.click(); link.remove();
            setTimeout(() => URL.revokeObjectURL(url), 5000);
            this.feedback(this.text.exported);
        } catch (error) { if (!this.closed) this.feedback(this.error(error), true); }
        finally {
            this.exportForm.elements.passphrase.value = ''; this.exportForm.elements.repeat.value = '';
            this.busy = false; this.controls();
        }
    }
    async restore() {
        if (this.busy || !this.available || this.closed) return;
        const passphrase = this.restoreForm.elements.passphrase.value, file = this.restoreForm.elements.file.files[0];
        if (!file) return;
        if (!validBackupPassword(passphrase)) { this.feedback(this.text.password, true); return; }
        if (file.size > MAX_BACKUP_FILE) { this.feedback(this.text.size, true); return; }
        this.busy = true; this.controls(); this.feedback(this.text.waiting);
        try {
            const backup = JSON.parse(await file.text());
            if (backup?.format !== 'zolee-backup' || backup.version !== 1) throw Error('backup.format');
            const result = await this.api('/api/backup/restore', {passphrase, backup, revision: this.revision}, 'POST', 120000);
            if (this.closed) return;
            this.revision = result.revision; this.available = false;
            this.feedback(this.text.restored);
        } catch (error) { if (!this.closed) this.feedback(error instanceof SyntaxError ? this.text.invalid : this.error(error), true); }
        finally { this.restoreForm.elements.passphrase.value = ''; this.busy = false; this.controls(); }
    }
    destroy() { this.closed = true; }
}
