//
//  i2c_drv.h
//  SmartIR
//
//  Created by Raees Kattali on 14/08/19.
//  Copyright © 2019 Raees Kattali. All rights reserved.
//

#include "nrf_drv_twi.h"

/* TWI instance ID. */
#if TWI0_ENABLED
#define TWI_INSTANCE_ID     0
#elif TWI1_ENABLED
#define TWI_INSTANCE_ID     1
#endif
/* Number of possible TWI addresses. */
#define TWI_ADDRESSES      127
/* TWI instance. */

static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);

void twi_init (void);
void scan_i2c_devices(void);