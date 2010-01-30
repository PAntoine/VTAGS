/*--------------------------------------------------------------------------
---                                                             ADD TO INDEX
---                                                        vtags - VHDL TAGS 
----------------------------------------------------------------------------
--- This function will add a new tag to the tags index. This will insert
--- the tag in sort order. This will be used later to genrerate the TAGS
--- file in the correct order.
---
--- Author:	Peter Antoine		Date: 1st Sep 2004
----------------------------------------------------------------------------
---                                         Copyright (c) 2004 Peter Antoine
----------------------------------------------------------------------------*/

/*TOP and bottom element must be set to "\0\0\0\0\0\0" and "\ff\ff\ff\ff" respectively for this to work */

#include "vtags.h"
#include <memory.h>

#include <stdio.h>
#include <stdlib.h>

extern	TAG_TABLE	tag_table;

TAG_BLOCK	temp[10];

int	BinarySplit(int prev_bin,int next_bin)
{
	TAG_ELEMENT*	new_value;

	/* if bin is larger by 2, move the pointer */	
	if(((tag_table.bins[prev_bin].size - tag_table.bins[next_bin].size) & 0x03) == 2)
	{
		if (tag_table.bins[prev_bin].size > tag_table.bins[next_bin].size)
		{
			/* move prev */
			new_value = tag_table.bins[next_bin].startPoint->prev;
			tag_table.bins[prev_bin].size--;
			tag_table.bins[next_bin].size++;
		}else{
			/* move next */
			new_value = tag_table.bins[next_bin].startPoint->next;
			tag_table.bins[prev_bin].size++;
			tag_table.bins[next_bin].size--;
		}

		tag_table.bins[next_bin].token		= new_value->token;
		tag_table.bins[next_bin].startPoint = new_value;
	
		return 1;
	}
	return 0;
}

int	InsertElement(TAG_ELEMENT* element)
{
	int	count;
	TAG_ELEMENT*	current;
	
	/* find the bin that it is in */
	for (count=0;count<NUM_OF_BINS-1;count++)
	{
		if (strcmp(element->token,tag_table.bins[count+1].token) < 0)
		{
			/* have found the bin we want */
			current = tag_table.bins[count].startPoint;
			
			/* find the point in the linked list that it should be in */
			while(strcmp(current->token,element->token) < 0)
				current = current->next;

			/* insert the entry */
			element->next = current;
			element->prev = current->prev;
			current->prev->next = element;
			current->prev = element;
			
			/* update the bin count */
			tag_table.bins[count].size++;
			
			/* exit the for loop */
			break;
		}
	}

	return count;
}

int crap = 0;

void	add_to_index(char* token, int token_size, int position, int line_size)
{
	int		bin,current;
	char*	token_pointer;

	/* add this to the name table */
	if ((tag_table.next_namespace + token_size + 1) >= TAG_NAMESPACE_SIZE)
	{
		tag_table.last_name_block->next = (TAG_NAMESPACE*) calloc(sizeof(TAG_NAMESPACE),1);
		tag_table.last_name_block = tag_table.last_name_block->next;
		tag_table.next_namespace = 0;
	}

	token_pointer = memcpy(&tag_table.last_name_block->namedata[tag_table.next_namespace],token,token_size+1);
	tag_table.next_namespace += token_size+1;

	/* add this to the tag table */
	if (TAG_BLOCK_SIZE == tag_table.last_tag)
	{
		tag_table.last_tag_block->next = (TAG_BLOCK*) calloc(sizeof(TAG_BLOCK),sizeof(char));
		tag_table.last_tag_block = tag_table.last_tag_block->next;
		tag_table.last_tag = 0;

		tag_table.last_tag_block->next = 0;
	}

	tag_table.last_tag_block->entry[tag_table.last_tag].token = token_pointer;
	tag_table.last_tag_block->entry[tag_table.last_tag].line_size = line_size;
	tag_table.last_tag_block->entry[tag_table.last_tag].file_position = position;

	bin = InsertElement(&tag_table.last_tag_block->entry[tag_table.last_tag]);
		
	tag_table.last_tag++;
	
	current = bin;
	
	/* cascading changes forward */
	while(current<NUM_OF_BINS-2 && BinarySplit(current,current+1))
		current++;
	
	/* cascading changes backwards */
	while(bin>0 && BinarySplit(bin-1,bin))
		bin--;
}
