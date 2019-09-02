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
#include "ir_decoder.h"
#include "nrfx_timer.h"
#include "nrfx_gpiote.h"
#include "nrf_delay.h"
#include "nrfx_ppi.h"
#include "nrf_drv_ppi.h"
#include "app_error.h"
#include "nrf_log.h"
#include "boards.h"

//IR LED Input
#define IR_LED 18

#define PWM_TIMER_REDUCE_CPU_CYCLES 50 //50ms
const nrfx_timer_t IR_CARRIER_TIMER = NRFX_TIMER_INSTANCE(1);
const nrfx_timer_t IR_PWM_TIMER = NRFX_TIMER_INSTANCE(2);

void ir_carrier_init(void);
void ir_pwm_timer_init(void);
void pwm_timer_event_handler(nrf_timer_event_t event_type, void* p_context);
void dummy_timer_event_handler(nrf_timer_event_t event_type, void* p_context);

static uint32_t  ir_bit_index = 0;
static uint32_t  ir_pulse_count = 0;
ir_data_t *ir_signal_burst_to_send;

void send_ir_burst(ir_data_t *ir_data, uint32_t pulse_count) {

    ret_code_t err_code;

    ir_pulse_count = pulse_count;
    ir_signal_burst_to_send = ir_data;

    err_code = nrf_drv_ppi_init();
    APP_ERROR_CHECK(err_code);
    
    if(!nrfx_gpiote_is_init()) {
      err_code = nrfx_gpiote_init();
      APP_ERROR_CHECK(err_code);
    }

    ir_carrier_init();
    ir_pwm_timer_init();

    ir_bit_index = 0;
}

//Timer 1 will toggle IR LED via PPI
void ir_carrier_init(void) {
    
    uint32_t time_us = 13;
    uint32_t time_ticks;
    uint32_t compare_evt_addr;
    uint32_t gpiote_task_addr;
    nrf_ppi_channel_t ppi_channel;
    ret_code_t err_code;

    nrfx_timer_config_t timer_cfg = NRFX_TIMER_DEFAULT_CONFIG;
    err_code = nrfx_timer_init(&IR_CARRIER_TIMER, &timer_cfg, dummy_timer_event_handler);
    APP_ERROR_CHECK(err_code);

    nrfx_gpiote_out_config_t config = NRFX_GPIOTE_CONFIG_OUT_TASK_TOGGLE(false);
    
    err_code = nrfx_gpiote_out_init(IR_LED, &config);
    APP_ERROR_CHECK(err_code);
    
    time_ticks = nrfx_timer_us_to_ticks(&IR_CARRIER_TIMER, time_us);
    nrfx_timer_extended_compare(&IR_CARRIER_TIMER, NRF_TIMER_CC_CHANNEL0, time_ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false);
    
    err_code = nrfx_ppi_channel_alloc(&ppi_channel);
    APP_ERROR_CHECK(err_code);
    
    compare_evt_addr = nrfx_timer_event_address_get(&IR_CARRIER_TIMER, NRF_TIMER_EVENT_COMPARE0);
    gpiote_task_addr = nrfx_gpiote_out_task_addr_get(IR_LED);
    
    err_code = nrfx_ppi_channel_assign(ppi_channel, compare_evt_addr, gpiote_task_addr);
    APP_ERROR_CHECK(err_code);
    
    err_code = nrfx_ppi_channel_enable(ppi_channel);
    APP_ERROR_CHECK(err_code);
    
    nrfx_gpiote_out_task_enable(IR_LED);

    nrfx_timer_enable(&IR_CARRIER_TIMER);
    nrfx_timer_pause(&IR_CARRIER_TIMER);

}

void ir_pwm_timer_init(void) {
    
    uint32_t timer_stop_task_addr;
    uint32_t timer_event_addr;
    nrf_ppi_channel_t ppi_channel;
    ret_code_t err_code;

    nrfx_timer_config_t timer_cfg = NRFX_TIMER_DEFAULT_CONFIG;
    timer_cfg.frequency = 4;

    err_code = nrfx_timer_init(&IR_PWM_TIMER, &timer_cfg, pwm_timer_event_handler);
    APP_ERROR_CHECK(err_code);

    nrfx_timer_extended_compare(&IR_PWM_TIMER, NRF_TIMER_CC_CHANNEL0, 100000, NRF_TIMER_SHORT_COMPARE0_STOP_MASK, true);

    //PPI from PWM TIMER to IR_CARRIER TIMER Pause
    timer_stop_task_addr = nrfx_timer_task_address_get(&IR_CARRIER_TIMER, NRF_TIMER_TASK_STOP);
    timer_event_addr = nrfx_timer_event_address_get(&IR_PWM_TIMER, NRF_TIMER_EVENT_COMPARE0);
    
    err_code = nrfx_ppi_channel_alloc(&ppi_channel);
    APP_ERROR_CHECK(err_code);

    err_code = nrfx_ppi_channel_assign(ppi_channel, timer_event_addr, timer_stop_task_addr);
    APP_ERROR_CHECK(err_code);

    nrfx_timer_enable(&IR_PWM_TIMER);
}

void pwm_timer_event_handler(nrf_timer_event_t event_type, void* p_context) {

    switch (event_type)
    {
        case NRF_TIMER_EVENT_COMPARE0:
        {
            ir_data_t *x = &ir_signal_burst_to_send[ir_bit_index];

            uint32_t width_us = x->duty_cycle_us;
            uint32_t state = x->duty_cycle_state;

            nrfx_timer_disable(&IR_PWM_TIMER);
            nrfx_timer_extended_compare(&IR_PWM_TIMER, NRF_TIMER_CC_CHANNEL0, (width_us - PWM_TIMER_REDUCE_CPU_CYCLES), NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);

            nrfx_timer_enable(&IR_PWM_TIMER);

            if(state) {
                nrfx_timer_resume(&IR_CARRIER_TIMER);
            }
            else
            {
                nrfx_timer_pause(&IR_CARRIER_TIMER);
            }

            
            ir_bit_index++;

            if(ir_bit_index > ir_pulse_count) {

              nrfx_timer_disable(&IR_PWM_TIMER);
              nrfx_timer_disable(&IR_CARRIER_TIMER);
              nrfx_timer_uninit(&IR_PWM_TIMER);
              nrfx_timer_uninit(&IR_CARRIER_TIMER);
              //completion_task(ir_bit_index, ir_signal_burst);
            }
            break;
        }
        default:
            break;
    }

}

void dummy_timer_event_handler(nrf_timer_event_t event_type, void* p_context) {}
