/* A böngészős és natív validátor közös, hibás bemenetekkel is ellenőrizhető. */
#include "schema_model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char **argv)
{
    char *data = malloc(SM_PROJECT_BYTES + 32769);
    if (!data)
        return 2;
    size_t size = fread(data, 1, SM_PROJECT_BYTES + 32769, stdin);
    cJSON *json = sm_json_parse(data, size);
    if (json && cJSON_IsArray(json) && argc > 1 && !strcmp(argv[1], "format"))
    {
        cJSON *output = cJSON_CreateArray(), *entry;
        cJSON_ArrayForEach(entry, json)
        {
            cJSON *value = cJSON_GetArrayItem(entry, 0);
            sm_signal_t signal = {true, cJSON_IsNumber(value), value->valuedouble};
            char text[160];
            bool valid = sm_value_text(signal, cJSON_GetArrayItem(entry, 1)->valueint,
                                      cJSON_GetArrayItem(entry, 2)->valuestring, text, sizeof(text));
            cJSON_AddItemToArray(output, valid ? cJSON_CreateString(text) : cJSON_CreateNull());
        }
        char *encoded = cJSON_PrintUnformatted(output);
        puts(encoded);
        free(encoded);
        cJSON_Delete(output);
        cJSON_Delete(json);
        free(data);
        return 0;
    }
    if (json && cJSON_IsArray(json) && argc > 1 && !strcmp(argv[1], "spray"))
    {
        cJSON *output = cJSON_CreateArray(), *entry;
        cJSON_ArrayForEach(entry, json)
        {
            double x, y;
            bool valid = sm_spray_point(cJSON_GetArrayItem(entry, 0)->valuedouble,
                                       cJSON_GetArrayItem(entry, 1)->valueint,
                                       cJSON_GetArrayItem(entry, 2)->valuedouble, &x, &y);
            cJSON *point = cJSON_CreateArray();
            cJSON_AddItemToArray(point, cJSON_CreateBool(valid));
            if (valid)
            {
                cJSON_AddItemToArray(point, cJSON_CreateNumber(x));
                cJSON_AddItemToArray(point, cJSON_CreateNumber(y));
            }
            cJSON_AddItemToArray(output, point);
        }
        char *encoded = cJSON_PrintUnformatted(output);
        puts(encoded);
        free(encoded);
        cJSON_Delete(output);
        cJSON_Delete(json);
        free(data);
        return 0;
    }
    char error[SM_ERROR_SIZE] = "json";
    bool valid =
        json && (argc > 1 && !strcmp(argv[1], "settings") ? sm_settings_validate(json, error)
                                                          : sm_project_validate(json, error));
    cJSON_Delete(json);
    free(data);
    puts(valid ? "OK" : error);
    return valid ? 0 : 1;
}
