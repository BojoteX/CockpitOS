#include "Protocol.h"

namespace DcsBios {
    ExportStreamListener* ExportStreamListener::firstExportStreamListener = NULL;

    static inline void flushRemainingListeners(ExportStreamListener*& start) {
        while (start) {
            start->onConsistentData();
            start = start->nextExportStreamListener;
        }
    }

    ProtocolParser::ProtocolParser() {
        processingData = false;
        state = DCSBIOS_STATE_WAIT_FOR_SYNC;
        sync_byte_count = 0;
        inDataBlock = false;  // ADD THIS
    }

    void ProtocolParser::processChar(unsigned char c) {

        switch (state) {
        case DCSBIOS_STATE_WAIT_FOR_SYNC:
            break;

        case DCSBIOS_STATE_ADDRESS_LOW:
            address = (unsigned int)c;
            state = DCSBIOS_STATE_ADDRESS_HIGH;
            break;

        case DCSBIOS_STATE_ADDRESS_HIGH:
            address = (c << 8) | address;
            if (address != 0x5555) state = DCSBIOS_STATE_COUNT_LOW;
            else state = DCSBIOS_STATE_WAIT_FOR_SYNC;
            break;

        case DCSBIOS_STATE_COUNT_LOW:
            count = (unsigned int)c;
            state = DCSBIOS_STATE_COUNT_HIGH;
            break;

        case DCSBIOS_STATE_COUNT_HIGH:
            count = (c << 8) | count;
            state = DCSBIOS_STATE_DATA_LOW;
            inDataBlock = true;  // ENTERING DATA BLOCK
            break;

        case DCSBIOS_STATE_DATA_LOW:
            data = (unsigned int)c;
            count--;
            state = DCSBIOS_STATE_DATA_HIGH;
            break;

        case DCSBIOS_STATE_DATA_HIGH:
            data = (c << 8) | data;
            count--;
            processingData = true;

            while (startESL && startESL->getLastAddressOfInterest() < address) {
                startESL->onConsistentData();
                startESL = startESL->nextExportStreamListener;
            }

            if (startESL) {
                ExportStreamListener* el = startESL;
                while (el) {
                    if (el->getFirstAddressOfInterest() > address) break;
                    if (el->getFirstAddressOfInterest() <= address && el->getLastAddressOfInterest() >= address) {
                        el->onDcsBiosWrite(address, data);
                    }
                    el = el->nextExportStreamListener;
                }
            }

            address += 2;
            if (count == 0) {
                state = DCSBIOS_STATE_ADDRESS_LOW;
                // inDataBlock set to false AFTER sync check
            }
            else state = DCSBIOS_STATE_DATA_LOW;
            break;
        }

        // SYNC detection - ONLY when NOT in data block
        if (!inDataBlock) {
            if (c == 0x55) sync_byte_count++;
            else sync_byte_count = 0;
        }

        // Exit data block AFTER sync check (so the last data byte isn't counted)
        if (inDataBlock && state == DCSBIOS_STATE_ADDRESS_LOW) {
            inDataBlock = false;
        }

        if (sync_byte_count == 4) {
            if (processingData) {
                flushRemainingListeners(startESL);
                processingData = false;
            }

            state = DCSBIOS_STATE_ADDRESS_LOW;
            sync_byte_count = 0;
            inDataBlock = false;  // Also reset here for safety
            startESL = ExportStreamListener::firstExportStreamListener;
        }
    }
}