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
#include "app_error.h"
#include "nrf_log.h"
#include "boards.h"
#include "nrfx_ppi.h"
#include "nrf_log_ctrl.h"

//TSOP38238
#define IR_IN 17

static ir_data_t ir_signal_burst[256] = {0, 0};

static uint32_t ir_bit_index = 0;

static nrf_ppi_channel_t ppi_channel;

ir_decode_complete_task *completion_task;

const nrfx_timer_t IR_CAPTURE_TIMER = NRFX_TIMER_INSTANCE(1);

void ir_capture_timer_interrupts(nrf_timer_event_t event_type, void* p_context);
void enable_ir_in_gpiote_interrupt();
void ir_in_gpiote_interrupt_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action);

bool is_decoding = false;

bool start_capturing(ir_decode_complete_task t) {

    if(is_decoding) {
        t(0, NULL);
        return false;
    }
    
    ir_bit_index = 0;
    completion_task = t;
    enable_ir_in_gpiote_interrupt();

    return true;
}

void enable_ir_in_gpiote_interrupt() {

    ret_code_t err_code;
    uint32_t timer_start_task_addr;
    uint32_t gpiote_in_event_addr;

    //GPIOTE Toggle interrupt
    if(!nrfx_gpiote_is_init()) {
        err_code = nrfx_gpiote_init();
        APP_ERROR_CHECK(err_code);
    }
    
    nrfx_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
    in_config.pull = NRF_GPIO_PIN_NOPULL;

    err_code = nrfx_gpiote_in_init(IR_IN, &in_config, ir_in_gpiote_interrupt_handler);
    APP_ERROR_CHECK(err_code);

    nrfx_gpiote_in_event_enable(IR_IN, true);

    //Timer init with 1us per tick.
    nrfx_timer_config_t timer_cfg = NRFX_TIMER_DEFAULT_CONFIG;
    timer_cfg.frequency = 4; //1MHz clock
    err_code = nrfx_timer_init(&IR_CAPTURE_TIMER, &timer_cfg, ir_capture_timer_interrupts);
    APP_ERROR_CHECK(err_code);
    
    uint32_t duty_cycle_time_ticks;

    duty_cycle_time_ticks = nrfx_timer_ms_to_ticks(&IR_CAPTURE_TIMER, 100);
    nrfx_timer_extended_compare(&IR_CAPTURE_TIMER, NRF_TIMER_CC_CHANNEL0, duty_cycle_time_ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);
    
    //PPI from GPIOTE to Timer Pause
    timer_start_task_addr = nrfx_timer_task_address_get(&IR_CAPTURE_TIMER, NRF_TIMER_TASK_STOP);
    gpiote_in_event_addr = nrfx_gpiote_in_event_addr_get(IR_IN);
    
    err_code = nrfx_ppi_channel_alloc(&ppi_channel);
    APP_ERROR_CHECK(err_code);
    
    err_code = nrfx_ppi_channel_assign(ppi_channel, gpiote_in_event_addr, timer_start_task_addr);
    APP_ERROR_CHECK(err_code);
    
    err_code = nrfx_ppi_channel_enable(ppi_channel);
    APP_ERROR_CHECK(err_code);
    
    //Reset Timer and wait for first GPIOTE Event
    nrfx_timer_enable(&IR_CAPTURE_TIMER);
    nrfx_timer_pause(&IR_CAPTURE_TIMER);
    nrfx_timer_clear(&IR_CAPTURE_TIMER);
}

void ir_capture_timer_interrupts(nrf_timer_event_t event_type, void* p_context){
    
    switch (event_type)
    {
        case NRF_TIMER_EVENT_COMPARE0:
            if(!is_decoding) {
                //Wait for IR signal start
                nrfx_timer_pause(&IR_CAPTURE_TIMER);
                break;
            }
            nrfx_timer_disable(&IR_CAPTURE_TIMER);
            nrfx_timer_uninit(&IR_CAPTURE_TIMER);
            nrfx_ppi_channel_disable(ppi_channel);
            nrfx_ppi_channel_free(ppi_channel);
            nrfx_gpiote_in_event_disable(IR_IN);
            nrfx_gpiote_in_uninit(IR_IN);
            is_decoding = false;
            completion_task(ir_bit_index, ir_signal_burst);
            break;
        default:
            break;
    }
}


void ir_in_gpiote_interrupt_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    //nrfx_timer_pause(&IR_CAPTURE_TIMER); -PPI will stop the timer

    NRF_TIMER1->TASKS_CAPTURE[1] = 1;
    uint32_t pulse_width_us = NRF_TIMER1->CC[1]; //IR Pulse width 
    nrfx_timer_clear(&IR_CAPTURE_TIMER);
    nrfx_timer_resume(&IR_CAPTURE_TIMER);

    if(is_decoding == false) {
        is_decoding = true;
    }
    else
    {
        ir_signal_burst[ir_bit_index].duty_cycle_state = nrf_gpio_pin_read(IR_IN);
        ir_signal_burst[ir_bit_index].duty_cycle_us = pulse_width_us + 5; //+ 5 for timing calibrations
        ir_bit_index++;
        //NRF_LOG_INFO("%d. %d - %d", ir_bit_index, ir_signal_burst[ir_bit_index].duty_cycle_state, ir_signal_burst[ir_bit_index].duty_cycle_us);
    }
}
