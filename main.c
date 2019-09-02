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
#include "ir_transmitter.h"
#include "nrfx_clock.h"


void enable_status_led(void);
void log_init(void);

void ir_decode_task_completed (int number_of_bits, ir_data_t *ir_data_ptr) {

    nrf_delay_ms(1000);

    //Test
    send_ir_burst(ir_data_ptr, number_of_bits);
}

void clock_event_handler(nrfx_clock_evt_type_t event) {}

int main(void)
{
    ret_code_t err_code = nrfx_clock_init(&clock_event_handler);
    APP_ERROR_CHECK(err_code);

    nrfx_clock_hfclk_start();

    while(!nrfx_clock_hfclk_is_running()){}
    
    log_init();

    NRF_LOG_INFO("Started");
    NRF_LOG_PROCESS();

    //Test DATA
    //ir_data_t a[] = {{1,9063}, {0,4472}, {1,541}, {0,551}, {1,538}, {0,552}, {1,537}, {0,1670}, {1,541}, {0,550}, {1,539}, {0,550}, {1,539}, {0,549}, {1,541}, {0,550}, {1,537}, {0,1670}, {1,541}, {0,550}};
    //send_ir_burst(a, 10);
    
    start_capturing(&ir_decode_task_completed);

    while (1) {
        //NRF_LOG_INFO("inside loop");
        NRF_LOG_PROCESS();
    }
}

void log_init(void) {
    
    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
    NRF_LOG_DEFAULT_BACKENDS_INIT();
    NRF_LOG_FLUSH();
    NRF_LOG_INFO("SmartIR Starting...");
}

