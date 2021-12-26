/*
	This is a small quality of life mini-library I wrote to make string manipulation in C somewhat tolerable.
	Spencer Isaacson aka Baremetal Baron 2021
*/

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
} bbs_string;

typedef struct bbs_StringList
{
	int count;
	bbs_string* strings;
} bbs_StringList;

#define bbs_get_count(x) sizeof(x)/sizeof(x[0])
#define bbs_to_string_list(x) ((bbs_StringList){ .strings = x, .count = bbs_get_count(x) })

int bbs_c_string_length(char* c_str) //replacement for strlen
{
	int n = 0;
	while(c_str[n++] != 0);
	return n-1;
}

bbs_string bbs_from_c_str(char* c_str)
{
	return (bbs_string){.length = bbs_c_string_length(c_str), .chars = c_str };
}

void bbs_clear_arena()
{
	memset(string_arena, 0, arena_size);
}

void bbs_print(bbs_string text)
{
	printf("%.*s\n", text.length, text.chars);
}

bbs_string bbs_join(bbs_StringList string_list, bbs_string separator)
{
	int n = 0;
	for (int i = 0; i < string_list.count; ++i)
	{
		for (int c = 0; c < string_list.strings[i].length; ++c)
			scratch_pad[n++] = string_list.strings[i].chars[c];

		if(i < string_list.count-1)
		{
			for (int c = 0; c < separator.length; ++c)
				scratch_pad[n++] = separator.chars[c];
		}
	}

	char* result = &string_arena[arena_write_index];
	memcpy(result, scratch_pad, n);
	memset(scratch_pad, 0, n);

	arena_write_index += n;
	return (bbs_string){ .length = n, .chars = result };
}

bool bbs_contains_char(bbs_string text, char c)
{
	for (int i = 0; i < text.length; ++i)
		if(text.chars[i] == c)
			return true;

	return false;
}

bbs_StringList bbs_split(bbs_string text, bbs_string delimiters)
{
	bbs_StringList result;
	result.count = 0;
	int text_cursor = 0;
	int u = 0;
	bool was_delimiter = true;

	while(text_cursor <  text.length)
	{
		if(bbs_contains_char(delimiters, text.chars[text_cursor]))
		{
			was_delimiter = true;
		}
		else
		{
			if(was_delimiter){
				result.count++;
			}

			was_delimiter = false;
		}
		
		text_cursor++;
	}

	text_cursor = 0;
	
	result.strings = (bbs_string*)&string_arena[arena_write_index];
	arena_write_index += sizeof(bbs_string)*result.count;
	u = 0;

	for (int i = 0; i < result.count; ++i)
	{
		result.strings[i].chars = (char*)&string_arena[arena_write_index];
		while(text_cursor <  text.length)
		{
			if(bbs_contains_char(delimiters, text.chars[text_cursor]))
			{
				was_delimiter = true;
				result.strings[i].length = u;
				u = 0;
				while(bbs_contains_char(delimiters, text.chars[text_cursor]))
					text_cursor++;
				break;
			}
			else
			{
				was_delimiter = false;
				result.strings[i].chars[u++] = text.chars[text_cursor];
			}
			
			text_cursor++;
		}

		arena_write_index += result.strings[i].length;
	}

	return result;
}

bool bbs_contains(bbs_string text, bbs_string substring)
{
	int text_cursor = 0;
	int substring_cursor = 0;
	while(text_cursor < text.length)
	{
		if(substring_cursor < substring.length)
		{
			if(text.chars[text_cursor] == substring.chars[substring_cursor])
				substring_cursor++;
			else substring_cursor = 0;
		}
		else 
			return true;
		text_cursor++;
	}

	return false;
}

int bbs_index_of(bbs_string text, bbs_string substring)
{
	int text_cursor = 0;
	int substring_cursor = 0;
	while(text_cursor < text.length)
	{
		if(substring_cursor < substring.length)
		{
			if(text.chars[text_cursor] == substring.chars[substring_cursor])
				substring_cursor++;
			else substring_cursor = 0;
		}
		else 
			return text_cursor-substring_cursor;
		text_cursor++;
	}

	return -1;
}

bbs_string bbs_substring(bbs_string	text, int index, int count)
{
	assert((index+count) < text.length);

	return (bbs_string){ .length = count, .chars = text.chars+index };
}

bbs_string bbs_replace(bbs_string text, bbs_string to_replace, bbs_string with)
{
	int index = bbs_index_of(text,to_replace);
	if(index > -1)
	{
		for (int i = 0; i < index; ++i)
			scratch_pad[i] = text.chars[i];
		for (int i = 0; i < with.length; ++i)
			scratch_pad[i+index] = with.chars[i];
		for (int i = index+with.length, text_index = index+to_replace.length; text_index < text.length; i++, text_index++)
			scratch_pad[i] = text.chars[text_index];

		char* result = &string_arena[arena_write_index];

		int n = bbs_c_string_length(scratch_pad);
		memcpy(result, scratch_pad, n);
		memset(scratch_pad, 0, n);

		arena_write_index += n;
		return (bbs_string){ .length = n, .chars = result };
	}

	return text; //if to_replace is not contained in the original text, return the original text unmodified
}

bbs_string bbs_delete_substring(bbs_string text, bbs_string substring)
{
	return bbs_replace(text, substring, (bbs_string){ .length = 0, .chars = NULL });
}

bbs_string bbs_delete_at(bbs_string text, int index, int count)
{
	assert((index+count) < text.length);

	char* result = &string_arena[arena_write_index];

	for (int i = 0; i < index; i++)
		result[i] = text.chars[i];

	for (int i = index+count; i < text.length; ++i)
		result[i-count] = text.chars[i];

	int n = text.length-count;
	arena_write_index += n;
	return (bbs_string){ .length = n, .chars = result };
}

bbs_string bbs_insert(bbs_string text, bbs_string substring, int index)
{
	assert(index < text.length);

	for (int i = 0; i < index; ++i)
		scratch_pad[i] = text.chars[i];

	for (int i = 0; i < substring.length; ++i)
		scratch_pad[i+index] = substring.chars[i];

	for (int i = index; i < text.length; ++i)
		scratch_pad[substring.length+i] = text.chars[i];
	
	char* result = &string_arena[arena_write_index];

	int n = bbs_c_string_length(scratch_pad);
	memcpy(result, scratch_pad, n);
	memset(scratch_pad, 0, n);

	arena_write_index += n;
	return (bbs_string){ .length = n, .chars = result };
}

bool bbs_string_equals(bbs_string a, bbs_string b)
{
	if(a.length != b.length)
		return false;

	for (int i = 0; i < a.length; ++i)
		if(a.chars[i] != b.chars[i])
			return false;

	return true;
}


bbs_string bbs_concatenate(bbs_string a, bbs_string b)
{
	char* result = &string_arena[arena_write_index];
	int total_length = a.length + b.length;

	for (int i = 0; i < a.length; ++i)
		result[i] = a.chars[i];

	for (int i = 0; i < b.length; ++i)
		result[i + a.length] = b.chars[i];

	arena_write_index += total_length;
	return (bbs_string){ .length = total_length, .chars = result };
}

int bbs_parse_int(bbs_string text) //assumes valid decimal integer string
{
    int digit_count = text.length;
    int result = 0;
    char look_up_place_value[10] = "0123456789";
    
    for (int i = 0; i < digit_count; ++i)
    {
        int tenspower = digit_count-(i+1);
        int place_value = 0;
        
        for (int x = 0; x < 10; ++x)
            if(text.chars[i] == look_up_place_value[x])
                place_value = x;

        for (int i = 0; i < tenspower; ++i)
            place_value*=10;

        result += place_value;
    }

    return result;
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