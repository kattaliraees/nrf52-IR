//
//  ir_transmitter.h
//  SmartIR
//
//  Created by Raees Kattali on 16/08/19.
//  Copyright © 2019 Raees Kattali. All rights reserved.
//

#include "ir_decoder.h"

typedef void  ir_transmit_complete_task(void);

bool send_ir_burst(ir_data_t *ir_data, uint32_t pulse_count, ir_transmit_complete_task t);