#ifndef UART_H_
#define UART_H_

#include <stdint.h>

void UART_SendString(const char *string);
void Send_Motor_Status(void);
#endif /* UART_H_ */