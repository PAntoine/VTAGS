/*--------------------------------------------------------------------------
---                                                         DIR STUFF HEADER
---                                                        vtags - VHDL TAGS 
----------------------------------------------------------------------------
--- This header file holds the directory structures that are needed to 
--- the directory walks. The structures are designed to be as cross-
--- platform as they can be. The problem is that dir's are very OS specific
--- so in those places where stuff is plat spec I will use a macro and
--- define the plat spec macros at the top.
---
--- Author:	Peter Antoine		Date: 2nd Sep 2004
----------------------------------------------------------------------------
---                                         Copyright (c) 2004 Peter Antoine
----------------------------------------------------------------------------*/

#ifndef	__DIR_STUFF_H__
#define __DIR_STUFF_H__

#ifdef __unix__
	#include <limits.h>
	#include <dirent.h>

	typedef	DIR*	DIR_ITEM;

	#define HANDLE void*
	#define RDWR_FILE_STATUS	(O_TRUNC|O_CREAT|O_RDWR)
	#define WRITE_FILE_STATUS	(O_TRUNC|O_CREAT|O_WRONLY)
	#define WRITE_FILE_PERM		(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
	#define READ_FILE_STATUS	(O_RDONLY)
	#define DIR_DELIMETER		'/'

	#define make_temp_filename(a)	mkstemp(a)
	#define get_current_dir(a)		getcwd(a,PATH_MAX)
	#define change_directory(a)		chdir(a)	

	#ifndef	NAME_MAX
		#define	NAME_MAX	PATH_MAX			/* this is safest, I dont create any files using this size */
	#endif
	
#elif defined(_WIN32)
	#include <io.h>
	#include <direct.h>
	#include <windows.h>
	
	typedef	HANDLE	DIR_ITEM;
	
	#define RDWR_FILE_STATUS	(O_BINARY|O_TRUNC|O_CREAT|O_RDWR)
	#define WRITE_FILE_STATUS	(O_BINARY|O_TRUNC|O_CREAT|O_WRONLY)
	#define WRITE_FILE_PERM		(S_IWRITE|S_IREAD)
	#define READ_FILE_STATUS	(O_BINARY|O_RDONLY)
	#define DIR_DELIMETER		'\\'

	#define make_temp_filename(a)	GetTempFileName(".","vtags_",0,a)
	#define get_current_dir(a)		GetCurrentDirectory(MAX_PATH,a)
	#define	change_directory(a)		SetCurrentDirectory(a)

	#ifndef	NAME_MAX
		#define	NAME_MAX	MAX_PATH			/* this is safest, I dont create any files using this size */
	#endif
	
#else
	#error("Your OS is not supported. Sorry!");

#endif

/*----------------------------------------*
 * Common defines for all platforms
 *----------------------------------------*/

#define	MAX_DEPTH	(20)		/* max depth of directories to search */

typedef	struct
{
	DIR_ITEM	dir;
	int			size;
	char*		name;
	
} DIR_ENTRY;

#endif
