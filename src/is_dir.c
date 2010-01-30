/*--------------------------------------------------------------------------
---                                                                   IS_DIR
---                                                        vtags - VHDL TAGS 
----------------------------------------------------------------------------
--- This function simply returns true if the named file is a directory if
--- not it returns false.
---
--- WARNING:
--- There are two versions of the function here. One for WIN32 and the
--- other for POSIX. This is the easiest way to code this, and dirs are
--- all OS specific. Sorry for the conditional compiled code.
---
--- Author:	Peter Antoine		Date: 1st Sep 2004
----------------------------------------------------------------------------
---                                         Copyright (c) 2004 Peter Antoine
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <sys/stat.h>
#include "dir_stuff.h"


#ifdef __unix__
int	is_dir(char* filename)
{
	int		result	= -1;
    struct	stat	file_stat;

	if (stat(filename, &file_stat) != -1)
	{
		if (S_ISDIR(file_stat.st_mode))
		{
			result = 1;
		}else{
			result = 0;
		}
	}

	return result;
}	
#endif

#ifdef _WIN32
#include <windows.h>
#define  INVALID_FILE_ATTRIBUTES   ((DWORD)-1) 							/* this seems to be broken in VC++ */

int	is_dir(char* filename)
{
	int		result = -1,attributes;

	attributes = GetFileAttributes(filename);

	if (attributes != INVALID_FILE_ATTRIBUTES)
	{	
		if (attributes & FILE_ATTRIBUTE_DIRECTORY)
			result = 1;
		else
			result = 0;
	}

	return result;
}
#endif
