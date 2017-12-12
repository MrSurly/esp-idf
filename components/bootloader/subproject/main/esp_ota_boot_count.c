#include "esp_ota_boot_count.h" 
#include "esp_flash_data_types.h"
#include "esp_flash_partitions.h"
#ifdef BOOTLOADER_BUILD
#include "bootloader_flash.h"
#include "bootloader_config.h"
#else 
#include "esp_spi_flash.h"
#include "esp_partition.h"
#endif
#include <string.h>



esp_err_t rewrite_ota_seq(uint32_t seq, uint32_t max_boot_count, uint8_t sec_id, uint32_t ota_part_offset) {
    esp_err_t ret;

    if (sec_id <= 3) {

        esp_ota_select_entry_t entry;
        memset(&entry, 0xff, sizeof(entry));
        entry.ota_seq = seq;
        entry.max_boot_count = max_boot_count;
        entry.crc = ota_select_crc(&entry);

#ifdef BOOTLOADER_BUILD
        ret = bootloader_flash_erase_sector(ota_part_offset / SPI_FLASH_SEC_SIZE + sec_id);
#else
        ret = spi_flash_erase_sector(ota_part_offset / SPI_FLASH_SEC_SIZE + sec_id);
#endif
        if (ret != ESP_OK) {
            return ret;
        } else {
#ifdef BOOTLOADER_BUILD
            return bootloader_flash_write(ota_part_offset + sec_id * SPI_FLASH_SEC_SIZE, &entry, sizeof(esp_ota_select_entry_t), false);
#else
            return spi_flash_write(ota_part_offset + sec_id * SPI_FLASH_SEC_SIZE, &entry, sizeof(esp_ota_select_entry_t));
#endif
        }
    } else {
        return ESP_ERR_INVALID_ARG;
    }
}


esp_err_t esp_ota_boot_count_op_bootloader(uint32_t ota_part_offset, esp_ota_boot_count_op_t op, uint32_t* count_return) {
    esp_err_t ret;
    esp_ota_select_entry_t count[2];


    switch(op) {
        case OTA_BOOT_COUNT_INCREMENT:
        case OTA_BOOT_COUNT_QUERY:
            {
#ifdef BOOTLOADER_BUILD
                const void *map = bootloader_mmap(ota_part_offset, SPI_FLASH_SEC_SIZE * 4);
                if (map == NULL) {
                    return ESP_FAIL;
                }
#else
                spi_flash_mmap_handle_t handle;
                const void *map;
                ret = spi_flash_mmap(ota_part_offset, SPI_FLASH_SEC_SIZE * 4, SPI_FLASH_MMAP_DATA, &map, &handle);
#endif

                memcpy(&count[0], map + SPI_FLASH_SEC_SIZE * 2, sizeof(esp_ota_select_entry_t));
                memcpy(&count[1], map + SPI_FLASH_SEC_SIZE * 3, sizeof(esp_ota_select_entry_t));
#ifdef BOOTLOADER_BUILD
                bootloader_munmap(map);
#else
                spi_flash_munmap(handle);
#endif
            }
            break;

        case OTA_BOOT_COUNT_ZERO:
        case OTA_BOOT_COUNT_DISABLE:
            // intentionally do nothing
            break;
    }

    bool valid0 = ota_select_valid(&count[0]);
    bool valid1 = ota_select_valid(&count[1]);

    switch(op) {
        case OTA_BOOT_COUNT_QUERY:
            if (count != NULL) {
                if (valid0 && valid1) {
                    *count_return = MAX(count[0].ota_seq, count[1].ota_seq);
                } else if (valid0) {
                    *count_return = count[0].ota_seq;
                } else if (valid1) {
                    *count_return = count[1].ota_seq;
                } else {
                    *count_return = -1;
                    return ESP_FAIL;
                }
                return ESP_OK;
            }
            break;

        case OTA_BOOT_COUNT_INCREMENT:

            if(valid0 && valid1) {
                if(count[0].ota_seq > count[1].ota_seq) { 
                    // Write count0.seq + 1 to count1
                    return rewrite_ota_seq(count[0].ota_seq + 1, 0, 3, ota_part_offset);
                } else {
                    // Write count1.seq + 1 to count0
                    return rewrite_ota_seq(count[1].ota_seq + 1, 0, 2, ota_part_offset);
                }
            } else if (valid0) {
                return rewrite_ota_seq(count[0].ota_seq + 1, 0, 3, ota_part_offset);
            } else if (valid1) {
                return rewrite_ota_seq(count[1].ota_seq + 1, 0, 2, ota_part_offset);
            } else {
                return ESP_FAIL;
            }
            break;

        case OTA_BOOT_COUNT_DISABLE:
        case OTA_BOOT_COUNT_ZERO:

            for (int i = 2; i <= 3; ++i) {
#ifdef BOOTLOADER_BUILD
                ret = bootloader_flash_erase_sector(ota_part_offset / SPI_FLASH_SEC_SIZE + i);
#else
                ret = spi_flash_erase_sector(ota_part_offset / SPI_FLASH_SEC_SIZE + i);
#endif
                if (ret != ESP_OK) {
                    return ret;
                }
            }

            if (op == OTA_BOOT_COUNT_ZERO) {
                return rewrite_ota_seq(0, 0, 2,  ota_part_offset);
            }


    }
    return ESP_FAIL;
}


#ifndef BOOTLOADER_BUILD
esp_err_t esp_ota_boot_count_op(esp_ota_boot_count_op_t op, uint32_t* count_return) {
    const esp_partition_t *find_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, NULL);
    if (find_partition != NULL) {
        if (find_partition->size <  SPI_FLASH_SEC_SIZE * 4) {
            return ESP_ERR_INVALID_SIZE;
        }
    }
    return esp_ota_boot_count_op_bootloader(find_partition->address, op, count_return);
}
#endif


