/*--------------------------------------------------------------------------
---                                                           GET INDEX ITEM 
---                                                        vtags - VHDL TAGS 
----------------------------------------------------------------------------
--- This function will walk through the index linked list and return the
--- next item. 
---
--- Author:	Peter Antoine		Date: 1st Sep 2004
----------------------------------------------------------------------------
---                                         Copyright (c) 2004 Peter Antoine
----------------------------------------------------------------------------*/

#include "vtags.h"

extern	TAG_TABLE	tag_table;

int	get_index_item(int* position,int* line_size)
{
	if (tag_table.next_item != &tag_table.tag_list.entry[TAG_END_ELEMENT])
	{
		tag_table.next_item = tag_table.next_item->next;

		/* now return the values */
		*position = tag_table.next_item->file_position;
		*line_size = tag_table.next_item->line_size;

		return 1;
	}
	return 0;
}
