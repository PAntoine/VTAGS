/*--------------------------------------------------------------------------
---                                                                MAKE TAGS
---                                                        vtags - VHDL TAGS 
----------------------------------------------------------------------------
--- This function will make the tags for the next file.
--- 
--- Author:	Peter Antoine		Date: 22nd Oct 2004
----------------------------------------------------------------------------
---                                         Copyright (c) 2004 Peter Antoine
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>

#include "vtags.h"

#define	FUNC_NAME	"\tfunction:"
#define	PROC_NAME	"\tprocedure:"

const	char	pre_fix[]	= "\t/\\c";
const	char	mid_fix[]	= "\\s\\+";
const	char	post_fix[]	= "\\>/";
const	char	body_bit[]	= "body\\s\\+";
const	char	extension[]	= ";\"\t";
const	char	arch_stuff[]= "\tarchitecture:";
const	char	pack_stuff[]= "\tpackage:";
const	char	line_end[] 	= "\x0a";

const	char	*funcProc_stuff[] = {FUNC_NAME,PROC_NAME};
const	char	funcProc_size[] = {sizeof(FUNC_NAME),sizeof(PROC_NAME)};

extern char*	reserved[];
extern char		res_type[];
extern int		in_package;
extern int		in_funcProc;
extern char		pack_token[];
extern int		pack_token_size;
extern char		arch_token[];
extern int		num_reserved;
extern int		reserved_size[];
extern int		in_architecture;
extern char		funcProc_token[];
extern int		funcProc_token_size;
extern int		arch_token_size;

void	add_tag(char* 	path_file_name,
				int 	file_name_size,
				int 	tempfile,
				char* 	token,
				int 	token_size,
				char* 	search_token,
				int 	search_size,
				int 	found,
				char* 	level,
				int 	hasbody,
				int 	operator,
				int 	line,
				int* 	filesize)
{
	int		line_size,size;
	char	line_stuff[15];
	char	token_string[MAX_TAG_STRING_SIZE];

	if (token_size == 0)
		return;

	/* create the tag line in the file */
	line_size = 0;
	memcpy(token_string,token,token_size);
	token_string[line_size+token_size] = '\t';
	line_size += token_size+1;

	memcpy(token_string+line_size,path_file_name,file_name_size);
	line_size += file_name_size;

	memcpy(token_string+line_size,pre_fix,sizeof(pre_fix));
	line_size += sizeof(pre_fix) - 1;

	memcpy(token_string+line_size,reserved[found],reserved_size[found]);
	line_size += reserved_size[found];

	memcpy(token_string+line_size,mid_fix,sizeof(mid_fix));
	line_size += sizeof(mid_fix) - 1;

	if (hasbody)
	{
		memcpy(token_string+line_size,body_bit,sizeof(body_bit));
		line_size += sizeof(body_bit) - 1;
	}

	token_string[line_size++] = '\\';
	token_string[line_size++] = '<';

	if (operator)
		token_string[line_size++] = '\"';

	memcpy(token_string+line_size,search_token,search_size);
	line_size += search_size;

	if (operator)
		token_string[line_size++] = '\"';

	memcpy(token_string+line_size,post_fix,sizeof(post_fix));
	line_size += sizeof(post_fix) - 1;

	if (level[0] == '2')
	{
		memcpy(token_string+line_size,extension,sizeof(extension));
		line_size += sizeof(extension);

		token_string[line_size-1] = res_type[found];

		sprintf(line_stuff,"\tline:%d",line);
		size = strlen(line_stuff);
		memcpy(token_string+line_size,line_stuff,size);
		line_size+= size;

		/* add the architecture line */
		if (in_architecture && found != ARCHITECTURE)
		{
			memcpy(token_string+line_size,arch_stuff,sizeof(arch_stuff));
			line_size += sizeof(arch_stuff) - 1;

			memcpy(token_string+line_size,arch_token,arch_token_size);
			line_size += arch_token_size;
		}

		/* add function or procedure lines */
		if (in_funcProc && (found != PROCEDURE && found != FUNCTION))
		{

			memcpy(token_string+line_size,funcProc_stuff[in_funcProc],funcProc_size[in_funcProc]);
			line_size += funcProc_size[in_funcProc] - 1;

			memcpy(token_string+line_size,funcProc_token,funcProc_token_size);
			line_size += funcProc_token_size;
		}

		/* add package lines */
		if (in_package && found != PACKAGE)
		{

			memcpy(token_string+line_size,pack_stuff,sizeof(pack_stuff));
			line_size += sizeof(pack_stuff) - 1;

			memcpy(token_string+line_size,pack_token,pack_token_size);
			line_size += pack_token_size;
		}

	}

	memcpy(token_string+line_size,line_end,sizeof(line_end));
	line_size += sizeof(line_end) - 1;

	/* write the entry to the temporary file */
	write(tempfile,token_string,line_size);

	/* empty strings breaks my sort */
	if (token[0] != '\0')
		add_to_index(token,token_size,*filesize,line_size);

	*filesize = *filesize + line_size;
}

void	make_tags(char* path_file_name,int file_name_size,int infile,int tempfile,char* level,int* filesize)
{
	int		is_a_list;
	int		search_size = 0;
	int 	token_size,hasbody,operator,found,line = 1;
	char	token[MAX_TOKEN_LENGTH];
	char*	list;
	char*	list_token;
	char*	search_token;
	
	while((found = find_next_tag(infile,token,&token_size,&hasbody,&operator,&line,&is_a_list)) != -1)
	{
		/* zero means its found END which we ignore */
		if (found > 0 && found < BEGIN)
		{
			if (is_a_list)
			{
				/* we have a list of items, i.e. "signal <name>,<name> : std_logic;" */
				list = token;
				search_size= 0;
				while ((list = get_next_list_entry(list,&list_token,&token_size)) != NULL)
				{
					if (search_size == 0)
					{
						search_token = list_token;
						search_size  = token_size;
					}

					add_tag(path_file_name,
							file_name_size,
							tempfile,
							list_token,token_size,
							search_token,search_size,
							found,
							level,
							hasbody,
							operator,
							line,
							filesize);
				}
				/* pick up last entry in the list */
				add_tag(path_file_name,
						file_name_size,
						tempfile,
						list_token,token_size,
						search_token,search_size,
						found,
						level,
						hasbody,
						operator,
						line,
						filesize);
			}else{
				/* just a single tag */
				add_tag(path_file_name,
						file_name_size,
						tempfile,
						token,token_size,
						token,token_size,
						found,
						level,
						hasbody,
						operator,
						line,
						filesize);
			}
		}
	}
}

