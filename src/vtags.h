/*--------------------------------------------------------------------------
---                                                              HEADER FILE
---                                                        vtags - VHDL TAGS 
----------------------------------------------------------------------------
--- This header file describes the structures that are needed by the VTAGS
--- application.
---
--- Author:	Peter Antoine		Date: 1st Sep 2004
----------------------------------------------------------------------------
---                                         Copyright (c) 2004 Peter Antoine
----------------------------------------------------------------------------*/

#ifndef __TAG_HEADER__
#define __TAG_HEADER__

#include "dir_stuff.h"

#define	TAG_FRONT_ELEMENT	0
#define TAG_END_ELEMENT		1

#define	TAG_NAMESPACE_SIZE	(64 * 1024)
#define TAG_BLOCK_SIZE		(200)
#define	NUM_OF_BINS			(20)

#define	SEARCH_BUFFER_SIZE	(64 * 1024)

#define	MAX_TOKEN_LENGTH	(128)
#define	MAX_TAG_STRING_SIZE	(1024)

typedef enum{
		END,
		ENTITY,
		ARCHITECTURE,
		COMPONENT,
		SIGNAL,
		PACKAGE,
		FUNCTION,
		PROCEDURE,
		CONSTANT,
		SUBTYPE,
		TYPE,
		VARIABLE,
		BEGIN,
		IF,
		CASE,
		MAX_RESERVED
} names;

/*------------------------------------------------------------*
 * Structure definitions
 *------------------------------------------------------------*/
typedef struct tt_namespace
{
	struct tt_namespace*	next;
	char					namedata[TAG_NAMESPACE_SIZE];
	
} TAG_NAMESPACE;

typedef struct tt_element
{
	int					line_size;
	int					file_position;
	const char*			token;
	struct tt_element*	prev;
	struct tt_element*	next;
	
} TAG_ELEMENT;

typedef	struct
{
	int				size;
	const char*		token;
	TAG_ELEMENT*	startPoint;
} BIN_ELEMENT;

typedef struct tt_block
{
	struct	tt_block*	next;
	TAG_ELEMENT			entry[TAG_BLOCK_SIZE];

} TAG_BLOCK;

typedef	struct tt_struct
{
	TAG_BLOCK*		last_tag_block;
	TAG_NAMESPACE*	last_name_block;

	int				last_tag;
	int				next_namespace;
	TAG_ELEMENT*	next_item;
	
	BIN_ELEMENT		bins[NUM_OF_BINS];
	TAG_BLOCK		tag_list;
	TAG_NAMESPACE	tag_names;

} TAG_TABLE;


/*------------------------------------------------------------*
 * Function definitions
 *------------------------------------------------------------*/
char**	GetListFromParam(char* parameter);
void	ClearList(char** list);
int		cpystrlen(char *s1, char *s2, int n);
int		makefilename(char* path_file_name,char* filename,int current_dir,DIR_ENTRY* dir_tree);
void	fillBuffer(int infile);
void	DiscardComment(int infile);
void	DiscardString(int infile);
void	skipToWhite(int infile);
int		GetToken(char* token,int infile,int* operator,int allow_lists,int* is_a_list);
int		CheckReserved(int infile,int inc_specials);
int		find_next_tag(int infile,char* token,int *token_size,int *hasbody,int* operator,int *line,int *is_list);
void	fillBuffer(int infile);
int		find_next_file(DIR_ENTRY* dir_tree,int* curr_dir,char** extention_list,char* filename,int recursive);
int		find_first_dir(char* start_path,DIR_ENTRY* dir_tree,int* curr_dir,int recursive);
int		BinarySplit(int prev_bin,int next_bin);
int		InsertElement(TAG_ELEMENT* element);
void	add_to_index(char* token, int token_size, int position, int line_size);
void	release_index(void);
int		get_index_item(int* position,int* line_size);
char*	get_next_list_entry(char* list,char** list_token,int* token_size);

#endif
