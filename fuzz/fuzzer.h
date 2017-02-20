#ifndef FUZZER_H
#define FUZZER_H

enum CmdStatus {
	CMD_FAIL = false,
	CMD_OK = true,
	CMD_ERR,          /* syntax error */
	CMD_QUIT,         /* quit, accept no further commands */
};

static const char *cmd_status_msg[] = {
	[CMD_FAIL] = "Fail\n",
	[CMD_OK]   = "",
	[CMD_ERR]  = "Syntax error\n",
	[CMD_QUIT] = "Bye\n",
};

#endif
