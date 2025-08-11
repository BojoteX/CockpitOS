#define USER_SETUP_ID 987

// ----------------- Display Driver -----------------
#define GC9A01_DRIVER

// ----------------- Display Size -------------------
#define TFT_WIDTH  	240
#define TFT_HEIGHT 	240

// ----------------- Pin Mapping (S2) -----------------
#define TFT_MOSI   	8         		// SDA
#define TFT_SCLK    	9         		// SCL
#define TFT_DC     	13         		// Data/Command (any GPIO, use as wired)
#define TFT_CS     	36         		// SS / CS (Chip Select)
#define TFT_RST    	12         		// Reset (any GPIO, use as wired)
#define TFT_MISO   	-1         		// Not used (write-only)

// ----------------- Pin Mapping (S3) -----------------
// #define TFT_MOSI   	11      		// SDA
// #define TFT_SCLK    	12      		// SCL
// #define TFT_DC     	33      		// Data/Command (any GPIO, use as wired)
// #define TFT_CS     	10      		// CS (Chip Select)
// #define TFT_RST    	-1      		// Reset (any GPIO, use as wired)
// #define TFT_MISO   	-1			// Not used (write-only)

// ----------------- Hardware SPI / DMA -------------
// #define TFT_SDA_READ					// If TFT_SDA_READ is ENABLED then DISABLE USE_HSPI_PORT
#define USE_HSPI_PORT         			// Enables hardware SPI and DMA (on ESP32-S2/S3)
#define SPI_FREQUENCY  80000000  
// #define SPI_FREQUENCY  40000000
// #define SPI_READ_FREQUENCY  20000000

// ----------------- Fonts & Sprites ----------------
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

// #define SUPPORT_TRANSACTIONS