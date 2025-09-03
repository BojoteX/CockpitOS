#pragma once

#define DCS_USB_RINGBUF_SIZE                     32  // Number of packets buffered (tune as needed)
#define DCS_USB_PACKET_MAXLEN                    64  // Max USB packet size (safe for DCS-BIOS)
// #define DCS_UDP_PACKET_MAXLEN                  64  // Should ALWAYS be 64 when USE_DCSBIOS_USB 

// --- UDP Receive buffer struct ---
typedef struct {
    uint8_t data[DCS_USB_PACKET_MAXLEN];
    size_t len;
    bool isLastChunk;
} DcsUdpRingMsg;

// --- UDP Receive Ring Buffer API ---
bool    dcsUdpRingbufPop(DcsUdpRingMsg* out);
void    dcsUdpRingbufPush(const uint8_t* data, size_t len, bool isLastChunk);
void    dcsUdpRingbufPushChunked(const uint8_t* data, size_t len);

size_t  dcsUdpRingbufPending(void);
size_t  dcsUdpRingbufAvailable(void);

// --- Stats ---
uint32_t dcsUdpRecvGetOverflow(void);
size_t   dcsUdpRecvGetHighWater(void);
size_t   dcsUdpRecvGetPending(void);
float    dcsUdpRecvAvgMsgLen(void);
size_t   dcsUdpRecvMaxMsgLen(void);

// --- RAW USB Out buffer struct (Device → Host) ---
typedef struct {
    uint8_t data[DCS_USB_PACKET_MAXLEN];
    size_t len;
    bool isLastChunk;
} DcsRawUsbOutRingMsg;

// --- RAW USB Out Ring Buffer API ---
bool    dcsRawUsbOutRingbufPop(DcsRawUsbOutRingMsg* out);
void    dcsRawUsbOutRingbufPush(const uint8_t* data, size_t len, bool isLastChunk);
void    dcsRawUsbOutRingbufPushChunked(const uint8_t* data, size_t len);

size_t  dcsRawUsbOutRingbufPending(void);
size_t  dcsRawUsbOutRingbufAvailable(void);

// --- Stats ---
uint32_t dcsRawUsbOutGetOverflow(void);
size_t   dcsRawUsbOutGetHighWater(void);
size_t   dcsRawUsbOutGetPending(void);
float    dcsRawUsbOutAvgMsgLen(void);
size_t   dcsRawUsbOutMaxMsgLen(void);

// --- Debug Tools
void dumpUsbOutRingBuffer();