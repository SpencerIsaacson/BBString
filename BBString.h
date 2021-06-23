/*
	This is a small quality of life mini-library I wrote to make string manipulation in C somewhat tolerable.
	Spencer Isaacson aka Baremetal Baron 2021
*/

//todo add asserts to ensure you're not going out of the available space
//todo replace unnecessary strlens

#ifndef BaremetalBaronString
#define BaremetalBaronString
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//8 megabytes of string storage, 1 kilobyte of scratch space
#define arena_size 1024*1024*8
#define scratch_pad_size 1024

int arena_write_index = 0;
char string_arena[arena_size];
char scratch_pad[scratch_pad_size];

typedef struct bbs_string
{
	int length;
	char* chars;
} bbs_string; //todo put into use

typedef struct bbs_StringArray
{
	int count;
	char** strings;
} bbs_StringArray;

void bbs_clear_scratchpad()
{
	for (int i = 0; i < scratch_pad_size; ++i)
	{
		scratch_pad[i] = 0;
	}
}

void bbs_clear_pool()
{
	for (int i = 0; i < arena_size; ++i)
	{
		string_arena[i] = 0;
	}
}

void bbs_print(bbs_string text)
{
	printf("%.*s\n", text.length, text.chars);
}

char* bbs_join(char** strings, int count, char* separator)
{
	int n = 0;
	for (int i = 0; i < count; ++i)
	{
		for (int c = 0; c < strlen(strings[i]); ++c)
		{
			scratch_pad[n++] = strings[i][c];
		}

		if(i<count-1)
		{
			for (int c = 0; c < strlen(separator); ++c)
			{
				scratch_pad[n++] = separator[c];
			}
		}
	}

	scratch_pad[n] = 0; //null terminate

	char* result = &string_arena[arena_write_index];
	for (int i = 0; i < strlen(scratch_pad); ++i)
		result[i] = scratch_pad[i];
	arena_write_index += strlen(result);
	return result;
}

bool bbs_contains_char(char* text, char c)
{
	for (int i = 0; i < strlen(text); ++i)
		if(text[i] == c)
			return true;

	return false;
}

bbs_StringArray bbs_split(char* text, char* delimiters) //todo get rid of mallocs
{
	bbs_StringArray* result = (bbs_StringArray*)&string_arena[arena_write_index];
	(*result).count = 0;
	int text_cursor = 0;
	int u = 0;
	bool was_delimiter = true;

	int str_len = strlen(text);
	while(text_cursor <  str_len)
	{
		if(bbs_contains_char(delimiters, text[text_cursor]))
		{
			was_delimiter = true;
		}
		else
		{
			if(was_delimiter){
				(*result).count++;
			}

			was_delimiter = false;
		}
		
		text_cursor++;
	}

	text_cursor = 0;
	
	(*result).strings = (char**)((&(result -> strings))+1);
	arena_write_index += sizeof(bbs_StringArray) + (((*result).count+1)*sizeof(char*));
	u = 0;

	for (int i = 0; i < (*result).count; ++i)
	{
		(*result).strings[i] = (char*)&string_arena[arena_write_index];
		while(text_cursor <  str_len)
		{
			if(bbs_contains_char(delimiters, text[text_cursor]))
			{
				was_delimiter = true;
				(result -> strings)[i][u] = 0;
				u = 0;
				while(bbs_contains_char(delimiters, text[text_cursor]))
					text_cursor++;
				break;
			}
			else
			{
				was_delimiter = false;
				result -> strings[i][u++] = text[text_cursor];
			}
			
			text_cursor++;
		}

		arena_write_index += strlen((*result).strings[i])+1;
	}

	return (*result);
}

bool bbs_contains(char* text, char* substring)
{
	int text_cursor = 0;
	int substring_cursor = 0;
	while(text_cursor < strlen(text))
	{
		if(substring_cursor < strlen(substring))
		{
			if(text[text_cursor] == substring[substring_cursor])
				substring_cursor++;
			else substring_cursor = 0;
		}
		else 
			return true;
		text_cursor++;
	}

	return false;
}

int bbs_index_of(char* text, char* substring)
{
	int text_cursor = 0;
	int substring_cursor = 0;
	while(text_cursor < strlen(text))
	{
		if(substring_cursor < strlen(substring))
		{
			if(text[text_cursor] == substring[substring_cursor])
				substring_cursor++;
			else substring_cursor = 0;
		}
		else 
			return text_cursor-substring_cursor;
		text_cursor++;
	}

	return -1;
}

char* bbs_replace(char* text, char* to_replace, char* with)
{
	int index = bbs_index_of(text,to_replace);
	if(index > -1)
	{
		for (int i = 0; i < index; ++i)
		{
			scratch_pad[i] = text[i];
		}
		for (int i = 0; i < strlen(with); ++i)
		{
			scratch_pad[i+index] = with[i];
		}
		for (int scratch_index = index+strlen(with), text_index = index+strlen(to_replace); text_index < strlen(text); scratch_index++, text_index++)
		{
			scratch_pad[scratch_index] = text[text_index];
		}

		scratch_pad[strlen(text)+strlen(with)-strlen(to_replace)] = 0; //null terminate

		char* result = &string_arena[arena_write_index];

		for (int i = 0; i <= strlen(scratch_pad); ++i)
			result[i] = scratch_pad[i];

		arena_write_index += strlen(result)+1;				
		return result;
	}
	return text;
}

char* bbs_delete_substring(char* text, char* substring)
{
	bbs_clear_scratchpad();
	bbs_replace(text,substring, "");
}

char* bbs_delete_at(char* text, int index, int count)
{
	if((index+count) >= strlen(text))//todo verify not off by one
		return NULL;

	char* result = &string_arena[arena_write_index];

	for (int i = 0; i < index; i++)
	{
		result[i] = text[i];
	}

	for (int i = index+count; i < strlen(text); ++i)
	{
		result[i-count] = text[i];
	}

	int length = strlen(text)-count;
	result[length] = 0; //null terminate
	arena_write_index += length;//todo make sure this isn't an off-by-one
	return result;
}

char* bbs_insert(char* text, char* substring, int index)
{
	if(index >= strlen(text))
		return NULL;

	for (int i = 0; i < index; ++i)
		scratch_pad[i] = text[i];

	for (int i = 0; i < strlen(substring); ++i)
		scratch_pad[i+index] = substring[i];

	for (int i = index; i < strlen(text); ++i)
		scratch_pad[strlen(substring)+i] = text[i];

	scratch_pad[strlen(substring)+strlen(text)] = 0;//null terminate
	
	char* result = &string_arena[arena_write_index];

	for (int i = 0; i < strlen(scratch_pad); ++i)
		result[i] = scratch_pad[i];

	arena_write_index += strlen(result);
	return result;
}

bool bbs_equals(char* a, char* b)
{
	int a_len = strlen(a);
	int b_len = strlen(b);

	if(a_len != b_len)
		return false;

	for (int i = 0; i < a_len; ++i)
		if(a[i] != b[i])
			return false;

	return true;	
}

char* bbs_concatenate(char* a, char* b)
{
	char* result = &string_arena[arena_write_index];
	int a_len = strlen(a);
	int b_len = strlen(b);
	int tot_len = a_len + b_len;

	for (int i = 0; i < a_len; ++i)
	{
		result[i] = a[i];
	}

	for (int i = 0; i < b_len; ++i)
	{
		result[i + a_len] = b[i];
	}

	result[tot_len] = 0; //null terminate
	arena_write_index += tot_len;//todo make sure this isn't an off-by-one
	return result;
}

int bbs_parse_int()
{
	assert(false); //not implemented. TODO
}

float bbs_parse_float()
{
	assert(false); //not implemented. TODO
}

double bbs_parse_double()
{
	assert(false); //not implemented. TODO
}


//TODO add string builder stuff?
#endif