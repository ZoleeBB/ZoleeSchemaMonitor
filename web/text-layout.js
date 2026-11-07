// Az app_font_20 egész pixeles karakterlépései. A teszt a C-forrással ellenőrzi.
const ascii = [
    5, 5, 8, 13, 11, 17, 15, 5, 6, 6, 11, 11, 5, 6, 5, 7,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 5, 5, 11, 11, 11, 9,
    18, 13, 13, 13, 15, 11, 10, 15, 15, 7, 5, 12, 11, 18, 15, 16,
    12, 16, 12, 11, 11, 15, 12, 19, 12, 11, 11, 7, 7, 7, 11, 9,
    6, 11, 12, 10, 12, 11, 7, 12, 12, 5, 5, 11, 5, 19, 12, 12,
    12, 12, 8, 10, 7, 12, 10, 16, 11, 10, 9, 8, 11, 8, 11,
];
const latin = [
    5, 5, 11, 11, 11, 11, 11, 10, 12, 17, 7, 10, 11, 0, 17, 10,
    9, 11, 7, 7, 6, 12, 13, 5, 5, 7, 8, 10, 15, 15, 16, 9,
    13, 13, 13, 13, 13, 13, 18, 13, 11, 11, 11, 11, 7, 7, 7, 7,
    15, 15, 16, 16, 16, 16, 16, 11, 16, 15, 15, 15, 15, 11, 12, 13,
    11, 11, 11, 11, 11, 11, 17, 10, 11, 11, 11, 11, 5, 5, 5, 5,
    12, 12, 12, 12, 12, 12, 12, 11, 12, 12, 12, 12, 12, 10, 12, 10,
];
const extra = new Map([['Ő', 16], ['ő', 12], ['Ű', 15], ['ű', 12], ['–', 10], ['—', 20], ['€', 11], ['↑', 17], ['↓', 17]]);

export function glyphWidth(character) {
    const code = character.codePointAt(0);
    if (code >= 32 && code < 127) return ascii[code - 32];
    if (code >= 160 && code < 256) return latin[code - 160];
    if (/[\u0000-\u001f\u061c\u115f\u1160\u180b-\u180e\u200b-\u200f\u2028-\u202f\u205f-\u206f\ufeff\uf8ff]/u.test(character)) return 0;
    return extra.get(character) ?? 16;
}

// A címke 20 px-es koordinátáiban tördelünk, a transzformációt ezután kapja.
export function labelLines(text, fieldWidth, fontSize) {
    const available = Math.max(1, Math.floor(Math.floor(fieldWidth) * 20 / fontSize));
    const lines = [];
    for (const paragraph of text.split(/\r\n|[\r\n]/u)) {
        const chars = Array.from(paragraph);
        let start = 0;
        if (!chars.length) lines.push({text: '', width: 0});
        while (start < chars.length) {
            let end = start, used = 0, boundary = start;
            while (end < chars.length && used + glyphWidth(chars[end]) <= available) {
                if (' ,.;:-_)]}'.includes(chars[end])) boundary = end + 1;
                used += glyphWidth(chars[end++]);
            }
            if (end < chars.length && boundary > start) end = boundary;
            end = Math.max(start + 1, end);
            const part = chars.slice(start, end);
            lines.push({text: part.join(''), width: part.reduce((sum, c) => sum + glyphWidth(c), 0)});
            start = end;
            while (chars[start] === ' ') start++;
        }
    }
    return lines;
}
