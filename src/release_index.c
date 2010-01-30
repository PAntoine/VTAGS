/*--------------------------------------------------------------------------
---                                                            RELEASE INDEX
---                                                        vtags - VHDL TAGS 
----------------------------------------------------------------------------
--- This function will release all the memory allocated for the tag table. 
---
--- Author:	Peter Antoine		Date: 1st Sep 2004
----------------------------------------------------------------------------
---                                         Copyright (c) 2004 Peter Antoine
----------------------------------------------------------------------------*/

#include "vtags.h"
#include <stdio.h>

extern	TAG_TABLE	tag_table;

void	release_index(void)
{
	TAG_BLOCK*		tag_block,*tag_block_next;
	TAG_NAMESPACE*	name_block,*name_block_next;
	
	
	/* release the name table */
	name_block = tag_table.tag_names.next;

	while(name_block != NULL)
	{
		name_block_next = name_block->next;
		free(name_block);
		name_block = name_block_next;
	}

	/* release the tag block table */
	tag_block = tag_table.tag_list.next;

	while(tag_block != NULL)
	{
		tag_block_next = tag_block->next;
		free(tag_block);
		tag_block = tag_block_next;
	}
}
