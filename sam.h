#ifndef SAM_H
#define SAM_H

#include "vis.h"

enum SamError {
	SAM_ERR_OK,
	SAM_ERR_MEMORY,
	SAM_ERR_ADDRESS,
	SAM_ERR_NO_ADDRESS,
	SAM_ERR_UNMATCHED_BRACE,
	SAM_ERR_REGEX,
	SAM_ERR_TEXT,
	SAM_ERR_COMMAND,
	SAM_ERR_EXECUTE,
};

enum SamError sam_cmd(Vis *vis, const char *cmd);
const char *sam_error(enum SamError);

#endif
