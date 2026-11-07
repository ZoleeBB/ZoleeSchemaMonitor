#include "app_store.h"
#include "app_backup.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char **argv)
{
    assert(argc > 1);
    esp_err_t init = app_store_init();
    cJSON *result = cJSON_CreateObject();
    cJSON_AddNumberToObject(result, "init", init);
    if (!strcmp(argv[1], "get"))
    {
        uint32_t revision;
        cJSON_AddItemToObject(result, "project", app_store_project(&revision));
        cJSON_AddNumberToObject(result, "revision", revision);
        cJSON_AddBoolToObject(result, "recovered", app_store_recovered());
        cJSON_AddItemToObject(result, "settings", app_store_settings(true));
    }
    else if (!strcmp(argv[1], "page"))
    {
        uint32_t revision;
        int count;
        cJSON_AddItemToObject(result, "page", app_store_page(atoi(argv[2]), &count, &revision));
        cJSON_AddNumberToObject(result, "count", count);
        cJSON_AddNumberToObject(result, "revision", revision);
    }
    else if (!strcmp(argv[1], "snapshot"))
        cJSON_AddItemToObject(result, "state", app_store_snapshot());
    else
    {
        char *data = malloc(SM_PROJECT_BYTES + 512);
        size_t length = fread(data, 1, SM_PROJECT_BYTES + 512, stdin);
        cJSON *json = sm_json_parse(data, length);
        free(data);
        char error[SM_ERROR_SIZE] = {0};
        esp_err_t status;
        if (!strcmp(argv[1], "project"))
        {
            uint32_t revision = 0;
            status = app_store_save_project(
                json, argc > 2 ? strtoul(argv[2], NULL, 10) : 0, &revision, error);
            cJSON_AddNumberToObject(result, "revision", revision);
        }
        else if ((!strcmp(argv[1], "restore") || !strcmp(argv[1], "restore-fail")))
        {
            uint32_t revision = 0;
            bool fail = !strcmp(argv[1], "restore-fail");
            if (fail)
                assert(rename("state", "unmounted") == 0);
            status = app_store_restore(
                json, argc > 2 ? strtoul(argv[2], NULL, 10) : 0, &revision, error);
            if (fail)
                assert(rename("unmounted", "state") == 0);
            cJSON_AddItemToObject(result, "after", app_store_snapshot());
            cJSON_AddNumberToObject(result, "revision", revision);
        }
        else
            status = app_store_save_settings(json, !strcmp(argv[1], "loxone"), error);
        cJSON_Delete(json);
        cJSON_AddNumberToObject(result, "status", status);
        cJSON_AddStringToObject(result, "error", error);
    }
    char *output = cJSON_PrintUnformatted(result);
    assert(output);
    puts(output);
    free(output);
    cJSON_Delete(result);
}
