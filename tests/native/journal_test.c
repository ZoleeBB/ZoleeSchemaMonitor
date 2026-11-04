#define _POSIX_C_SOURCE 200809L
#include "schema_model.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

static bool s_fail_rename;
int __real_rename(const char *from, const char *to);
int __wrap_rename(const char *from, const char *to)
{
    if (s_fail_rename)
    {
        errno = EIO;
        return -1;
    }
    /* Az éles FatFs viselkedése: létező célra nincs rename. */
    if (access(to, F_OK) == 0)
    {
        errno = EEXIST;
        return -1;
    }
    return __real_rename(from, to);
}

static void damage(const char *directory, unsigned slot, long offset)
{
    char path[256];
    snprintf(path, sizeof(path), "%s/state-%u.bin", directory, slot);
    FILE *file = fopen(path, "r+b");
    assert(file && !fseek(file, offset, SEEK_SET));
    int byte = fgetc(file);
    assert(byte >= 0 && !fseek(file, offset, SEEK_SET));
    assert(fputc(byte ^ 2, file) >= 0);
    assert(!fclose(file));
}

int main(void)
{
    char directory[] = "/tmp/schema-journal-XXXXXX";
    assert(mkdtemp(directory));
    uint32_t generation = 0;
    assert(!sm_journal_read_slot(directory, 0, &generation));
    assert(sm_journal_write(directory, "{\"title\":\"régi\"}", 1));
    assert(sm_journal_write(directory, "{\"title\":\"új\"}", 2));
    char *json = sm_journal_read_slot(directory, 0, &generation);
    assert(json && generation == 2 && strstr(json, "új"));
    free(json);
    /* A fejléc és a törzs sérülése sem nyomhatja el az előző példányt. */
    damage(directory, 0, 8);
    assert(!sm_journal_read_slot(directory, 0, &generation));
    json = sm_journal_read_slot(directory, 1, &generation);
    assert(json && generation == 1 && strstr(json, "régi"));
    free(json);
    assert(sm_journal_write(directory, "{\"title\":\"új\"}", 2));
    damage(directory, 0, 25);
    assert(!sm_journal_read_slot(directory, 0, &generation));
    assert(sm_journal_write(directory, "{}", 2));
    s_fail_rename = true;
    assert(!sm_journal_write(directory, "{\"interrupted\":true}", 3));
    json = sm_journal_read_slot(directory, 0, &generation);
    assert(json && generation == 2);
    free(json);
    s_fail_rename = false;
    assert(sm_journal_write(directory, "{\"title\":\"régi\"}", 3));
    char path[256];
    snprintf(path, sizeof(path), "%s/state-0.bin", directory);
    assert(!truncate(path, 25));
    assert(!sm_journal_read_slot(directory, 0, &generation));
    /* Félbemaradt .part nem lesz automatikusan aktív. */
    snprintf(path, sizeof(path), "%s/state-0.bin.part", directory);
    FILE *partial = fopen(path, "wb");
    assert(partial);
    fputs("partial", partial);
    fclose(partial);
    json = sm_journal_read_slot(directory, 1, &generation);
    assert(json && generation == 3);
    free(json);
    unlink(path);
    for (unsigned i = 0; i < 2; i++)
    {
        snprintf(path, sizeof(path), "%s/state-%u.bin", directory, i);
        unlink(path);
    }
    assert(!rmdir(directory));
    assert(!sm_journal_write("/does-not-exist", "{}", 3));
    puts("journal OK");
}
