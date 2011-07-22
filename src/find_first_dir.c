/*--------------------------------------------------------------------------
---                                                     FIND FIRST DIRECTORY 
---                                                        vtags - VHDL TAGS 
----------------------------------------------------------------------------
--- This function will walk down a directory tree starting at the given
--- directory and will find the 'rightmost' branch. It will keep track of
--- all the directories that it has walked through, so that the tree can be
--- walked recursively.
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

extern char	full_path[];

#ifdef __unix__

#define	FAILED_TO_OPEN_DIR	0

/*------------------------------------------------------------*
 * POSIX Version
 *
 * This uses opendir, et al. to negotiate the dir tree.
 *------------------------------------------------------------*/
int		find_first_dir(char* start_path,DIR_ENTRY* dir_tree,int* curr_dir,int recursive)
{
	int		found = 0,failed = 0;
	long	hold;
	
	struct	dirent*	d_ent;
    struct	stat	dir_stat;
	
	if (start_path == NULL || *curr_dir > MAX_DEPTH)
		failed = 1;
	
	*curr_dir = 0;
	
	/* set the base of the search */
    dir_tree[*curr_dir].dir = opendir(start_path);
	dir_tree[*curr_dir].size = cpystrlen(dir_tree[*curr_dir].name,start_path,NAME_MAX);

	if (dir_tree[*curr_dir].name[dir_tree[*curr_dir].size-1] != DIR_DELIMETER)
	{
		dir_tree[*curr_dir].name[dir_tree[*curr_dir].size++] = DIR_DELIMETER;
	}
	
	dir_tree[*curr_dir].name[dir_tree[*curr_dir].size] = '\0';
				
	if (dir_tree[*curr_dir].dir != FAILED_TO_OPEN_DIR)
		chdir(start_path);

	while(dir_tree[*curr_dir].dir != FAILED_TO_OPEN_DIR && *curr_dir >= 0 && !found)
	{
		hold = telldir(dir_tree[*curr_dir].dir);

		if ((d_ent = readdir(dir_tree[*curr_dir].dir)) != NULL)
		{
			/* we have some thing in the dir */
			if (d_ent->d_name[0] != '.')
			{
				/* NOTE: This is BAD code. It ignores ALL hidden files/dir's */
				if (stat(d_ent->d_name, &dir_stat) != -1)
				{
					if (S_ISDIR(dir_stat.st_mode))
					{
						if (recursive)
						{
							/* lets add the name to the full path */
							*curr_dir = *curr_dir + 1;
							dir_tree[*curr_dir].name = dir_tree[*curr_dir-1].name + dir_tree[*curr_dir-1].size;
							dir_tree[*curr_dir].size = cpystrlen(dir_tree[*curr_dir].name,d_ent->d_name,NAME_MAX);
							dir_tree[*curr_dir].name[dir_tree[*curr_dir].size++] = DIR_DELIMETER;
							dir_tree[*curr_dir].name[dir_tree[*curr_dir].size] = '\0';

							/* open the dir */
							dir_tree[*curr_dir].dir = opendir(d_ent->d_name);

							/* lets walk into this directory */
							chdir(d_ent->d_name);
						}
					}else{
						/* found 'rightmost' file -- position back to
						 * the file we have just read.
						 */
						seekdir(dir_tree[*curr_dir].dir,hold);
						found = 1;
					}
				}
			}
		}else{
			/* we are now at the end of a (empty?) dir
			 * let go back up a level
			 */
			dir_tree[*curr_dir].name[0] = '\0';
			chdir(full_path);

			closedir(dir_tree[*curr_dir].dir);
			*curr_dir = *curr_dir - 1;
		}
	}

	return found;
}

#endif

#ifdef _WIN32
#include <windows.h>
	
WIN32_FIND_DATA FileData; 

/*------------------------------------------------------------*
 * WIN32 Version
 *
 * This uses FindFirstFile, et al. to negoitiate the dir tree.
 *------------------------------------------------------------*/

int		find_first_dir(char* start_path, DIR_ENTRY* dir_tree, int* curr_dir,int recursive)
{
	int	found = 0,failed = 0;
	 
	*curr_dir = 0;

	if (chdir(start_path) != -1)
	{
		dir_tree[*curr_dir].dir = FindFirstFile("*.*", &FileData);
		
		dir_tree[*curr_dir].size = cpystrlen(dir_tree[*curr_dir].name,start_path,NAME_MAX);
		dir_tree[*curr_dir].name[dir_tree[*curr_dir].size++] = DIR_DELIMETER;
		dir_tree[*curr_dir].name[dir_tree[*curr_dir].size] = '\0';

		if (dir_tree[*curr_dir].dir == INVALID_HANDLE_VALUE) 
		{ 
			printf("Failed\n");
			failed = 1;
		}
		 
		while (!found && !failed)
		{
			if (!FindNextFile(dir_tree[*curr_dir].dir, &FileData)) 
			{
				if (GetLastError() == ERROR_NO_MORE_FILES) 
				{ 
					dir_tree[*curr_dir].name[0] = '\0';
					chdir(full_path);

					FindClose(dir_tree[*curr_dir].dir);
					(*curr_dir)--;

					if (*curr_dir < 0)
					{
						failed = 1;
					}
				} 
			}else{
				if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if (FileData.cFileName[0] != '.' && recursive)
					{
						/* we have a directory, lets move into it */
						if(chdir(FileData.cFileName) != -1)
						{
							(*curr_dir)++;

							dir_tree[*curr_dir].name = dir_tree[*curr_dir-1].name + dir_tree[*curr_dir-1].size;
							dir_tree[*curr_dir].size = cpystrlen(dir_tree[*curr_dir].name,FileData.cFileName,NAME_MAX);
							dir_tree[*curr_dir].name[dir_tree[*curr_dir].size++] = DIR_DELIMETER;
							dir_tree[*curr_dir].name[dir_tree[*curr_dir].size] = '\0';
							
							dir_tree[*curr_dir].dir = FindFirstFile("*.*",&FileData);
						}
					}
				}else{
					/* we have found the "rightmost" file */
					found = 1;
				}
			}
		}
	}
		
	return found;
}

#endif

