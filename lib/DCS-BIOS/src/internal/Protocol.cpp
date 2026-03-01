#include "Protocol.h"

namespace DcsBios {
	ExportStreamListener* ExportStreamListener::firstExportStreamListener = NULL;

	// Flush onConsistentData() for all remaining listeners from the current start pointer.
	// This makes end-of-frame commits deterministic on SYNC, even when lastAddressOfInterest == 0xFFFF.
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
		// startESL is initialized on first SYNC sequence.
	}

	void ProtocolParser::processChar(unsigned char c) {

		switch(state) {
			case DCSBIOS_STATE_WAIT_FOR_SYNC:
				break;

			case DCSBIOS_STATE_ADDRESS_LOW:
				address = (uint16_t)c;
				state = DCSBIOS_STATE_ADDRESS_HIGH;
				break;

			case DCSBIOS_STATE_ADDRESS_HIGH:
				address = ((uint16_t)c << 8) | address;
				if (address != 0x5555) state = DCSBIOS_STATE_COUNT_LOW;
				else state = DCSBIOS_STATE_WAIT_FOR_SYNC;
				break;

			case DCSBIOS_STATE_COUNT_LOW:
				count = (uint16_t)c;
				state = DCSBIOS_STATE_COUNT_HIGH;
				break;

			case DCSBIOS_STATE_COUNT_HIGH:
				count = ((uint16_t)c << 8) | count;
				state = DCSBIOS_STATE_DATA_LOW;
				break;

			case DCSBIOS_STATE_DATA_LOW:
				data = (uint16_t)c;
				count = count - 1;
				state = DCSBIOS_STATE_DATA_HIGH;
				break;

			case DCSBIOS_STATE_DATA_HIGH:
				data = ((uint16_t)c << 8) | data;
				count = count - 1;

				// We have processed at least one payload word in this frame.
				processingData = true;

				// Original behavior: flush listeners when address exceeds their range.
				while(startESL && startESL->getLastAddressOfInterest() < address) {
					startESL->onConsistentData();
					startESL = startESL->nextExportStreamListener;
				}

				// Dispatch write to any listener interested in this address.
				if (startESL) {
					ExportStreamListener* el = startESL;
					while(el) {
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
					// sync_byte_count = 0;  // Reset sync detection after data block
				}
				else state = DCSBIOS_STATE_DATA_LOW;
				break;
		}

		// SYNC detection (0x55 0x55 0x55 0x55)
		if (c == 0x55) sync_byte_count = sync_byte_count + 1;
		else sync_byte_count = 0;

		if (sync_byte_count == 4) {

			// End-of-frame boundary:
			// Flush any listeners that did not get flushed via address progression.
			if (processingData) {
				flushRemainingListeners(startESL);
				processingData = false;
			}

			state = DCSBIOS_STATE_ADDRESS_LOW;
			sync_byte_count = 0;
			startESL = ExportStreamListener::firstExportStreamListener;
		}
	}
}
