/*--------------------------------------------------------------------------
---                                                                     main
---                                                        vtags - VHDL TAGS 
----------------------------------------------------------------------------
--- This program will generate a tags file that can be read by VIM. It will
--- parse a source tree and try to generate a ctags file. It is designed to
--- work with VHDL'87 which is what I use.
---
--- Usage:
--- 
--- vtags [-r] [-q] [-e<extension list>] [-f <file_path>] <path>
---
--- -R       Recursive. 
---          This will cause the program to search the sub-directories as
---          well.
---
--- -e       Extension List.
---		     This is a comma separated list of file types that will be 
---          searched. The default is vhdl,vhd. These will be searched both
---          as upper and lower case.
---
--- -f       Tags File.
---          The TAGS file to generate. If this is not present it will
---          default to "./tags". If name = '-' then its written to stdout.
---
--- -u		 Unsorted.	
---          This will generate an unsorted tags file.
---
--- -l       Format level.
---          Level 1 not extensions, just the basic tags.
---          Level 2 add the extensions (;") stuff.
---
--- <path>   Search Path List
---          This is comma separated list of directories that are to be 
---          searched for the source files to be parsed. If the -r flag is
---          present then all the sub-directories are searched.
---
---
--- Author:	Peter Antoine		Date: 10 Sep 2004
----------------------------------------------------------------------------
---                                    Copyright (c) 2004-2008 Peter Antoine
---                                      Released under the Artistic Licence
----------------------------------------------------------------------------
{{{ Revision History
--- Version   Author Date        Changes
--- -------   ------ ----------  -------------------------------------------
--- 0.1       PA     10.09.2004  Initial revision
--- 0.2       PA     15.09.2004  Win32 and Unix version. Plus bug fixes.
---                              Also supports "ops", but not completely.
--- 0.3       PA     20.10.2004  Added Format 2 tags. Added -R as an alias
---                              to -r to me more compatible. Also added
---                              '-' filename to stdout, trying to get vtags
---                              to work with Tlist.
--- 0.4       PA     22.10.2004  Now handles single files, also generates 
---                              the line number for each file. Also have
---                              fixed the recursive stuff.
--- 0.5       PA     23.10.2004  Now groups items by architecture, so it
---                              works like a C++ class structure.
--- 0.6       PA     12.11.2004  BUG FIXES: fixed sort problems with losing
---                              losing the first element of the sort and
---                              anything that is in bin[19]. Also 'begin'
---                              where turning up in the tags file. Also
---                              added the groupings for packages and for
---                              for function and procedures. Also did not
---                              transverse dirs that has symbolic links
---                              in them, now it does. It wont transverse
---                              the whole symbolic tree, just the subs
---                              below the current.
--- 0.7       PA     16.03.2005  Added the default path is current path
---                              and makes '.' path work correctly.
---                              Fixed bugs in the Win32 version which
---                              have crept in. One known bug is still
---                              in Win32 version. If it fails to go up
---                              a level on the last file, it researches
---                              the previous file. 
--- 0.8       PA     20.09.2007  BUG FIX: -- someone actually uses this!
---                              Not allowing for a signal,variable
---                              to define multiple symbols on one line.
---                              This has been changed to allow for this.
---                              Also removed some of the debug rubbish
---                              that was in the code.
--- 0.9       PA     15.12.2008  Fixed a couple of problem and added a
---                              nmake make file.
--- 0.10      PA     05.12.2009  BUG FIXES:
---                              Applied patches from Jun Ma to fix the
---                              the build. Also sorted out the temp
---                              file cross platform creation issues.
---                              Also fixed the very broken makefiles.
--- 0.11      PA     10.12.2010  BUG FIX: The find first directory 
                                 function had a problem with its name
								 creation. This would stop the code 
								 walking the tree properly.
-----------------------------------------------------------------------}}}*/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "errno.h"

#include "vtags.h"
#include "dir_stuff.h"

#define	__VTAGS_VERSION__	"0.11"

typedef enum{
		ERROR_OK,
		ERROR_FILENAME_P_WRONG,
		ERROR_EXTENSION_P_WRONG,
		ERROR_UNKNOWN_PARAMETER,
		ERROR_BAD_PATH_LIST,
		ERROR_FILENAME_WRONG,
		ERROR_INVALID_TAGS_FILE,
		ERROR_INVALID_DIRECTORY,
		ERROR_NOT_ENOUGH_PARAMETERS,
		ERROR_FAILED_TO_OPEN_TEMPFILE,
		ERROR_LEVEL_VALUE_WRONG	
} ERROR_CODES;

const char*	gErrorCodes[] = { "Ok.",
							"Invalid filename parameter.",
							"Invalid extension parameter.",
							"Unknown parameter.",
							"Bad Path List supplied.",
							"Filename not found.",
							"Could not open TAGS file.",
							"Invalid directory.",
							"Not enough parameters.",
							"Could not create/open temporary file.",
							"Invalid level - must be 1 or 2."};

const 	char	file_header1[]=	"!_TAG_FILE_FORMAT\t";
const	char	file_header2[]= "\x0a!_TAG_FILE_SORTED\t";
const	char	file_header3[]=	"\t/0=unsorted, 1=sorted, 2=foldcase/\x0a"
								"!_TAG_PROGRAM_AUTHOR\tPeter Antoine\t/vtags@peterantoine.me.uk/\x0a"
								"!_TAG_PROGRAM_NAME\t\tvtags\t/VHDL Tags/\x0a"
								"!_TAG_PROGRAM_URL\t\twww.peterantoine.me.uk\\vtags.html\t/home page/\x0a"
								"!_TAG_PROGRAM_VERSION\t" __VTAGS_VERSION__ "\t//\x0a";

unsigned char	check_char[3][256];

/*------------------------------------------------------------*
 * reserved word list
 *
 * be careful changing this list, CheckReserved uses a trick that
 * may not work if you add a new word to the list.
 *------------------------------------------------------------*/
const	char*	reserved[] = {	"end","entity","architecture","component",
								"signal","package","function","procedure",
								"constant","subtype","type","variable","begin","if","case"};

#define	number_of_reserved  ((sizeof(reserved)/sizeof(char*)))
const	int		num_reserved = number_of_reserved;

const	char	res_type[] = {	' ','e','a','c','s','p','f','P','d','t','t','v',' '};

int		reserved_size[number_of_reserved];


/*------------------------------------------------------------*
 * Search stuff
 *------------------------------------------------------------*/
const	char	lowest_name[]  = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
const	char	highest_name[] = "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"; 

const	char	default_level[] = "2";

char*	path_default[2] = {".",NULL};
char*	extn_default[3] = {".vhdl",".vhd",NULL};

/* global definition of tag_table */
/*------------------------------------------------------------*
 * GLOBAL Definitions
 *------------------------------------------------------------*/
int			in_package = 0;
int			in_funcProc = 0;
int			in_architecture = 0;

int			arch_token_size = 0;
int			pack_token_size = 0;
int			funcProc_token_size = 0;

char		pack_token[MAX_TOKEN_LENGTH];
char		arch_token[MAX_TOKEN_LENGTH];
char		funcProc_token[MAX_TOKEN_LENGTH];

char		full_path[NAME_MAX * 2];		/* cant break this! */

TAG_TABLE	tag_table;

int	main(int argc, char** argv)
{
	int			position,line_size,filesize= 0,not_sorted = 0,bytesread;
	int			count,dir_count=0,file_name_size;
	int			start,failed = ERROR_OK,found;
	int			recursive = 0, quiet = 0,current_dir = 0;
	int			outfile,infile,tempfile,filename_len;
	char*		level = (char*)&default_level,*dot;
	char**		path_list = path_default;
	char**		extension_list = extn_default;
	char		token_string[MAX_TAG_STRING_SIZE],line_stuff[15];
	char		tempname[NAME_MAX];
	char		filename[NAME_MAX];
	char		path_file_name[256];
	char		run_dir[NAME_MAX];
	DIR_ENTRY	dir_tree[MAX_DEPTH+1];
	
	memcpy(filename,"tags",5);
	memcpy(tempname,"./vtags_XXXXXX",sizeof("./vtags_XXXXXX"));

	/* need to do this before we start */
	get_current_dir(run_dir);

	/* first decode the parameters */
	if (argc > 1)
	{
		start = 1;

		while (start < argc && !failed)
		{
			if (argv[start][0] == '-')
			{
				switch(argv[start][1])
				{
					case 'r':
					case 'R':	recursive = 1;						break;
					case 'u':	not_sorted = 1;						break;	
					case 'q':	quiet = 1;							break;	
					case 'f':	if (argv[start][2] != '\0')
									strncpy(filename,&argv[start][2],256);

								else if (argv[start+1][0] != '-' || (argv[start+1][0] == '-' && argv[start+1][1] == '\0'))
									strncpy(filename,argv[++start],256);
								else
									failed = ERROR_FILENAME_P_WRONG;
								break;
								
					case 'e':	if (argv[start][2] != '\0')
									extension_list = GetListFromParam(&argv[start][2]);

								else if (argv[start+1][0] != '-')
									extension_list = GetListFromParam(argv[++start]);
								else
									extension_list = NULL;

								if (extension_list == NULL)
									failed = ERROR_EXTENSION_P_WRONG;
								break;

					case 'l':	if (argv[start][2] != '\0')
									level = &argv[start][2];
								else
									level = argv[++start];

								if (!((level[0] == '1' || level[0] == '2') && level[1] == '\0'))
									failed = ERROR_LEVEL_VALUE_WRONG;
								break;	
									
									
					default:	failed = ERROR_UNKNOWN_PARAMETER;
				}
			}else{
				/* must be the path */
				if (!(argv[start][0] == '.' && argv[start][1] == '\0'))
				{
					path_list = GetListFromParam(argv[start]);

					if (path_list == NULL)
						failed = ERROR_BAD_PATH_LIST;
				}
			}
				
			start++;
		}
	}else{
		failed = ERROR_NOT_ENOUGH_PARAMETERS;
	}
				
	if (!quiet)
	{
		printf( "\nVTAGS version " __VTAGS_VERSION__ " - Copyright (c) 2004 - 2010 Peter Antoine\n"
				"All Rights reserved -- Released under the conditions of the Artistic Licence\n\n");
	}
	
	if (!failed)
	{
		if (filename[0] == '-' && filename[1] == '\0')
			outfile = fileno(stdout);
		else if ((outfile = open(filename,WRITE_FILE_STATUS,WRITE_FILE_PERM)) == -1)
			failed = ERROR_INVALID_TAGS_FILE;
	}
	
	if (!failed)
	{	
		make_temp_filename(tempname);

		if ((tempfile = open(tempname,RDWR_FILE_STATUS,WRITE_FILE_PERM)) == -1)
			failed = ERROR_FAILED_TO_OPEN_TEMPFILE;
	}
		
	if (failed || extension_list == NULL)
	{
		printf(	"Failed: %s\n"
				"Usage: %s [-r] [-q] [-l<level>] [-e<extension list>] [-f <file_path>] <path>\n"
				" -R            Recursive.  Will look in the sub-directories. (for compatibility) \n"
				" -r            Recursive.  Will look in the sub-directories. \n"
				" -q            Quiet.      Will not print the version information.\n"
				" -u            Unsorted.   Will generate an unsorted tags file\n"
				" -l<level>     Level.      The TAGS file format level 1 or 2 [2] \n"
				" -e<list>      Extensions. The comma separated list of file extensions to search\n"
				" -f<file>      Tags File.  The TAGS file that will be generated.\n"
				" <path_list>   Path List.  The comma separated list of directories/files to be searched\n",gErrorCodes[failed],argv[0]);
		exit(0-failed);
	}
	
	/*-----------------------------------------*
	 * Initials the system.
	 *-----------------------------------------*/

	dir_tree[0].name = full_path;
	
	/* calculate the size of the reserved words */
	for (count=0;count<num_reserved;count++)
		reserved_size[count] = strlen(reserved[count]);

	/* setting up pointer to point to the correct place */
	tag_table.last_tag_block  = &tag_table.tag_list;
	tag_table.last_name_block = &tag_table.tag_names;
	
	tag_table.tag_names.next = NULL;
	tag_table.last_tag_block->next = NULL;	
	tag_table.last_tag = 2;
	
	/* starting bin entry */
	tag_table.bins[0].token = lowest_name;
	tag_table.bins[0].size  = 0;
	tag_table.bins[0].startPoint = &tag_table.tag_list.entry[TAG_FRONT_ELEMENT];
	
	/* all other bin pointers point to the last element */
	for (count=1;count<NUM_OF_BINS;count++)
	{
		tag_table.bins[count].token = highest_name;
		tag_table.bins[count].size  = 0;
		tag_table.bins[count].startPoint = &tag_table.tag_list.entry[TAG_END_ELEMENT];
	}

	/* the first and last elements should point at each other to start */
	tag_table.tag_list.entry[TAG_END_ELEMENT].line_size = 0;
	tag_table.tag_list.entry[TAG_END_ELEMENT].file_position = 0;
	tag_table.tag_list.entry[TAG_END_ELEMENT].token = highest_name;
	tag_table.tag_list.entry[TAG_END_ELEMENT].prev  = &tag_table.tag_list.entry[TAG_FRONT_ELEMENT];
	tag_table.tag_list.entry[TAG_END_ELEMENT].next	= NULL;
	
	tag_table.tag_list.entry[TAG_FRONT_ELEMENT].line_size = 0;
	tag_table.tag_list.entry[TAG_FRONT_ELEMENT].file_position = 0;
	tag_table.tag_list.entry[TAG_FRONT_ELEMENT].token	= lowest_name;
	tag_table.tag_list.entry[TAG_FRONT_ELEMENT].prev	= NULL;
	tag_table.tag_list.entry[TAG_FRONT_ELEMENT].next	= &tag_table.tag_list.entry[TAG_END_ELEMENT];
	
	/*-----------------------------------------*
	 * All parameters are validated, we can now
	 * do the work.
	 *-----------------------------------------*/

	while(path_list[dir_count] != NULL)
	{
		/* open the requested directory */
		if (is_dir(path_list[dir_count]))
		{
			if (find_first_dir(path_list[dir_count],dir_tree,&current_dir,recursive))
			{
				while((infile = find_next_file(dir_tree,&current_dir,extension_list,filename,recursive)) != -1)
				{
					/* make the current filename */
					filename_len = strlen(filename);
					memcpy(dir_tree[current_dir].name+dir_tree[current_dir].size,filename,filename_len+1);
					file_name_size = strlen(full_path);
		
					
					make_tags(full_path,file_name_size,infile,tempfile,level,&filesize);
					
					/* clear the filename from the path */
					dir_tree[current_dir].name[dir_tree[current_dir].size] = '\0';	
					
					close(infile);
				}
			}
		}else{
			/* open file */
			infile = -1;
			
			if ((dot = strrchr(path_list[dir_count],'.')) != NULL)
			{
				count = 0;
								
				/* we have an extension-list find a file 
				 * not the worlds greatest solution, but it will work */
				while(extension_list[count] != NULL && infile == -1)
				{
					if (strcmp(dot,extension_list[count]) == 0)
					{
						if ((infile = open(path_list[dir_count],READ_FILE_STATUS)) != -1)
							fillBuffer(infile);
					}

					count++;
				}
			}

			make_tags(path_list[dir_count],strlen(path_list[dir_count]),infile,tempfile,level,&filesize);

			close(infile);
		}

		dir_count++;
	}

	/*-----------------------------------------*
	 * Now convert the intermediate file to
	 * a sorted tags file.
	 *-----------------------------------------*/
	
	/* write the TAGS file header */
	
	write(outfile,file_header1,sizeof(file_header1)-1);
	write(outfile,level,1);
	write(outfile,file_header2,sizeof(file_header2)-1);
	if (not_sorted)
		write(outfile,"0",1);
	else
		write(outfile,"1",1);
	write(outfile,file_header3,sizeof(file_header3)-1);

	if (not_sorted)
	{
		/* if not sorted then just add a header to the temp file */
		lseek(tempfile,0,SEEK_SET);

		do{
			bytesread = read(tempfile,token_string,MAX_TAG_STRING_SIZE);

			if (bytesread > 0)
				write(outfile,token_string,bytesread);
		}
		while (bytesread == MAX_TAG_STRING_SIZE);

	} else {
		/* start searching from the front of the linked list */
		tag_table.next_item = &tag_table.tag_list.entry[TAG_FRONT_ELEMENT];
		
		while(get_index_item(&position,&line_size))
		{
			lseek(tempfile,position,SEEK_SET);
			
			if (read(tempfile,token_string,line_size) != line_size)
				printf("Failed the read %d\n",position);
			write(outfile,token_string,line_size);
		}
		
		/* tidy up an leave the building */
		if (path_list != path_default)
			ClearList(path_list);
		
		if (extension_list != extn_default)
			ClearList(extension_list);
		
		release_index();
	}
	
	close(tempfile);
	close(outfile);

	change_directory(run_dir);

	remove(tempname);

	return 0;
}
/* vi: set fdm=marker : set sw=4 ts=4 : */
