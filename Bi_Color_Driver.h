
#include <stdio.h>
#include "app_util_platform.h"
#include "app_error.h"
#include "nrf_drv_twi.h"

#include "nrf_delay.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"


#define LongToBin(n) \
    (\
    ((n >> 21) & 0x80) | \
    ((n >> 18) & 0x40) | \
    ((n >> 15) & 0x20) | \
    ((n >> 12) & 0x10) | \
    ((n >>  9) & 0x08) | \
    ((n >>  6) & 0x04) | \
    ((n >>  3) & 0x02) | \
    ((n      ) & 0x01)   \
    )
#define Bin(n) LongToBin(0x##n##l)


extern nrf_drv_twi_t twi_instance;

typedef struct {
    uint16_t displaybuffer[8];
    uint8_t i2c_addr;
    
    uint8_t rotation;
    int16_t cursor_y;
    int16_t cursor_x;
} NRF_bicolor;

#define LED_ON 1
#define LED_OFF 0

#define LED_RED 1
#define LED_YELLOW 2
#define LED_GREEN 3


#ifndef _BV
  #define _BV(bit) (1<<(bit))
#endif

#ifndef _swap_int16_t
  #define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }
#endif


  
#define HT16K33_BLINK_CMD 0x80
#define HT16K33_BLINK_DISPLAYON 0x01
#define HT16K33_BLINK_OFF 0
#define HT16K33_BLINK_2HZ  1
#define HT16K33_BLINK_1HZ  2
#define HT16K33_BLINK_HALFHZ  3
#define HT16K33_CMD_BRIGHTNESS 0xE0  
#define HT16K33_CMD_WRITE 0x00


// This needs to match the settings in nrf_drv_cfg.h for enabling the TWI, the SCL and SDA does not need to match,
// These are set in the init function called when initializing the object
nrf_drv_twi_t twi_instance = NRF_DRV_TWI_INSTANCE(0);
#define BICOLOR_SCL_PIN 27
#define BICOLOR_SDA_PIN 26

/**
 * @brief TWI events handler.
 */
uint8_t device_address = 0; // Address used to temporarily store the current address being checked
bool device_found = false; 
void twi_handler(nrf_drv_twi_evt_t const * p_event, void * p_context) ;

void twi_init () ;

void NRF_bicolor_TWI_init(void) ;

void NRF_bicolor_setBrightness(NRF_bicolor * t, uint8_t b) ;

void NRF_bicolor_blinkRate(NRF_bicolor * t, uint8_t b) ;

void NRF_bicolor_begin(NRF_bicolor* t, uint8_t _addr /* 0x70 */) ;

void NRF_bicolor_clear(NRF_bicolor* t) ;

uint8_t NRF_bicolor_getRotation(NRF_bicolor* t) ;

void NRF_bicolor_drawPixel(NRF_bicolor* t, int16_t x, int16_t y, uint16_t color) ;

uint8_t NRF_bicolor_write(NRF_bicolor* t, uint8_t c, uint16_t textcolor, uint16_t bgcolor);

void NRF_bicolor_writeDisplay(NRF_bicolor* t);