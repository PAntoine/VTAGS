/*--------------------------------------------------------------------------
---                                                            FIND NEXT TAG
---                                                        vtags - VHDL TAGS 
----------------------------------------------------------------------------
--- This will find the next tag in the file. It will return the token and
--- the line that the token is on. It will also return the tag line and 
--- the size of the tag line in TAG format (vim tags).
---
--- Author:	Peter Antoine		Date: 1st Sep 2004
----------------------------------------------------------------------------
---                                         Copyright (c) 2004 Peter Antoine
----------------------------------------------------------------------------*/

#include "vtags.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "lookuptables.h"

extern char*	reserved[];
extern int		reserved_size[];

extern int		in_architecture;
extern int		arch_token_size;
extern char		arch_token[];

extern int		in_package;
extern int		pack_token_size;
extern char		pack_token[];

extern int		in_funcProc;
extern int		funcProc_token_size;
extern char		funcProc_token[];

static	int		line_no;

int				my_eof = 0;
int				position = 0;
int				levelcount = 0;
int				pack_level = 0;
int				first_begin = 0;
int				funcProc_level = 0;
unsigned char	read_buffer[SEARCH_BUFFER_SIZE];

/*------------------------------------------------------------*
 * FILL BUFFER
 *
 * This function will read more data into the buffer. It will
 * also set the position to the start of the buffer.
 *------------------------------------------------------------*/
void	fillBuffer(int infile)
{
	int	bytes_read;
	
	position = 0;

	if ((bytes_read = read(infile,read_buffer,SEARCH_BUFFER_SIZE)) != SEARCH_BUFFER_SIZE)
		my_eof = 1;
	else
		my_eof = 0;

	/* guard char */
	if (bytes_read > 0)
		read_buffer[bytes_read-1] = 0xff;
	else
		read_buffer[0] = 0xff;
}


/*------------------------------------------------------------*
 * Discard Comments
 *
 * This function will discard the comments if it is a comment.
 * Like all the helper functions it may need to refill the
 * buffer.
 *------------------------------------------------------------*/
void DiscardComment(int infile)
{
	int		not_found = 1;
	
	position++;
	
	if (read_buffer[position] == 0xff)
		fillBuffer(infile);

	if (read_buffer[position] == '-')
	{
		/* we have a comment */
		do{
			while(!(read_buffer[position] == 0x0a || read_buffer[position] == 0x0d || read_buffer[position] == 0xff))
			{
				position++;
			}

			if (read_buffer[position] == 0xff && !my_eof)
				fillBuffer(infile);
			else
				not_found = 0;
		}
		while(not_found);
	}
}

/*------------------------------------------------------------*
 * found_is
 *
 * This function will discard all the stuff between where it
 * is called from until it finds the 'is' or it finds after 
 * the bracket pairs a ';'.
 *------------------------------------------------------------*/
int	found_is(int infile,int* line)
{
	int	found = 0,finished = 0,level = 0;
	int	found_s = 0,found_i = 0, found_w = 0;
	
	do
	{
		switch(read_buffer[position])
		{
			case 0xff:	
				if (my_eof)
					finished = 1;
				else
					fillBuffer(infile);	
				break;

			case 0x0a:	*line = *line + 1;
						position++;
						break;
				
			case 0x0d: /* handle the DOS non-sense */
						position++;
						*line = *line + 1;

						if (read_buffer[position] == 0x0a)
							position++;
						else if (read_buffer[position] == 0xff)
						{
							fillBuffer(infile);
							if(read_buffer[position] == 0x0a)
								position++;
							else if (read_buffer[position] == 0xff)
								finished = 1;
						}
						break;

			case ';':	/* handle the end of the definition */
						if (level <= 0)
							finished = 1;

						position++;
						break;

			case '-':	/* may have a comment -- need to remove */
						DiscardComment(infile);
						break;			

			case '(':	level++;
						position++;
						break;

			case ')':	level--;
						position++;
						break;	
							
			default:	if (level <= 0)
						{
							if (found_s && terminator[read_buffer[position]])
							{
								found = 1;
								finished = 1;
							}
							else if (found_i && tolower(read_buffer[position]) == 's')
							{
								found_s = 1;
								found_i = 0;
								found_w = 0;
							}
							else if (found_w &&  tolower(read_buffer[position]) == 'i')
							{
								found_s = 0;
								found_i = 1;
								found_w = 0;
							}
							else if (white_space[read_buffer[position]])
							{
								found_s = 0;
								found_i = 0;
								found_w = 1;
							}
							else
							{
								found_s = 0;
								found_i = 0;
								found_w = 0;
							}
						}
							
						position++;
		}
	}
	while(!finished);
	
	return found;	
}
/*------------------------------------------------------------*
 * Discard String
 *
 * This function will discard the string that we have just
 * found.
 *------------------------------------------------------------*/
void DiscardString(int infile)
{
	int		not_found = 1;
	
	position++;
	
	/* we have a string */
	do{
		/* not sure if VHDL allows for multi-line strings ??? */
		while(!(read_buffer[position] == '\"' || read_buffer[position] == 0xff))
			position++;

		if (read_buffer[position] == 0xff && !my_eof)
			fillBuffer(infile);
		else
			not_found = 0;
	}
	while(not_found);
}

/*------------------------------------------------------------*
 * Skip to white space
 *
 * This function will discard all caracters untill we hit the
 * next white space. This is because if we have not found the
 * reserved word as the first letter after the white space then
 * it wont be around until the next white space.
 *------------------------------------------------------------*/
void skipToWhite(int infile)
{
	int	not_found = 1;
	
	position++;
	
	do{
		while(!terminator[read_buffer[position]] && read_buffer[position] != 0xff)
			position++;

		if (read_buffer[position] == 0xff && !my_eof)
			fillBuffer(infile);
		else
			not_found = 0;
	}
	while (not_found);
}

/*------------------------------------------------------------*
 * GetToken
 *
 * This function will fill the token with all the characters
 * it finds until it gets a terminator.
 *------------------------------------------------------------*/
int GetToken(char* token,int infile,int* operator,int allow_lists,int *is_a_list)
{
	int	not_found = 1,token_length = 0,eol = 0;
	
	/* this are set by this function */
	*operator = 0;
	
	position++;
	
	/* remove leading white spaces */
	do{
		while(white_space[read_buffer[position]])
			position++;

		if (read_buffer[position] == 0xff && !my_eof)
			fillBuffer(infile);
		else
			not_found = 0;
	}
	while (not_found);

	not_found = 1;
	
	/* now read the character */
	if (read_buffer[position] == '\"')
	{
		/* we are in a world of smells, these are 'special names' that are
		 * only (as far as I can stand from the spec) operator names. But,
		 * as this is a tags generator and not a compiler then I'll collect
		 * all chars between here and the next " or eol, I am not going to
		 * validate them.
		 */

		*operator = 1;
		
		position++;

		do{
			switch(read_buffer[position])
			{
				case 0xff:	fillBuffer(infile);
							break;
				case 0x0a:
				case 0x0d:	eol = 1;
							break;
				case '\"':	not_found = 0;
							break;
				default:	token[token_length++] = read_buffer[position];
							position++;
							break;
			}
		}
		while(not_found && !eol);

	}else{
		/* normal names */
		if (allow_lists)
		{
			do{
				/* commas and white spaces are allowed in lists 
				 * WARNING: tokens in the list may be bigger than allowed by
				 *          the VHDL spec. This needs to be caught later.    
				 *          I am using MAX_TAG_STRING_SIZE to end it somewhere
				 *          in case of a bad file, we dont want to loop forever.
				 */
				while((!list_term[read_buffer[position]]) && token_length < MAX_TAG_STRING_SIZE)
				{
					if (read_buffer[position] == ',')
					{
						*is_a_list = 1;
					}
					token[token_length++] = read_buffer[position++];
				}

				/* trim white at end */
				while(white_space[token[token_length-1]] && token_length > 0)
				{
					token_length--;
				}

				token[token_length] = '\0';

				if (read_buffer[position] == 0xff)
					fillBuffer(infile);
				else
					not_found = 0;
			}
			while (not_found && token_length < MAX_TAG_STRING_SIZE);

		}else{
			/* lists are not allowed */
			do{
				while(!terminator[read_buffer[position]] && token_length < MAX_TOKEN_LENGTH)
				{
					token[token_length++] = read_buffer[position++];
				}

				token[token_length] = '\0';

				if (read_buffer[position] == 0xff)
					fillBuffer(infile);
				else
					not_found = 0;
			}
			while (not_found && token_length < MAX_TOKEN_LENGTH);
		}
	}

	return token_length;
}


/*------------------------------------------------------------*
 * Check Reserved
 *
 * This function will check to see if the word is in the
 * reserved list. If it is it will return the reserved work
 * index.
 *
 * Note: This uses a doggy trick to check for the reserved
 * words, take care in changing the list of reserved.
 *------------------------------------------------------------*/
int	CheckReserved(int infile,int inc_specials)
{
	int		index, check, num_reserved,not_found = 1;
	char	hold[3];

	/* only search for IF and CASE when doing architecture level count stuff */
	if (inc_specials)
		num_reserved = MAX_RESERVED;
	else
		num_reserved = BEGIN + 1;
		
	/* reserved words are unique after three chars */
	if (read_buffer[position] == 0xff) fillBuffer(infile);
	hold[0] = tolower(read_buffer[position++]);

	if (read_buffer[position] == 0xff) fillBuffer(infile);
	hold[1] = tolower(read_buffer[position++]);

	if (read_buffer[position] == 0xff) fillBuffer(infile);
	hold[2] = tolower(read_buffer[position++]);

	/* search for the first three chars in the reserved list */
	for (index=0;index<num_reserved;index++)
	{
		if (index == IF && reserved[index][0] == hold[0] && reserved[index][1] == hold[1])
			break;
		else if (reserved[index][0] == hold[0] && reserved[index][1] == hold[1] && reserved[index][2] == hold[2])
			break;
	}

	/* IF is a pain in the arse!!! */
	if (inc_specials && index == IF && (white_space[hold[2]] || terminator[hold[2]]))
	{
		return index;

	} else{
		check = 3;	
		
		/* we should now know what we are looking for */
		if (index < num_reserved)
		{
			/* we have a reserved word */
			do{
				while(reserved[index][check] == tolower(read_buffer[position]) && check < reserved_size[index])
				{
					check++;
					position++;
				}

				if (read_buffer[position] == 0xff && !my_eof)
					fillBuffer(infile);
				else
					not_found = 0;
			}
			while(not_found);
		
			/* check is the number of chars we have read and that its followed by a white space */
			if (check != reserved_size[index]  || !white_space[read_buffer[position]])
				index = -1;

			return index;
		}
		else
		{
			return -1;
		}
	}
}


/*------------------------------------------------------------*
 * FIND NEXT TAG
 *
 * This function will find the next tag in the current file.
 * It may now also find a list of tags to create. If the 
 * signal or variable tag has a list of symbols to be defined
 * then this will return that list, that will need to be
 * transversed for it to be found.
 *------------------------------------------------------------*/
int	find_next_tag(int infile,char* token,int *token_size,int *hasbody,int* operator,int *line,int* is_a_list)
{
	int		notused;
	int		bytes_read,curr_pos,count;
	int		found = 0;

	*hasbody = 0;
	*is_a_list = 0;

	do
	{
		switch(read_buffer[position])
		{
			/* white space removal */
			case 0x0a:	*line = *line + 1;
						position++;
						break;
				
			case 0x0d: /* handle the DOS non-sense */
						position++;
						*line = *line + 1;

						if (read_buffer[position] == 0x0a)
							position++;
						else if (read_buffer[position] == 0xff)
						{
							fillBuffer(infile);
							if(read_buffer[position] == 0x0a)
								position++;
							else if (read_buffer[position] == 0xff)
								found = -1;
						}
						break;

			case 0x20:
			case 0x09:
				position++;
				break;
	
			case '\"':
				DiscardString(infile);		
				
			/* check for comments */	
			case '-':
				DiscardComment(infile);
				break;

			/* check only for the first letters of the reserved words */
			case 'E':
			case 'A':
			case 'C':
			case 'S':
			case 'P':
			case 'F':
			case 'T':
			case 'V':
			case 'B':
			case 'e':
			case 'a':
			case 'c':
			case 's':
			case 'p':
			case 'f':
			case 't':
			case 'v':
			case 'b':
				found = CheckReserved(infile,0);
				
				if (found == -1)
				{
					skipToWhite(infile);
					found = 0;
				}
				else if (found == BEGIN && in_architecture)
				{
					first_begin = 1;
					levelcount++;
					pack_level++;
					funcProc_level++;
					found = 0;
				}
				else if (found == END)
				{
					/* we have an end -- need to handle this */
					position++;
					if ((found = CheckReserved(infile,1)) == -1)
					{
						skipToWhite(infile);
					}

					if (in_package)
					{
						if (found != CASE && found != IF)
						{
							pack_level--;

							if (pack_level <= 0)
								in_package = 0;
						}
					}

					if (in_funcProc)
					{
						if (found != CASE && found != IF)
						{
							funcProc_level--;

							if (funcProc_level <= 0)
								in_funcProc = 0;
						}
					}


					if (in_architecture)
					{
						if (found != CASE && found != IF)
						{
							levelcount--;

							if (found == ARCHITECTURE)
							{
								/* end architecture */
								in_architecture = 0;
							}
							else if (first_begin && levelcount <= 0)
							{
								/* we have run out of levels */
								in_architecture = 0;
							}
						}
					}

					found = 0;
				}
					
				break;

			/* using 0xff as the end of buffer marker */	
			case 0xff:
				if (my_eof)
					found = -1;
				else
					fillBuffer(infile);	
				break;
				
			/* if it does not start with a reserved, then skip to next white */
			default:
				skipToWhite(infile);
		}
	}
	while(!found);
			
	/* we have a reserved word, and we also know which one we have found 
	 * so lets get create the search string that we need 
	 */
	if (found != -1)
	{
		switch(found)
		{
			case END:					/* we want to lose the next token after the end */
				GetToken(token,infile,&notused,0,NULL);
				token[0] = '\0';
				*token_size = 0;
				break;

			case FUNCTION:				/* <function> <token> <stuff> [is] --- problem definition and imp are no different!  (well no 'is')*/
			case PROCEDURE:				/* <procedure> <token> <stuff> [is] --- problem definition and imp are no different! (well no 'is')*/
				
				*token_size = GetToken(token,infile,operator,0,NULL);

				if (found_is(infile,line))
				{
					/* function or procedure body --- start the funcProc grouping stuff */
					in_funcProc = 1;
					funcProc_level = 0;
					memcpy(funcProc_token,token,*token_size);
					funcProc_token_size = *token_size;
				}
				break;
				
				
			case PACKAGE:				/* <package> [<body>] <token> [is] */
				
				*token_size = GetToken(token,infile,operator,0,NULL);

				if (tolower(token[0]) == 'b' && tolower(token[1]) == 'o' && tolower(token[2]) == 'd' && tolower(token[3]) == 'y' && token[4] == '\0')
				{
					*hasbody = 1;
					*token_size = GetToken(token,infile,operator,0,NULL);
				}else{
					/* start the package group stuff */
					in_package = 1;
					memcpy(pack_token,token,*token_size);
					pack_token_size = *token_size;
					pack_level = 0;
				}

				break;

			/* search string: "/\c<reserved>\s\+<name>/" */
			case ARCHITECTURE:			/* <architecture> <token> [of] <entity_name> <is> -- non-unique need the entity as well */
				first_begin = 0;
				in_architecture = 1;
				levelcount = 0;
				*token_size = GetToken(token,infile,operator,0,NULL);
				memcpy(arch_token,token,*token_size);
				arch_token_size = *token_size;
				break;

			case SIGNAL:				/* <signal> <token> [,<token>] ':' <other stuff> */
			case VARIABLE:				/* <variable> <token> [,<token>] ':' <stuff> */
				*token_size = GetToken(token,infile,operator,1,is_a_list);
				break;

			case TYPE:					/* <type> <token> 'is' <stuff> */
			case COMPONENT:				/* <component> <token> [is]  */
			case CONSTANT:				/* <constant> <token> ':' <stuff> */
			case SUBTYPE:				/* <subtype> <token> 'is' <stuff> */
			case ENTITY:				/* <entity> <token> [of]*/
				*token_size = GetToken(token,infile,operator,0,NULL);
				break;
		}
	}
	
	return found;
}
