void uart_init(void);
int uart_putchar(char c, FILE *stream);

FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);

