#include "shell_system.h"
#include "shell.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>


/**
 * shell_func_t : contains informations of a shell command
 */
typedef struct{
	const char * cmd;
	enum shell_error (* func)(int argc, char ** argv);
	const char * description;
} shell_func_t;


#define SHELL_FUNC_LIST_MAX_SIZE 32
#define SHELL_CMD_MAX_SIZE 24
#define SHELL_ARGC_MAX 8
#define SHELL_BUFFER_SIZE 128

static UART_HandleTypeDef* shell_huart = NULL;

static const char prompt[] = "$> ";

static uint8_t pos = 0;             // current buffer index
static char buf[SHELL_BUFFER_SIZE];
static char backspace[] = "\b \b";

static int shell_func_list_size = 0;
static shell_func_t shell_func_list[SHELL_FUNC_LIST_MAX_SIZE];

static uint8_t shell_exec(char * cmd);

static const char* get_shell_err_desc(enum shell_error err) {
    switch (err) {
    case SHELL_ERR_NONE:
        return "OK/NONE";
    case SHELL_ERR_UNKNOWN_CMD:
        return "Unknown command";
    case SHELL_ERR_INVALID_ARG_CNT:
        return "Invalid argument count";
    case SHELL_ERR_INVALID_ARG_VAL:
        return "Invalid argument value";
    default:
        return "???";
    }
}

#if 0
/**
 * __io_putchar : Required to printf() on uart
 * @param ch character to write on uart
 * @return 0 if HAL_OK
 */
uint8_t __io_putchar(int ch) {
	if(HAL_OK != HAL_UART_Transmit(shell_huart, (uint8_t *)&ch, 1, HAL_MAX_DELAY)){
		return 1;
	}
	return 0;
}
#endif

/**
 * uart_write : Write on uart
 * @param s string to write on uart
 * @param size length of the string
 * @return 0 if HAL_OK
 */
static uint8_t uart_write(const char *s, uint16_t size) {
	if(HAL_OK != HAL_UART_Transmit(shell_huart, (uint8_t*)s, size, 0xFFFF)){
		return 1;
	}
	return 0;
}

/**
 * sh_help : shell help menu
 * @param argc number of arguments of the command
 * @param argv arguments in strings array
 * @return 0
 */
static enum shell_error sh_help(int argc, char ** argv) {
    printf("Registered %u shell functions, capacity = %u\r\n", shell_func_list_size, SHELL_FUNC_LIST_MAX_SIZE);
	int i;
	for(i = 0 ; i < shell_func_list_size ; i++) {
		printf("%s : %s\r\n", shell_func_list[i].cmd, shell_func_list[i].description);
	}
	return SHELL_ERR_NONE;
}


/**
    \note UART FIFO is enabled; without FIFO some character loss from RX was observed
 */
int shell_init(UART_HandleTypeDef* huart) {
	shell_huart = huart;

	//uart_write(starting, strlen(starting));
	uart_write(prompt, strlen(prompt));

	if (shell_add("help", sh_help, "Show shell commands help") != 0) {
        printf("\n\nFailed to add shell 'help' command! List full?\n\n");
	}

	return 0;
}

/**
 * shell_add : add a command to the shell
 * @param cmd string related to the command
 * @param pfunc command function reference
 * @param description of the function displayed on shell help menu
 * @return 0 if number of commands is valid
 */
unsigned char shell_add(const char * cmd, enum shell_error (* pfunc)(int argc, char ** argv), const char * description) {
	if (shell_func_list_size < SHELL_FUNC_LIST_MAX_SIZE) {
		shell_func_list[shell_func_list_size].cmd = cmd;
		shell_func_list[shell_func_list_size].func = pfunc;
		shell_func_list[shell_func_list_size].description = description;
		shell_func_list_size++;
		return 0;
	}

	return -1;
}

/** Process the received character
 */
void shell_on_rx_char(const char shell_character) {
	switch (shell_character) {
	case '\r':
    case '\n':
		// ENTER key pressed
		printf("\r\n");
		if (pos < SHELL_BUFFER_SIZE) {
            buf[pos++] = 0;
            shell_exec(buf);
		} else {
            printf("Command too long, ignored!\r\n");
		}
        pos = 0;
        uart_write(prompt,strlen(prompt));
		break;

	case '\b':
		// DELETE key pressed
		if (pos > 0) {
			pos--;
			uart_write(backspace, 3);
		}
		break;

	default:
		if (pos < SHELL_BUFFER_SIZE) {
			uart_write(&shell_character, 1);    // echo
			buf[pos++] = shell_character;
		}
	}
}

/**
 * shell_exec : search and execute the command
 * @param cmd command to process
 */
uint8_t shell_exec(char * cmd) {
    if (cmd[0] == '\0') {
        return -1;
    }

	int argc;
	char * argv[SHELL_ARGC_MAX];
	char *p;

	// Separating header and arguments
	char header[SHELL_CMD_MAX_SIZE] = "";
	int h = 0;

	while(cmd[h] != ' ' && h < SHELL_CMD_MAX_SIZE){
		header[h] = cmd[h];
		h++;
	}
	header[h] = '\0';

	// Searching for the command and parameters
	for(int i = 0 ; i < shell_func_list_size ; i++) {
		if (!strcmp(shell_func_list[i].cmd, header)) {
			argc = 1;
			argv[0] = cmd;

			for(p = cmd ; *p != '\0' && argc < SHELL_ARGC_MAX ; p++){
				if(*p == ' ') {
					*p = '\0';
					argv[argc++] = p+1;
				}
			}

			enum shell_error err = shell_func_list[i].func(argc, argv);
			if (err != SHELL_ERR_NONE) {
    			printf("%s error: %s\r\n", cmd, get_shell_err_desc(err));
			}
			return err;
		}
	}
	printf("%s: command not found\r\n", cmd);
	return -1;
}
