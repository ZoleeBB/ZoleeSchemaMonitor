#include "schema_model.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static uint32_t crc32(uint32_t crc, const unsigned char *data, size_t length)
{
    while (length--)
    {
        crc ^= *data++;
        for (unsigned bit = 0; bit < 8; bit++)
            crc = (crc >> 1) ^ (0xedb88320U & (0U - (crc & 1U)));
    }
    return crc;
}

static uint32_t read32(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void write32(unsigned char *p, uint32_t value)
{
    for (unsigned i = 0; i < 4; i++)
        p[i] = value >> (i * 8);
}

static bool transfer(int file, void *buffer, size_t length, bool writing)
{
    unsigned char *p = buffer;
    while (length)
    {
        ssize_t count = writing ? write(file, p, length) : read(file, p, length);
        if (count < 0 && errno == EINTR)
            continue;
        if (count <= 0)
            return false;
        p += count;
        length -= count;
    }
    return true;
}

char *sm_journal_read_slot(const char *directory, unsigned slot, uint32_t *generation)
{
    char path[256];
    if (slot > 1 ||
        snprintf(path, sizeof(path), "%s/state-%u.bin", directory, slot) >= (int)sizeof(path))
        return NULL;
    int file = open(path, O_RDONLY);
    if (file < 0)
        return NULL;
    unsigned char header[24];
    bool ok = transfer(file, header, sizeof(header), false) && !memcmp(header, "ZSMST01\n", 8);
    uint32_t length = ok ? read32(header + 12) : 0;
    struct stat info;
    ok = ok && length > 0 && length <= SM_PROJECT_BYTES + 32768 && fstat(file, &info) == 0 &&
         info.st_size == (off_t)(length + sizeof(header)) && read32(header + 20) == 0 &&
         read32(header + 8) > 0 && read32(header + 8) % 2 == slot;
    char *json = ok ? malloc(length + 1) : NULL;
    ok = ok && json && transfer(file, json, length, false) &&
         ~crc32(crc32(0xffffffffU, header + 8, 8), (unsigned char *)json, length) ==
             read32(header + 16);
    close(file);
    if (!ok)
    {
        free(json);
        return NULL;
    }
    json[length] = 0;
    *generation = read32(header + 8);
    return json;
}

bool sm_journal_write(const char *directory, const char *json, uint32_t generation)
{
    if (!json || !*json || !generation || strlen(json) > SM_PROJECT_BYTES + 32768)
        return false;
    char path[256], temporary[264];
    if (snprintf(path, sizeof(path), "%s/state-%u.bin", directory, (unsigned)(generation % 2)) >=
        (int)sizeof(path))
        return false;
    snprintf(temporary, sizeof(temporary), "%s.part", path);
    unsigned char header[24] = {0};
    size_t length = strlen(json);
    memcpy(header, "ZSMST01\n", 8);
    write32(header + 8, generation);
    write32(header + 12, length);
    write32(header + 16,
            ~crc32(crc32(0xffffffffU, header + 8, 8), (const unsigned char *)json, length));
    int file = open(temporary, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (file < 0)
        return false;
    bool ok = transfer(file, header, sizeof(header), true) &&
              transfer(file, (void *)json, length, true) && fsync(file) == 0;
    if (close(file) != 0)
        ok = false;
    /* A FatFs nem engedi létező cél átnevezéses felülírását. Csak a következő
     * generációhoz tartozó, inaktív példányt töröljük; az aktív másik slot él. */
    if (ok)
        ok = unlink(path) == 0 || errno == ENOENT;
    if (ok)
        ok = rename(temporary, path) == 0;
    if (!ok)
        unlink(temporary);
#ifndef ESP_PLATFORM
    if (ok)
    {
        int folder = open(directory, O_RDONLY);
        if (folder >= 0)
        {
            ok = fsync(folder) == 0;
            close(folder);
        }
        else
            ok = false;
    }
#endif
    return ok;
}
