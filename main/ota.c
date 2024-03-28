#include "ota.h"

#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "esp_flash_partitions.h"

#define CONFIG_SYS_LOG_LEVEL SYS_LOG_LEVEL_WRN
#define SYS_LOG_DOMAIN "OTA"
#define CONS_ABORT()
#include "sys_log.h"

#define HASH_LEN 32 /* SHA-256 digest length */
static void _print_sha256(const uint8_t *image_hash, const char *label)
{
    char hash_print[HASH_LEN * 2 + 1];
    hash_print[HASH_LEN * 2] = 0;
    for (int i = 0; i < HASH_LEN; ++i)
    {
        sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
    }
    SYS_LOG_INF("%s %s", label, hash_print);
}

void ota_partition_check()
{
    uint8_t sha_256[HASH_LEN] = {0};
    esp_partition_t partition;

    // get sha256 digest for the partition table
    partition.address = ESP_PARTITION_TABLE_OFFSET;
    partition.size = ESP_PARTITION_TABLE_MAX_LEN;
    partition.type = ESP_PARTITION_TYPE_DATA;
    esp_partition_get_sha256(&partition, sha_256);
    _print_sha256(sha_256, "SHA-256 for the partition table:");

    // get sha256 digest for bootloader
    partition.address = ESP_BOOTLOADER_OFFSET;
    partition.size = ESP_PARTITION_TABLE_OFFSET;
    partition.type = ESP_PARTITION_TYPE_APP;
    esp_partition_get_sha256(&partition, sha_256);
    _print_sha256(sha_256, "SHA-256 for bootloader:         ");

    // get sha256 digest for running partition
    esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
    _print_sha256(sha_256, "SHA-256 for current firmware:   ");

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK)
    {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY)
        {
            // run diagnostic function ...
            bool diagnostic_is_ok = true; // diagnostic();
            if (diagnostic_is_ok)
            {
                SYS_LOG_INF("Diagnostics completed successfully! Continuing execution ...");
                esp_ota_mark_app_valid_cancel_rollback();
            }
            else
            {
                SYS_LOG_ERR("Diagnostics failed! Start rollback to the previous version ...");
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        }
        else
        {
            SYS_LOG_INF("");
        }
    }
    else
    {
        SYS_LOG_INF("");
    }
}
