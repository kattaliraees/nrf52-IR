//
//  ir_decoder.c
//  SmartIR
//
//  Created by Raees Kattali on 16/08/19.
//  Copyright © 2019 Raees Kattali. All rights reserved.
//


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

TSOP38238
#define IR_IN 17

static ir_data_t ir_signal_burst[256] = {0, 0};

static uint32_t ir_bit_index = 0;

ir_decode_complete_task *completion_task;

const nrfx_timer_t IR_CAPTURE_TIMER = NRFX_TIMER_INSTANCE(1);

void ir_capture_timer_interrupts(nrf_timer_event_t event_type, void* p_context);

void enable_ir_in_gpiote_interrupt();
void ir_in_gpiote_interrupt_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action);

bool started = false;

void start_capturing(ir_decode_complete_task t) {
  completion_task = t;
  enable_ir_in_gpiote_interrupt();
}

void enable_ir_in_gpiote_interrupt() {

    ret_code_t err_code;
    nrf_ppi_channel_t ppi_channel;
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

    duty_cycle_time_ticks = nrfx_timer_ms_to_ticks(&IR_CAPTURE_TIMER, 1000);
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
            if(!started) {
              nrfx_timer_pause(&IR_CAPTURE_TIMER);
              break;
            }
            nrfx_timer_disable(&IR_CAPTURE_TIMER);
            nrfx_timer_uninit(&IR_CAPTURE_TIMER);
            completion_task(ir_bit_index, ir_signal_burst);
            started = false;
            break;
        default:
            break;
    }
}


void ir_in_gpiote_interrupt_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    //nrfx_timer_pause(&IR_CAPTURE_TIMER); -PPI will stop the timer

    if(started == false) {
        started = true;
    }
    else
    {
        NRF_TIMER1->TASKS_CAPTURE[1] = 1;
        uint32_t pulse_width_us = NRF_TIMER1->CC[1];
        
        ir_signal_burst[ir_bit_index].duty_cycle_state = nrf_gpio_pin_read(IR_IN);
        ir_signal_burst[ir_bit_index].duty_cycle_us = pulse_width_us;
        ir_bit_index++;
        //NRF_LOG_INFO("%d. %d - %d", ir_bit_index, ir_signal_burst[ir_bit_index].duty_cycle_state, ir_signal_burst[ir_bit_index].duty_cycle_us);
    }

    
    nrfx_timer_clear(&IR_CAPTURE_TIMER);
    nrfx_timer_resume(&IR_CAPTURE_TIMER);
}
