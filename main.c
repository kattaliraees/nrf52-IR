MIT License

Copyright (c) 2019 Raees Kattali

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
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

void clock_event_handler(nrfx_clock_evt_type_t event) {}
void ir_decode_task_completed (int number_of_bits, ir_data_t *ir_data_ptr);
void ir_transmit_task_completed (void);

int main(void)
{
    ret_code_t err_code = nrfx_clock_init(&clock_event_handler);
    APP_ERROR_CHECK(err_code);

    nrfx_clock_hfclk_start();

    while(!nrfx_clock_hfclk_is_running()){}
    
    //Log Init
    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
    NRF_LOG_DEFAULT_BACKENDS_INIT();
    NRF_LOG_FLUSH();

    
    NRF_LOG_INFO("Started");
    NRF_LOG_PROCESS();
    
    start_capturing(&ir_decode_task_completed);

    while (1) {
        NRF_LOG_PROCESS();
    }
}

void ir_decode_task_completed (int number_of_bits, ir_data_t *ir_data_ptr) {
    
    nrf_delay_ms(2000);
    send_ir_burst(ir_data_ptr, number_of_bits, &ir_transmit_task_completed);
}

void ir_transmit_task_completed (void) {
    start_capturing(&ir_decode_task_completed);
}
