# nrf52, 51 - IR Capture/Transmitter #

  * Capturing IR Burst from any IR remote and ability to send the same burst. 
  * This will work irrespective of the IR Protocol
  * Tested with nrf SDK v15.3 & nrf52832 custom board (IR LED + TSOP38238)
  * Required nrfx drivers PPI, Timer, LOG, Clock
  * Have to be in 64MHz HFCLK
  
  ```C
  start_capturing(&ir_decode_task_completed);
  
  void ir_decode_task_completed (int number_of_bits, ir_data_t *ir_data_ptr) {
    nrf_delay_ms(5000);
    send_ir_burst(ir_data_ptr, number_of_bits, &ir_transmit_task_completed); //Sending what is received after 5 Secs
  }
    
  void ir_transmit_task_completed (void) {
    start_capturing(&ir_decode_task_completed);
  }
  ```
  
  ![Image of Schematic](https://github.com/kattaliraees/nrf52-IR/blob/master/schematic.png)
  

