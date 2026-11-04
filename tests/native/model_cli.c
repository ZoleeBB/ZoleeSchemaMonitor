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
    char error[SM_ERROR_SIZE] = "json";
    bool valid =
        json && (argc > 1 && !strcmp(argv[1], "settings") ? sm_settings_validate(json, error)
                                                          : sm_project_validate(json, error));
    cJSON_Delete(json);
    free(data);
    puts(valid ? "OK" : error);
    return valid ? 0 : 1;
}
