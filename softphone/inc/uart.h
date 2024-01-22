#ifndef UART_H_INCLUDED
#define UART_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

void uart_init(void);

int uart_init_shell(void);

#ifdef __cplusplus
}
#endif

#endif
