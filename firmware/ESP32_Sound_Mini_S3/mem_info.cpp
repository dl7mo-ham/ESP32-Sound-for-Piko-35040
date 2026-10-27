/*
 * mem_info.cpp  -  Soundmodul Mini, siehe mem_info.h
 */

#include "mem_info.h"
#include "sound_store.h"
#include <esp_ota_ops.h>

MemInfo memInfoRead() {
    // Die Firmware-Groesse aendert sich bis zum naechsten Start nicht,
    // ESP.getSketchSize() liest dafuer das Image aus dem Flash - einmal reicht.
    static uint32_t appUsed = 0, appSize = 0;
    if (appSize == 0) {
        const esp_partition_t* p = esp_ota_get_running_partition();
        appSize = p ? p->size : 0;
        appUsed = ESP.getSketchSize();
    }
    MemInfo m;
    m.appUsed   = appUsed;
    m.appSize   = appSize;
    m.fsUsed    = (uint32_t)soundStoreUsedBytes();
    m.fsSize    = (uint32_t)soundStoreTotalBytes();
    m.heapFree  = ESP.getFreeHeap();
    m.heapMin   = ESP.getMinFreeHeap();
    m.heapSize  = ESP.getHeapSize();
    m.psramFree = ESP.getFreePsram();
    m.psramSize = ESP.getPsramSize();
    return m;
}

uint8_t memPercent(uint32_t used, uint32_t size) {
    if (size == 0) return 0;
    uint64_t p = ((uint64_t)used * 100 + size / 2) / size;
    return (uint8_t)(p > 100 ? 100 : p);
}
