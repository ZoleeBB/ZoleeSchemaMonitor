#include "app_network.h"
#include "app_store.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_random.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "mdns.h"
#include "nvs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SemaphoreHandle_t s_lock;
static esp_netif_t *s_sta, *s_ap;
static app_network_status_t s_status;
static int64_t s_restart;
static bool s_configured;

static void event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START && s_configured)
        esp_wifi_connect();
    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
    {
        const ip_event_got_ip_t *info = data;
        s_status.connected = true;
        snprintf(s_status.ip, sizeof(s_status.ip), IPSTR, IP2STR(&info->ip_info.ip));
    }
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED)
    {
        s_status.connected = false;
        s_status.ip[0] = 0;
    }
    xSemaphoreGive(s_lock);
}

static esp_err_t recovery_start(void)
{
    /* A helyreállító hálózat nem nyitott. A véletlen kulcs a kijelzőről olvasható. */
    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP));
    wifi_config_t *config = calloc(1, sizeof(*config));
    if (!config)
        return ESP_ERR_NO_MEM;
    char password[17];
    nvs_handle_t nvs;
    esp_err_t result = nvs_open("schema", NVS_READWRITE, &nvs);
    if (result != ESP_OK)
    {
        free(config);
        return result;
    }
    size_t length = sizeof(password);
    if (nvs_get_str(nvs, "recovery_key", password, &length) != ESP_OK)
    {
        uint8_t random[16];
        esp_fill_random(random, sizeof(random));
        const char alphabet[] = "abcdefghjkmnpqrstuvwxyz23456789";
        for (unsigned i = 0; i < 16; i++)
            password[i] = alphabet[random[i] % (sizeof(alphabet) - 1)];
        password[16] = 0;
        result = nvs_set_str(nvs, "recovery_key", password);
        if (result == ESP_OK)
            result = nvs_commit(nvs);
    }
    nvs_close(nvs);
    if (result != ESP_OK)
    {
        free(config);
        return result;
    }
    snprintf((char *)config->ap.ssid,
             sizeof(config->ap.ssid),
             "ZoleeSchema-%02X%02X%02X",
             mac[3],
             mac[4],
             mac[5]);
    snprintf((char *)config->ap.password, sizeof(config->ap.password), "%s", password);
    config->ap.ssid_len = strlen((char *)config->ap.ssid);
    config->ap.channel = 1;
    config->ap.max_connection = 2;
    config->ap.authmode = WIFI_AUTH_WPA2_PSK;
    config->ap.pmf_cfg.capable = true;
    if (!s_ap)
        s_ap = esp_netif_create_default_wifi_ap();
    if (!s_ap)
    {
        free(config);
        return ESP_FAIL;
    }
    result = esp_wifi_set_mode(WIFI_MODE_APSTA);
    if (result == ESP_OK)
        result = esp_wifi_set_config(WIFI_IF_AP, config);
    if (result == ESP_OK)
    {
        xSemaphoreTake(s_lock, portMAX_DELAY);
        s_status.recovery = true;
        snprintf(s_status.recovery_ssid, sizeof(s_status.recovery_ssid), "%s", config->ap.ssid);
        snprintf(s_status.recovery_password, sizeof(s_status.recovery_password), "%s", password);
        xSemaphoreGive(s_lock);
    }
    free(config);
    return result;
}

static void worker(void *arg)
{
    (void)arg;
    unsigned attempts = 0;
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        xSemaphoreTake(s_lock, portMAX_DELAY);
        bool connected = s_status.connected, recovery = s_status.recovery;
        bool restart = s_restart && esp_timer_get_time() >= s_restart;
        xSemaphoreGive(s_lock);
        if (restart)
            esp_restart();
        if (!connected)
        {
            attempts++;
            if (s_configured && attempts % 15 == 0)
                (void)esp_wifi_connect();
            if (!recovery && (!s_configured || attempts >= 45))
                (void)recovery_start();
        }
        else
            attempts = 0;
    }
}

esp_err_t app_network_start(void)
{
    s_lock = xSemaphoreCreateMutex();
    if (!s_lock)
        return ESP_ERR_NO_MEM;
    esp_err_t result = esp_netif_init();
    if (result != ESP_OK)
        return result;
    result = esp_event_loop_create_default();
    if (result != ESP_OK)
        return result;
    s_sta = esp_netif_create_default_wifi_sta();
    if (!s_sta)
        return ESP_FAIL;
    esp_netif_set_hostname(s_sta, "zolee-schema-monitor");
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    result = esp_wifi_init(&init);
    if (result != ESP_OK)
        return result;
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event, NULL));
    cJSON *settings = app_store_settings(false);
    const cJSON *wifi = cJSON_GetObjectItemCaseSensitive(settings, "wifi");
    wifi_config_t *config = calloc(1, sizeof(*config));
    if (!config)
    {
        cJSON_Delete(settings);
        return ESP_ERR_NO_MEM;
    }
    size_t ssid_length = strlen(sm_json_string(wifi, "ssid")),
           password_length = strlen(sm_json_string(wifi, "password"));
    memcpy(config->sta.ssid, sm_json_string(wifi, "ssid"), ssid_length);
    memcpy(config->sta.password, sm_json_string(wifi, "password"), password_length);
    config->sta.threshold.authmode = password_length ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
    config->sta.pmf_cfg.capable = true;
    s_configured = ssid_length > 0;
    snprintf(s_status.ssid, sizeof(s_status.ssid), "%s", sm_json_string(wifi, "ssid"));
    cJSON_Delete(settings);
    result = esp_wifi_set_mode(WIFI_MODE_STA);
    if (result == ESP_OK && s_configured)
        result = esp_wifi_set_config(WIFI_IF_STA, config);
    free(config);
    if (result == ESP_OK)
        result = esp_wifi_start();
    if (result != ESP_OK)
        return result;
    (void)esp_wifi_set_ps(WIFI_PS_NONE);
    if (mdns_init() == ESP_OK)
    {
        mdns_hostname_set("zolee-schema-monitor");
        mdns_instance_name_set("ZoleeSchemaMonitor");
        mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
    }
    return xTaskCreate(worker, "schema_network", 4096, NULL, 4, NULL) == pdPASS ? ESP_OK
                                                                                : ESP_ERR_NO_MEM;
}

void app_network_status(app_network_status_t *status)
{
    memset(status, 0, sizeof(*status));
    if (!s_lock)
        return;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    *status = s_status;
    xSemaphoreGive(s_lock);
    if (status->connected)
    {
        wifi_ap_record_t access_point;
        if (esp_wifi_sta_get_ap_info(&access_point) == ESP_OK)
            status->rssi = access_point.rssi;
    }
}

void app_network_restart_later(void)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_restart = esp_timer_get_time() + 3000000;
    xSemaphoreGive(s_lock);
}
