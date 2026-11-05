#pragma once
#include "app_diagnostics.h"
#include "app_network.h"
/* Az app_main hívja, kijelzőzár nélkül. A privát AP-jelszó csak ide kerül. */
void app_diagnostic_view_update(const app_diagnostics_t *data, const app_network_status_t *network);
void app_diagnostic_view_poll(void);
