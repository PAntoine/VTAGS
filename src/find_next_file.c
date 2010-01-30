/*--------------------------------------------------------------------------
---                                                           FIND NEXT FILE
---                                                        vtags - VHDL TAGS 
----------------------------------------------------------------------------
--- This function will get the next file. If it needs to it will walk down
--- to the next directory. As with find first file it will find the
--- 'rightmost' file that it can from where it is.
---
--- WARNING:
--- There are two versions of the function here. One for WIN32 and the
--- other for POSIX. This is the easiest way to code this, and dirs are
--- all OS specific. Sorry for the conditional compiled code.
---
--- Author:	Peter Antoine		Date: 2nd Sep 2004
----------------------------------------------------------------------------
---                                         Copyright (c) 2004 Peter Antoine
----------------------------------------------------------------------------*/

#include "dir_stuff.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "vtags.h"

extern	char		full_path[NAME_MAX * 2];
#ifdef __unix__
#define	FAILED_TO_OPEN_DIR	0

/*------------------------------------------------------------*
 * POSIX Version
 *
 * This uses opendir, et al. to negotiate the dir tree.
 *
 * The rather strange way I neg. the tree is due to the presents
 * of symbolic links. These are now partially transversed.
 *------------------------------------------------------------*/
int		find_next_file(DIR_ENTRY* dir_tree,int* curr_dir,char** extention_list,char* filename,int recursive)
{
	int		found  = 0,count;
	int		infile = -1;
	char	cur_dirname[256],*dot;
	
	struct	dirent*	d_ent;
    struct	stat	dir_stat;
	
	if (*curr_dir < MAX_DEPTH && *curr_dir > -1) 
	{
		do
		{
			if ((d_ent = readdir(dir_tree[*curr_dir].dir)) != NULL)
			{
				/* check to see if its a directory */
				if (d_ent->d_name[0] != '.')
				{
					/* NOTE: This is BAD code. It ignores ALL hidden files */
					if (stat(d_ent->d_name, &dir_stat) != -1)
					{
						if (S_ISDIR(dir_stat.st_mode))
						{
							/* dont go into the directory if the 'recursive' flag is not set */
							if (recursive)
							{
								/* lets add the name to the full path */
								*curr_dir = *curr_dir + 1;
								dir_tree[*curr_dir].dir = opendir(d_ent->d_name);

								if (dir_tree[*curr_dir].dir == NULL)
								{
									/* failed to open the directory */
									*curr_dir = *curr_dir - 1;
								}else{
									dir_tree[*curr_dir].name = dir_tree[*curr_dir-1].name + dir_tree[*curr_dir-1].size;
									dir_tree[*curr_dir].size = cpystrlen(dir_tree[*curr_dir].name,d_ent->d_name,NAME_MAX);
									dir_tree[*curr_dir].name[dir_tree[*curr_dir].size++] = DIR_DELIMETER;
									dir_tree[*curr_dir].name[dir_tree[*curr_dir].size] = '\0';

									/* lets walk into this directory */
									chdir(d_ent->d_name);
								}
							}
						}else{
							/* found 'rightmost' file -- position back to
							 * the file we have just read.
							 */
							if ((dot = strrchr(d_ent->d_name,'.')) != NULL)
							{
								count = 0;

								/* we have an extension-list find a file */
								while(extention_list[count] != NULL && !found)
								{
									if (strcmp(dot,extention_list[count]) == 0)
									{
										/* found a file with one of the extension */
										if ((infile = open(d_ent->d_name,READ_FILE_STATUS)) != -1)
										{
											fillBuffer(infile);
											found = 1;
										}
									}

									count++;
								}
							}
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
		while(!found && *curr_dir > -1 && *curr_dir < MAX_DEPTH);
	}

	/* report the file name */
	if (found)
		strcpy(filename,d_ent->d_name);

	return infile;
}
#endif


#ifdef _WIN32

extern WIN32_FIND_DATA FileData; 

static int			defered_change = 0;
static unsigned int	hold_size = 0;
static char			hold_path[NAME_MAX];

/*------------------------------------------------------------
 * strnlen
 *
 * Version of the posix function that returns a length of a
 * string. But, It will also stop after max_size bytes. This
 * stops buffer overruns and spurious crashing.
 *------------------------------------------------------------*/
unsigned int	strnlen(const unsigned char* buffer,unsigned int max_size)
{
	unsigned int	size = 0;

	while(buffer[size] != '\0' && size < max_size)
		size++;
	
	return size;
}

/*------------------------------------------------------------*
 * WIN32 Version
 *
 * This uses FindNextFile, et al. to negotiate the dir tree.
 *
 * This works differently from the POSIX version, as there is
 * no seek on the windows variant of the dir functions, I will
 * do the seek on the 'back end' of the search.
 *
 * I wish I had made a note of how differed change worked when
 * I wrote it. I think that if the last file is a good one, I
 * need to change the directory level after I have handled the
 * file. I think this because I am one file behind, the get
 * next mechanism that Win32 uses.
 *
 * Windows is a pain in the ar*e.
 *------------------------------------------------------------*/
int		find_next_file(DIR_ENTRY* dir_tree,int* curr_dir,char** extention_list,char* filename,int recursive)
{
	int		count,infile = -1, found = 0;
	char	*dot;
	
	/* handle the case when the last file is a file that we are
	 * looking for.
	 */
	if (defered_change != 0)
	{
		if (defered_change == -1)
		{
			dir_tree[*curr_dir].name[0] = '\0';
		}else{
			dir_tree[(*curr_dir)+1].name = dir_tree[*curr_dir].name + dir_tree[*curr_dir].size;
			dir_tree[(*curr_dir)+1].size = hold_size;

			memcpy(dir_tree[(*curr_dir)+1].name,hold_path,hold_size);
					
			dir_tree[(*curr_dir)+1].name[dir_tree[(*curr_dir)+1].size++] = DIR_DELIMETER;
			dir_tree[(*curr_dir)+1].name[dir_tree[(*curr_dir)+1].size] = '\0';
		}

		(*curr_dir) += defered_change;
		defered_change = 0;
	}
	
	if (*curr_dir < MAX_DEPTH && *curr_dir > -1) 
	{
		do
		{
			/* check to see if current file in directory is one we are looking for */
			if (dir_tree[*curr_dir].dir != INVALID_HANDLE_VALUE)
			{
				count = 0;
				
				if ((dot = strrchr(FileData.cFileName,'.')) != NULL)
				{
					while(extention_list[count] != NULL && !found)
					{
						if (strcmp(dot,extention_list[count]) == 0)
						{
							/* found a file with one of the extention */
							if ((infile = open(FileData.cFileName,READ_FILE_STATUS)) != -1)
							{
								strncpy(filename,FileData.cFileName,NAME_MAX);
								fillBuffer(infile);
								found = 1;
							}
						}
						count++;
					}
				}
			}

			/* now get the next file ready */
			if (!FindNextFile(dir_tree[*curr_dir].dir, &FileData)) 
			{
				if (GetLastError() == ERROR_NO_MORE_FILES) 
				{
					FindClose(dir_tree[*curr_dir].dir);
					chdir("..");			/*BUG FIX: unix fix for symbolic links broke WIN 32 version */
				
					/* we dont want to change the name before we have used it */
					if (found)
					{
						defered_change = -1;
					}else{
						dir_tree[*curr_dir].name[0] = '\0';
						(*curr_dir)--;
					}
				}
			}else{

				if (FileData.dwFileAttributes | FILE_ATTRIBUTE_DIRECTORY)
				{
					if (FileData.cFileName[0] != '.')
					{
						if (recursive)
						{
							/* we have a directory, lets move into it */
							if(chdir(FileData.cFileName) != -1)
							{
								/* BUG FIX: Oh my god this code was poor! now does what it supposed to */
								if (!found)
								{
									/* change the directory */
									dir_tree[(*curr_dir)+1].name = dir_tree[*curr_dir].name + dir_tree[*curr_dir].size;
									dir_tree[(*curr_dir)+1].size = cpystrlen(dir_tree[(*curr_dir)+1].name,FileData.cFileName,NAME_MAX);
									dir_tree[(*curr_dir)+1].name[dir_tree[(*curr_dir)+1].size++] = DIR_DELIMETER;
									dir_tree[(*curr_dir)+1].name[dir_tree[(*curr_dir)+1].size] = '\0';
								
									dir_tree[(*curr_dir)+1].dir = FindFirstFile("*.*",&FileData);
									(*curr_dir)++;
								}else{
									/* we want to defer changing directory until after we have used it */
									hold_size = strnlen(FileData.cFileName,NAME_MAX);
									memcpy(hold_path,FileData.cFileName,hold_size);
									defered_change = 1;
								
									dir_tree[(*curr_dir)+1].dir = FindFirstFile("*.*",&FileData);
								}
							}
						}
					}
				}
			}
		}
		while(!found && *curr_dir > -1 && *curr_dir < MAX_DEPTH);
	}

	/* Known fault:
	 * If the last but one file is end of the directory and about to go up a level, then
	 * it will do the last file twice.
	 */
	
	return infile;
}

#endif
