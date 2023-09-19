//
//  ir_decoder.h
//  SmartIR
//
//  Created by Raees Kattali on 16/08/19.
//  Copyright © 2019 Raees Kattali. All rights reserved.
//


#ifndef IR_DECODERL_H_
#define IR_DECODERL_H_ 

typedef struct  {
  uint32_t duty_cycle_state;
  uint32_t duty_cycle_us;
}ir_data_t;

typedef void  ir_decode_complete_task(int, ir_data_t *ir_data_ptr);

bool start_capturing(ir_decode_complete_task t);

#endif

