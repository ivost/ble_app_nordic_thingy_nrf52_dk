#include "Bi_Color_Driver.h"

/**
 * @brief TWI events handler.
 */

void twi_handler(nrf_drv_twi_evt_t const * p_event, void * p_context)
{   
    switch(p_event->type)
    {
        case NRF_DRV_TWI_EVT_DONE:
            device_found = true;
            break;
        case NRF_DRV_TWI_EVT_ADDRESS_NACK:
            NRF_LOG_INFO("No address ACK on address: %#x!\r\n", device_address);
            break;
        case NRF_DRV_TWI_EVT_DATA_NACK:
            NRF_LOG_INFO("No data ACK on address: %#x!\r\n", device_address);
            break;
        default:
            break;        
    }   
}

/**
 * @brief TWI initialization.
 */
void twi_init () {
    ret_code_t err_code;
    
    const nrf_drv_twi_config_t twi_config = {
       .scl                = BICOLOR_SCL_PIN,
       .sda                = BICOLOR_SDA_PIN,
       .frequency          = NRF_TWI_FREQ_100K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH
    };
    
    err_code = nrf_drv_twi_init(&twi_instance, &twi_config, twi_handler, NULL);
    APP_ERROR_CHECK(err_code);
    
    nrf_drv_twi_enable(&twi_instance);
}

/**
 * @brief Initialize TWI Wrapper.
 */
void NRF_bicolor_TWI_init(void) {
    twi_init();
    nrf_delay_ms(100);  // Delay to allow everything to start up
}

/*
 *  Function to set Led brightness
*/
void NRF_bicolor_setBrightness(NRF_bicolor * t, uint8_t b) {
    if (b > 15) {
        b = 15;  // Turn fully on if invalid input
    }
    uint8_t const bicolor_brightness_cmd = HT16K33_CMD_BRIGHTNESS | b;
    while(nrf_drv_twi_tx(&twi_instance, t->i2c_addr, &bicolor_brightness_cmd, 1, false) == NRF_ERROR_BUSY);
    nrf_delay_ms(1);
}

/*
 * Function to set the blink rate of the LEDs.
 *
*/
void NRF_bicolor_blinkRate(NRF_bicolor * t, uint8_t b) {
    if (b > 3) {
        b = 0;  // Turn off if level invalid
    }
    uint8_t const bicolor_blink_cmd = HT16K33_BLINK_CMD | HT16K33_BLINK_DISPLAYON | (b << 1);
    while(nrf_drv_twi_tx(&twi_instance, t->i2c_addr, &bicolor_blink_cmd, 1, false) == NRF_ERROR_BUSY);
    nrf_delay_ms(1);
}

/*
 * Init and configure TWi for Bi-color LED
 *
*/
void NRF_bicolor_begin(NRF_bicolor* t, uint8_t _addr /* 0x70 */) {
    t->i2c_addr = _addr;
    t->rotation = 0;
    t->cursor_y = 0;
    t->cursor_x = 0;
    
    //NRF_LOG_INFO("Writing to address %x",_addr);

    // Clear dataBuffer
    for (uint8_t i = 0; i < 8; i++) {
        t->displaybuffer[i] = 0x0;
    }
    
    // Turn on ocillator
    uint8_t const bicolor_oscillator_on = 0x21; 
    while (nrf_drv_twi_tx(&twi_instance, _addr, &bicolor_oscillator_on, sizeof(bicolor_oscillator_on), false) == NRF_ERROR_BUSY);
    nrf_delay_ms(10);
    // Turn off blink
    NRF_bicolor_blinkRate(t, HT16K33_BLINK_OFF);
    // Set brightness to max fully lit
    NRF_bicolor_setBrightness(t, 7);
}

/*
 * Clear LED data and set data to zero. 
*/
void NRF_bicolor_clear(NRF_bicolor* t) {
    for (uint8_t i = 0; i < 8; i++) {
        t->displaybuffer[i] = 0;
    }
}

uint8_t NRF_bicolor_getRotation(NRF_bicolor* t) {
    return t->rotation;
}

void NRF_bicolor_drawPixel(NRF_bicolor* t, int16_t x, int16_t y, uint16_t color) {
    if ((y < 0) || (y >= 8)) return;
    if ((x < 0) || (x >= 8)) return;
    
    switch(NRF_bicolor_getRotation(t)) {
    case 1:
        _swap_int16_t(x, y);
        x = 8 - x - 1;
        break;
    case 2:
        x = 8 - x - 1;
        y = 8 - y - 1;
        break;
    case 3:
        _swap_int16_t(x, y);
        y = 8 - y - 1;
        break;
    }
    
    if (color == LED_GREEN) {
        // Turn on green LED
        t->displaybuffer[y] |= 1 << x;
        // Turn off red LED.
        t->displaybuffer[y] &= ~(1 << (x+8));
    } else if (color == LED_RED) {
        // Turn on red LED.
        t->displaybuffer[y] |= 1 << (x+8);
        // Turn off green LED.
        t->displaybuffer[y] &= ~(1 << x);
    } else if (color == LED_YELLOW) {
        // Turn on green and red LED.
        t->displaybuffer[y] |= (1 << (x+8)) | (1 << x);
    } else if (color == LED_OFF) {
        // Turn off green and red LED.
        t->displaybuffer[y] &= ~(1 << x) & ~(1 << (x+8));
    }
}


uint8_t NRF_bicolor_write(NRF_bicolor* t, uint8_t c, uint16_t textcolor, uint16_t bgcolor) {
    NRF_bicolor_drawChar(t, t->cursor_x, t->cursor_y, c, textcolor, bgcolor);
    t->cursor_x += 6;
    return 1;
}

uint8_t NRF_bicolor_write_string(NRF_bicolor* t, const uint8_t *buffer, uint8_t size, uint16_t textcolor, uint16_t bgcolor) {
    uint8_t n = 0;
    while(size--) {
        if (NRF_bicolor_write(t, *buffer++, textcolor, bgcolor)) n++;
        else break;
    }
    return n;
}

void NRF_bicolor_writeDisplay(NRF_bicolor* t) {
    // Insert start write cmd at beginning
    uint8_t twi_write_data[17] = {HT16K33_CMD_WRITE};
    // Insert data from displaybuffer into TWI write array
    for (uint8_t i = 0; i < 8; i++) {
        twi_write_data[(i*2)+1] = t->displaybuffer[i] & 0xFF;
        twi_write_data[(i*2)+2] = t->displaybuffer[i] >> 8;
    }
    while (nrf_drv_twi_tx(&twi_instance, t->i2c_addr, twi_write_data, 17, false) == NRF_ERROR_BUSY);
    nrf_delay_ms(1);
}