#ifndef IRBABY_IR_H_
#define IRBABY_IR_H_
#include "conf.h"

typedef enum
{
    IR_SEND_DECODE,
    IR_SEND_INCOMING
} ir_send_type;
int ir_transmission(ir_send_type send_type);
void ir_receive_start();
void ir_receive_stop();
#endif // IRBABY_IR_H_