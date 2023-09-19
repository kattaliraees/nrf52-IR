//
//  i2c_drv.c
//  SmartIR
//
//  Created by Raees Kattali on 14/08/19.
//  Copyright © 2019 Raees Kattali. All rights reserved.
//

#include "i2c_drv.h"
#include "boards.h"
#include "nrf_log_ctrl.h"
#include "nrf_log.h"

#define AT24C32_I2C_ADDR    0x50
#define DS3231_I2C_ADDR     0x68



void twi_init (void) {
    
    ret_code_t err_code;
    
    const nrf_drv_twi_config_t twi_config = {
        .scl                = ARDUINO_SCL_PIN,
        .sda                = ARDUINO_SDA_PIN,
        .frequency          = NRF_DRV_TWI_FREQ_100K,
        .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
        .clear_bus_init     = false
    };
    
    err_code = nrf_drv_twi_init(&m_twi, &twi_config, NULL, NULL);
    APP_ERROR_CHECK(err_code);
    
    nrf_drv_twi_enable(&m_twi);
}

void scan_i2c_devices(void) {
    
    ret_code_t err_code;
    uint8_t sample_data;
    
    err_code = nrf_drv_twi_rx(&m_twi, AT24C32_I2C_ADDR, &sample_data, sizeof(sample_data));
    if (err_code == NRF_SUCCESS) {
        NRF_LOG_INFO("AT24C32 found");
    }
    else
    {
        NRF_LOG_INFO("Couldn't find AT24C32 in I2C bus");
        NRF_LOG_FLUSH();
    }
    
    err_code = nrf_drv_twi_rx(&m_twi, DS3231_I2C_ADDR, &sample_data, sizeof(sample_data));
    if (err_code == NRF_SUCCESS) {
        NRF_LOG_INFO("DS3231 found");
    }
    else
    {
        NRF_LOG_INFO("Couldn't find DS3231 in I2C bus");
        NRF_LOG_FLUSH();
    }
}





