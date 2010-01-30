/*--------------------------------------------------------------------------
---                                                             VTAGS EXTRAS
---                                                        vtags - VHDL TAGS 
----------------------------------------------------------------------------
--- This file holds a set of functions that are too small for files of there
--- own and are used by the other functions of the application.
---
---
--- Author:	Peter Antoine		Date: 1st Sep 2004
----------------------------------------------------------------------------
---                                         Copyright (c) 2004 Peter Antoine
----------------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>

#include "vtags.h"
#include "dir_stuff.h"

extern const char white_space[256];
extern const char terminator[256];
extern const char list_term[256];

/*------------------------------------------------------------*
 * GetListFromParam
 *
 * This function will build a list from a single comma sep.
 * variable from the command line. This will malloc the list
 * of paths and generate a simple array.
 *
 * The last element of the array should be NULL.
 *------------------------------------------------------------*/
char**	GetListFromParam(char* parameter)
{
	int		last,comma_count = 0;
	int		i = 0, j, items = 0;
	char**	array;
	
	/* first count the number of commas */
	while(parameter[i] != '\0')
	{
		if (parameter[i++] == ',')
			comma_count++;
	}

	if (comma_count > 0)
	{
		array = (char**) calloc(comma_count+2,sizeof(char*));
		last = 0;
		
		/* get all the elements from the list */
		for (j=0;j < i;j++)
		{
			if (parameter[j] == ',')
			{
				array[items] = (char*) malloc(j-last);
				memcpy(array[items],&parameter[last],j-last);
				array[items][j-last] = '\0';
				items++;
				last = j + 1;
			}
		}

		/* catch the last element */
		if (j-last > 0)
		{
			array[items] = (char*) malloc(j-last);
			memcpy(array[items],&parameter[last],j-last);
			array[items][j-last] = '\0';
		}
	}else{
		j = strlen(parameter);

		if (j > 0)
		{
			array = (char**) calloc(2,sizeof(char*));
			array[0] = (char*) malloc(j+1);
			memcpy(array[0],parameter,j+1);
		}
	}

	return array;
}

/*------------------------------------------------------------*
 * ClearList
 *
 * This will release the memory allocated to the list.
 *------------------------------------------------------------*/
void	ClearList(char** list)
{
	int	count = 0;
	
	while(list[count] != NULL)
		free(list[count++]);

	free(list);
}

/*------------------------------------------------------------*
 * cpystrlen
 *
 * This function will copy a string from s2, to s1. It will
 * return the number of bytes that has been copied. It will
 * copy a max of n bytes.
 *------------------------------------------------------------*/
int	cpystrlen(char *s1, char *s2, int n)
{
	int count;
	
	for (count=0;count<n && s2[count] != '\0';count++)
	{
		s1[count] = s2[count];
	}

	return count;
}


/*------------------------------------------------------------*
 * makefilename
 * 
 * This function will build the file path of the current file.
 * It will also return the size of the string (excluding the
 * trailing 0).
 *
 * The filename it returns will be a max of 256 chars. The
 * buffer passed in must be at least this long.
 *------------------------------------------------------------*/
int	makefilename(char* path_file_name,char* filename,int current_dir,DIR_ENTRY* dir_tree)
{
	int		filename_length = 0,temp,count;

	temp = strlen(filename);
	memcpy(dir_tree[current_dir].name+dir_tree[current_dir].size,filename,temp);

	filename_length += temp;

	path_file_name[filename_length] = '\0';

	return filename_length;
}

/*------------------------------------------------------------*
 * clearfilename
 * 
 * This function will remove the filename from the end if the
 * full path.
 *------------------------------------------------------------*/
void	clearfilename(char* filename,int current_dir,DIR_ENTRY* dir_tree)
{
	dir_tree[current_dir].name[dir_tree[current_dir].size] = '\0';
}

/*------------------------------------------------------------*
 * get_next_list_entry
 * 
 * This function will find the next list entry from a comma
 * separated list. Essentially it will search the string for
 * the next non-valid name char and that will be the end of the
 * token. If that char is a comma it will return the value 
 * after the comma as the start of the list. Else, it will
 * return NULL as this must be the end of the list. It will
 * do white space removal at the start of the list.
 *------------------------------------------------------------*/
char*	get_next_list_entry(char* list,char** list_token,int* token_size)
{
	char*	current = list;
	char*	next_list = NULL;

	*token_size = 0;

	/* remove the white space from the start */
	while (white_space[*current])
	{
		current++;
	}

	/* we have the start of the token */
	*list_token = current;

	if (*current != ',')
	{
		/* now lets find the end */
		while(!terminator[*current])
		{
			current++;
			*token_size = *token_size + 1;
		}

		/* remove the white space from after the token */
		while (white_space[*current])
		{
			current++;
		}

		/* test for end of list */
		if (*current == ',')
		{
			next_list = current + 1;
		}
	}

	return next_list;
}

