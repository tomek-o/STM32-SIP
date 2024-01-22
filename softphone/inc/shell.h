#ifndef INC_SHELL_H_
#define INC_SHELL_H_

enum shell_error {
    SHELL_ERR_NONE = 0,
    SHELL_ERR_UNKNOWN_CMD,
    SHELL_ERR_INVALID_ARG_CNT,
    SHELL_ERR_INVALID_ARG_VAL,
};

unsigned char shell_add(const char * cmd, enum shell_error (* pfunc)(int argc, char ** argv), const char * description);

#endif /* INC_SHELL_H_ */
