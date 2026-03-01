#ifndef __DCSBIOS_EXPORTSTREAMLISTENER_H
#define __DCSBIOS_EXPORTSTREAMLISTENER_H

namespace DcsBios {
	class ExportStreamListener {
	protected:
		uint16_t firstAddressOfInterest;
		uint16_t lastAddressOfInterest;
	public:
		virtual void onDcsBiosWrite(uint16_t address __attribute__((unused)), uint16_t value __attribute__((unused))) {}
		virtual void onConsistentData() {}
		ExportStreamListener* nextExportStreamListener;
		inline uint16_t getFirstAddressOfInterest() { return firstAddressOfInterest; }
		inline uint16_t getLastAddressOfInterest() { return lastAddressOfInterest; }
		
		static ExportStreamListener* firstExportStreamListener;
		ExportStreamListener(uint16_t firstAddressOfInterest, uint16_t lastAddressOfInterest) {
			this->firstAddressOfInterest = firstAddressOfInterest & ~(0x01);
			this->lastAddressOfInterest = lastAddressOfInterest & ~(0x01);
			if (firstExportStreamListener == NULL) {
				firstExportStreamListener = this;
				nextExportStreamListener = NULL;
				return;
			}
			ExportStreamListener** prevNextPtr = &firstExportStreamListener;
			ExportStreamListener* nextESL = firstExportStreamListener;
			while (nextESL && ((nextESL->getLastAddressOfInterest() < this->lastAddressOfInterest) ||
				(nextESL->getLastAddressOfInterest() == this->lastAddressOfInterest && nextESL->getFirstAddressOfInterest() < this->firstAddressOfInterest))) {
				prevNextPtr = &(nextESL->nextExportStreamListener);
				nextESL = nextESL->nextExportStreamListener;
			}
			this->nextExportStreamListener = nextESL;
			*prevNextPtr = this;
		}
		static void loopAll() {
			ExportStreamListener* el = firstExportStreamListener;
			while (el) {
				el->loop();
				el = el->nextExportStreamListener;
			}
		}
		virtual void loop() {}
	};
}
#endif