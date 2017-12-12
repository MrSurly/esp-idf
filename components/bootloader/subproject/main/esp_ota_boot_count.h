

#ifndef _ESP_OTA_BOOT_COUNT_H
#define _ESP_OTA_BOOT_COUNT_H

#include "esp_err.h"
#include "esp_flash_data_types.h"
#include "rom/crc.h"
#include <stdbool.h>
#include <sys/param.h>

#ifdef __cplusplus
extern "C"
{
#endif

inline uint32_t ota_select_crc(const esp_ota_select_entry_t *s)
{
  return crc32_le(UINT32_MAX, (uint8_t*)&s->ota_seq, 4);
}

inline bool ota_select_valid(const esp_ota_select_entry_t *s)
{
  return s->ota_seq != UINT32_MAX && s->crc == ota_select_crc(s);
}

/**
 * @brief OTA boot count operation identifier
 *
 * @param OTA_BOOT_COUNT_ZERO Set the counter to zero
 * @param OTA_BOOT_COUNT_INCREMENT Increment the counter
 * @param OTA_BOOT_COUNT_QUERY Get current counter value
 * @param OTA_BOOT_COUNT_DISABLE Disable the counting mechanism
 */
typedef enum {
    OTA_BOOT_COUNT_ZERO,
    OTA_BOOT_COUNT_INCREMENT,
    OTA_BOOT_COUNT_QUERY,
    OTA_BOOT_COUNT_DISABLE,
} esp_ota_boot_count_op_t;


/**
 * @brief Perform various operations on OTA boot count
 *
 * @param op This determines which operation is performed:
 *      OTA_BOOT_COUNT_ZERO Set the counter to zero
 *      OTA_BOOT_COUNT_INCREMENT Increment the counter
 *      OTA_BOOT_COUNT_QUERY Get current counter value
 *      OTA_BOOT_COUNT_DISABLE Disable the counting mechanism
 *
 *  @param count_return  This is only used for the OTA_BOOT_COUNT_QUERY operation.  
 *  If non-NULL, then the count is returned via this pointer.
 *
 *  @return
 *      - ESP_OK: No error
 *      - ESP_FAIL: Operation failed
 *      - ESP_INVALID_SIZE: If the OTA partition is too small. It must be at least 4 sectors.
 */

esp_err_t esp_ota_boot_count_op(esp_ota_boot_count_op_t op, uint32_t* count_return);

esp_err_t rewrite_ota_seq(uint32_t seq, uint32_t max_boot_count, uint8_t sec_id, uint32_t ota_part_offset);

#ifdef BOOTLOADER_BUILD
esp_err_t esp_ota_boot_count_op_bootloader(uint32_t ota_part_offset, esp_ota_boot_count_op_t op, uint32_t* count_return);
#endif
    
#ifdef __cplusplus
}
#endif

#endif /* _ESP_OTA_BOOT_COUNT_H */
