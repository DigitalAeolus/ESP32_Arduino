#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <Arduino.h>
#include <esp_heap_caps.h>

class MemoryManager {
public:
    static void printMemoryInfo() {
        Serial.printf("Free heap: %d bytes\n", esp_get_free_heap_size());
        Serial.printf("Free SPIRAM: %d bytes\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        Serial.printf("Free internal: %d bytes\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
        Serial.printf("Largest free block: %d bytes\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    }
    
    static bool checkMemoryHealth() {
        size_t free_heap = esp_get_free_heap_size();
        size_t free_spiram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        
        if (free_heap < 20000) {
            Serial.println("WARNING: Low heap memory!");
            return false;
        }
        
        if (free_spiram < 50000) {
            Serial.println("WARNING: Low SPIRAM memory!");
            return false;
        }
        
        return true;
    }
    
    static void* safeAllocSPIRAM(size_t size) {
        void* ptr = heap_caps_aligned_alloc(4, size, MALLOC_CAP_SPIRAM);
        if (ptr == NULL) {
            Serial.printf("Failed to allocate %d bytes in SPIRAM\n", size);
            printMemoryInfo();
        }
        return ptr;
    }
    
    static void* safeAllocInternal(size_t size) {
        void* ptr = heap_caps_aligned_alloc(4, size, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
        if (ptr == NULL) {
            Serial.printf("Failed to allocate %d bytes in internal RAM\n", size);
            printMemoryInfo();
        }
        return ptr;
    }
};

#endif