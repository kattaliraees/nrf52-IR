//
//  main.c
//  SmartIR
//
//  Created by Raees Kattali on 14/02/19.
//  Copyright © 2019 Raees Kattali. All rights reserved.
//

#include <stdbool.h>
#include <stdint.h>

#include "nrf_delay.h"
#include "boards.h"
#include "app_util_platform.h"
#include "app_error.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "i2c_drv.h"
#include "AT24C32.h"
#include "DS3231.h"
#include "ir_transmitter.h"
#include "nrfx_clock.h"


void enable_status_led(void);
void log_init(void);

void ir_decode_task_completed (int number_of_bits, ir_data_t *ir_data_ptr) {

    nrf_delay_ms(1000);

    send_ir_burst(ir_data_ptr, number_of_bits);
}

void clock_event_handler(nrfx_clock_evt_type_t event) {

    NRF_LOG_INFO("clock_event_handler");
}

int main(void)
{
    ret_code_t err_code;
    

    err_code = nrfx_clock_init(&clock_event_handler);
    APP_ERROR_CHECK(err_code);

    twi_init();
    scan_i2c_devices();

    //NRF_LOG_INFO("Device Started");
    nrfx_clock_hfclk_start();
    //NRF_LOG_INFO("Waiting for HFCLK to Start..");
    while(!nrfx_clock_hfclk_is_running()){}
    

    enable_status_led();
    log_init();

    NRF_LOG_INFO("Started");
    NRF_LOG_PROCESS();

    //Test DATA
    //ir_data_t a[] = {{1,9063}, {0,4472}, {1,541}, {0,551}, {1,538}, {0,552}, {1,537}, {0,1670}, {1,541}, {0,550}, {1,539}, {0,550}, {1,539}, {0,549}, {1,541}, {0,550}, {1,537}, {0,1670}, {1,541}, {0,550}};

    start_decoding(&ir_decode_task_completed);
    //send_ir_burst(a, 10);
    

    while (1) {
        //NRF_LOG_INFO("l");
        NRF_LOG_PROCESS();
    }
}



void enable_status_led(void) {
    
    nrf_gpio_cfg_output(STATUS_LED);
    //nrf_gpio_cfg_output(IR_LED);
    nrf_gpio_pin_set(STATUS_LED);
    nrf_delay_ms(100);
    nrf_gpio_pin_clear(STATUS_LED);
}

void log_init(void) {
    
    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
    NRF_LOG_DEFAULT_BACKENDS_INIT();
    NRF_LOG_FLUSH();
    NRF_LOG_INFO("SmartIR Starting...");
}

