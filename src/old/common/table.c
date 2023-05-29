/*
 * Copyright (c) 2009, David J. Rager
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 * 
 * table.c
 *
 *  Created on: Mar 3, 2009
 *      Author: djrager
 */
#include "table.h"

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

// #define snprintf _snprintf

#define MAX(a, b) (a > b ? a : b)

#define TABLE_INIT_SIZE 64;

#define TABLE_ROW_SEPARATOR ((void*)-1)

struct table_t* table_init(FILE* file, const unsigned char* title, unsigned int verbose)
{
	struct table_t* tbl;

	if (title == NULL)
		goto error;
	if (file == NULL)
		goto error;

	tbl = (struct table_t*)malloc(sizeof(struct table_t));
	if (tbl == NULL)
		goto error;

	tbl->rows_buffer_size = TABLE_INIT_SIZE;

	tbl->rows_buffer = (unsigned char***)malloc(tbl->rows_buffer_size * sizeof(unsigned char**));
	if (tbl->rows_buffer == NULL)
		goto error1;
	memset(tbl->rows_buffer, 0, tbl->rows_buffer_size * sizeof(unsigned char**));

	tbl->title = (unsigned char*)strdup((const char *)title);
	if(tbl->title == NULL)
		goto error2;

	tbl->file = file;
	tbl->verbose = verbose;
	tbl->num_rows = 0;
	tbl->num_cols = 0;

	return tbl;

error2:
	free(tbl->rows_buffer);

error1:
	free(tbl);

error:
	return 0;
}

static int __attribute__((unused)) grow_table(struct table_t* tbl) 
{
	unsigned char*** tmp = (unsigned char***)malloc(tbl->rows_buffer_size * 2 * sizeof(unsigned char**));
	if (tmp == NULL)
		goto error;

	memset(tmp, 0, tbl->rows_buffer_size * 2 * sizeof(unsigned char**));
	memcpy(tmp, tbl->rows_buffer, tbl->rows_buffer_size * sizeof(unsigned char**));
	tbl->rows_buffer = tmp;
	tbl->rows_buffer_size *= 2;

	return 0;

error:
	return -1;
}

static unsigned char* get_hex_token(unsigned int width, unsigned int i);

static unsigned char* get_time_token(unsigned int i)
{
	const struct tm* tm;
	char* tmp = 0;
	unsigned int size = 32;
	unsigned int ret;

	if ((tm = localtime((time_t*)&i)) == 0)
	{
		return get_hex_token(8, i);
	}

	do
	{
		size *= 2;
		free(tmp);

		tmp = (char*)malloc(size);
		if (tmp == NULL)
			break;
		ret = strftime(tmp, size, "%A, %B %d, %Y, %I:%M:%S %p", gmtime((time_t*)&i));
	}
	while (ret == 0);

	return (unsigned char*)tmp;
}

static unsigned int count_escapes(const unsigned char* buf, unsigned int len)
{
	unsigned int count = 0;

	if (buf == 0)
		return 0;

	while (len > 0)
	{
		switch(*buf)
		{
		case '\a':
		case '\b':
		case '\f':
		case '\n':
		case '\r':
		case '\t':
		case '\v':
			count++;
			break;
		default:
			break;
		}

		buf++, len--;
	}

	return count;
}

static unsigned char* replace_escapes(const unsigned char* buf)
{
	unsigned int len, count;

	unsigned char* dest;
	unsigned int i = 0;

	if (buf == 0)
		return 0;

	len = strlen((const char*)buf);
	count = count_escapes(buf, len);

	dest = malloc(len + count + 1);
	if(dest == 0)
		return 0;

	while (len > 0)
	{
		switch(*buf)
		{
		case '\a':
			dest[i++] = '\\';
			dest[i++] = 'a';
			break;
		case '\b':
			dest[i++] = '\\';
			dest[i++] = 'b';
			break;
		case '\f':
			dest[i++] = '\\';
			dest[i++] = 'f';
			break;
		case '\n':
			dest[i++] = '\\';
			dest[i++] = 'n';
			break;
		case '\r':
			dest[i++] = '\\';
			dest[i++] = 'r';
			break;
		case '\t':
			dest[i++] = '\\';
			dest[i++] = 't';
			break;
		case '\v':
			dest[i++] = '\\';
			dest[i++] = 'v';
			break;
		default:
			dest[i++] = *buf;
			break;
		}

		buf++, len--;
	}

	dest[i] = 0;

	return dest;
}

static unsigned char* get_str_token(unsigned int width, const unsigned char* str)
{
	unsigned char* tmp;

	if (width > 0)
	{
		unsigned int i;

		tmp = (unsigned char*)malloc(width + 1);

		if (tmp != NULL)
		{
			i = 0;
			while ((i < width) && (str[i] != 0))
			{
				tmp[i] = str[i];
				i++;
			}
			tmp[i] = 0;
		}
	}
	else
		tmp = replace_escapes(str);

	return tmp;
}

static unsigned char* get_char_token(unsigned char c)
{
	unsigned char* str = (unsigned char*)malloc(2);
	if(str != NULL)
		snprintf((char *)str, 2, "%c", c);
	return str;
}

static unsigned char* get_int_token(unsigned int width, int i)
{
	unsigned char* str = 0;
	unsigned int oldsize, size = 2;
	int ret = 0;

	do
	{
		oldsize = size;
		str = (unsigned char*)malloc(size);
		if(str == NULL)
			break;

		ret = snprintf((char*)str, size, "%0*i", width, i);
		if((ret < 0) || ((unsigned int)ret >= size))
		{
			free(str);
			str = 0;
			size *= 2;
		}
	} while (((ret < 0) || ((unsigned int)ret >= oldsize)) && (size <= 16));

	return str;
}

static unsigned char* get_unsigned_token(unsigned int width, unsigned int i)
{
	unsigned char* str = 0;
	unsigned int oldsize, size = 2;
	int ret = 0;

	do
	{
		oldsize = size;
		str = (unsigned char*)malloc(size);
		if(str == NULL)
			break;

		ret = snprintf((char*)str, size, "%0*u", width, i);
		if((ret < 0) || ((unsigned int)ret >= size))
		{
			free(str);
			str = 0;
			size *= 2;
		}
	} while (((ret < 0) || ((unsigned int)ret >= oldsize)) && (size <= 16));

	return str;
}

static unsigned char* get_octal_token(unsigned int width, unsigned int i)
{
	unsigned char* str = 0;
	unsigned int oldsize, size = 3;
	int ret = 0;

	do
	{
		oldsize = size;
		str = (unsigned char*)malloc(size);
		if(str == NULL)
			break;

		ret = snprintf((char*)str, size, "0%0*o", width, i);
		if((ret < 0) || ((unsigned int)ret >= size))
		{
			free(str);
			str = 0;
			size = ret + 1;
		}
	} while (((ret < 0) || ((unsigned int)ret >= oldsize)) && (size <= 24));

	return str;
}

static unsigned char* get_hex_token(unsigned int width, unsigned int i)
{
	unsigned char* str = 0;
	unsigned int oldsize, size = 4;
	int ret = 0;

	do
	{
		oldsize = size;
		str = (unsigned char*)malloc(size);
		if(str == NULL)
			break;

		ret = snprintf((char*)str, size, "0x%0*x", width, i);
		if((ret < 0) || ((unsigned int)ret >= size))
		{
			free(str);
			str = 0;
			size *= 2;
		}
	} while (((ret < 0) || ((unsigned int)ret >= oldsize)) && (size <= 16));

	return str;
}

static unsigned char* get_float_token(unsigned int width, unsigned int precision, double f)
{
	unsigned char* str = 0;
	unsigned int oldsize, size = 4;
	int ret = 0;

	do
	{
		oldsize = size;
		str = (unsigned char*)malloc(size);
		if(str == NULL)
			break;

		ret = snprintf((char*)str, size, "%0*.*f", width, precision, f);
		if((ret < 0) || ((unsigned int)ret >= size))
		{
			free(str);
			str = 0;
			size = ret + 1;
		}
	} while (((ret < 0) || ((unsigned int)ret >= oldsize)) && (size <= 128));

	return str;
}

static int append_token(struct table_t* tbl, unsigned char*** row, unsigned int* toks, unsigned int* size, unsigned char* token)
{
	if(tbl == NULL) // should probably be assert
		goto error;
	if(row == NULL)
		goto error;
	if(*row == NULL)
		goto error;
	if(toks == NULL)
		goto error;
	if(size == NULL)
		goto error;
	if(*size == 0)
		goto error;
	if(token == NULL)
		goto error;

	if(*toks + 1 >= *size)
	{
		unsigned char** tmp = (unsigned char**)malloc(*size * 2 * sizeof(unsigned char*));
		if (tmp == NULL)
			goto error;

		memset(tmp, 0, *size * 2 * sizeof(unsigned char*));
		memcpy(tmp, *row, *toks * sizeof(unsigned char*));
		free(*row);
		*row = tmp;
		*size *= 2;
	}

	(*row)[(*toks)++] = token;

	if(*toks > tbl->num_cols)
		tbl->num_cols = *toks;

	return 0;

error:
	return -1;
}

int table_row(struct table_t* tbl, const unsigned char* format, ...)
{
	va_list args;
	unsigned char** tok_it;
	const unsigned char* tmp;
	unsigned char* tok = 0;
	unsigned char** row = 0;
	unsigned int ntoks = 0;
	unsigned int srow = 10;
	unsigned int width = 0;
	unsigned int precision = 0;

	if (tbl == NULL)
		goto error;
	if (format == NULL)
		goto error;

	row = (unsigned char**)malloc(srow * sizeof(unsigned char*));
	if(row == NULL)
		goto error;
	memset(row, 0, srow * sizeof(unsigned char*));

	if (tbl->num_rows >= tbl->rows_buffer_size)
	{
		unsigned char*** tmp = (unsigned char***)malloc(tbl->rows_buffer_size * 2 * sizeof(unsigned char**));
		if (tmp == NULL)
			goto error1;

		memset(tmp, 0, tbl->rows_buffer_size * 2 * sizeof(unsigned char**));
		memcpy(tmp, tbl->rows_buffer, tbl->rows_buffer_size * sizeof(unsigned char**));
		free(tbl->rows_buffer);
		tbl->rows_buffer = tmp;
		tbl->rows_buffer_size *= 2;
	}

	tmp = format;
	va_start (args, format);

	while(*tmp)
	{
		if(*tmp++ == '%')
		{
restart:
			switch(*tmp)
			{
			case 'S':
			case 's': // string
				tok = get_str_token(width, va_arg(args, unsigned char*));
				if(tok == NULL)
					goto error2;
				if(append_token(tbl, &row, &ntoks, &srow, tok) < 0)
					goto error2;
				width = precision = 0;
				break;
			case 'C':
			case 'c': // character
				tok = get_char_token(va_arg(args, int));
				if(tok == NULL)
					goto error2;
				if(append_token(tbl, &row, &ntoks, &srow, tok) < 0)
					goto error2;
				width = precision = 0;
				break;
			case 'd':
			case 'i': // integer - decimal
				tok = get_int_token(width, va_arg(args, int));
				if(tok == NULL)
					goto error2;
				if(append_token(tbl, &row, &ntoks, &srow, tok) < 0)
					goto error2;
				width = precision = 0;
				break;
			case 'u': // unsigned - decimal
				tok = get_unsigned_token(width, va_arg(args, unsigned int));
				if(tok == NULL)
					goto error2;
				if(append_token(tbl, &row, &ntoks, &srow, tok) < 0)
					goto error2;
				width = precision = 0;
				break;
			case 'o': // unsigned - octal
				tok = get_octal_token(width, va_arg(args, unsigned int));
				if(tok == NULL)
					goto error2;
				if(append_token(tbl, &row, &ntoks, &srow, tok) < 0)
					goto error2;
				width = precision = 0;
				break;
			case 't': // time
				tok = get_time_token(va_arg(args, unsigned int));
				if(tok == NULL)
					goto error2;
				if(append_token(tbl, &row, &ntoks, &srow, tok) < 0)
					goto error2;
				width = precision = 0;
				break;
			case 'X':
			case 'x': // unsigned - hex
				tok = get_hex_token(width, va_arg(args, unsigned int));
				if(tok == NULL)
					goto error2;
				if(append_token(tbl, &row, &ntoks, &srow, tok) < 0)
					goto error2;
				width = precision = 0;
				break;
			case 'G':
			case 'g':
			case 'F':
			case 'f': // double
				tok = get_float_token(width, precision, va_arg(args, double));
				if(tok == NULL)
					goto error2;
				if(append_token(tbl, &row, &ntoks, &srow, tok) < 0)
					goto error2;
				width = precision = 0;
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				do
				{
					width = width * 10 + (unsigned int)*tmp - '0';
					tmp++;
				}
				while ((*tmp >= '0') && (*tmp <= '9'));

				switch(*tmp)
				{
				case '\0':
					goto error2;
					break;
				case '.':
doprecision:
					tmp++;
					switch(*tmp)
					{
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						do
						{
							precision = precision * 10 + (unsigned int)*tmp - '0';
							tmp++;
						}
						while ((*tmp >= '0') && (*tmp <= '9'));

						break;
					default:
						goto error2;
						break;
					}
					break;
				}

				goto restart;
				break;
			case '.':
				goto doprecision;
				break;
			case '\0':
				goto error2;
				break;
			default:
				break;
			}

			tmp++;
		}
		else
			goto error2;
	}

	tbl->rows_buffer[tbl->num_rows++] = row;
	return 0;

error2:
	tok_it = row;
	while(*tok_it)
		free((void*)*tok_it++);

error1:
	free(row);

error:
	return -1;
}

int table_separator(struct table_t* tbl)
{
	if (tbl == NULL)
		return -1;

	if ((tbl->num_rows > 0) && (tbl->rows_buffer[tbl->num_rows - 1] == TABLE_ROW_SEPARATOR))
		return 0;

	if (tbl->num_rows >= tbl->rows_buffer_size)
	{
		unsigned char*** tmp = (unsigned char***)malloc(tbl->rows_buffer_size * 2 * sizeof(unsigned char**));
		if (tmp == NULL)
			goto error;

		memset(tmp, 0, tbl->rows_buffer_size * 2 * sizeof(unsigned char**));
		memcpy(tmp, tbl->rows_buffer, tbl->rows_buffer_size * sizeof(unsigned char**));
		free(tbl->rows_buffer);
		tbl->rows_buffer = tmp;
		tbl->rows_buffer_size *= 2;
	}

	tbl->rows_buffer[tbl->num_rows++] = TABLE_ROW_SEPARATOR;
	return 0;

error:
	return -1;
}

void free_table(struct table_t* tbl)
{
	unsigned char** tok_it;		// token iterator
	unsigned int i;

	if (tbl == NULL)
		return;

	for(i = 0; i < tbl->num_rows; i++)	// for each row
	{
		tok_it = tbl->rows_buffer[i];		// get the first token from the row
		if (tok_it == TABLE_ROW_SEPARATOR)	// skip row separator markers
			continue;

		while(*tok_it)				// while there are tokens
			free((void*)*tok_it++);		// free the token
		free((void*)tbl->rows_buffer[i]);	// free the row
	}

	// free the table
	free(tbl->title);
	free(tbl->rows_buffer);
	free(tbl);
}

// CSV format based on RFC 4180, October 2005
// http://tools.ietf.org/html/rfc4180
static void print_csv_field(FILE* file, const unsigned char* field)
{
	int i;
	i = strcspn((const char*)field, "\r\n\",");
	if(i == strlen((const char*)field))
		fprintf(file, "%s", field);
	else
	{
		fprintf(file, "\"");
		for(i = 0; i < strlen((const char*)field); i++)
		{
			if(field[i] == '"')
				fprintf(file, "\"\"");
			else
				fprintf(file, "%c", field[i]);
		}
		fprintf(file, "\"");
	}
}

int print_csv(struct table_t* tbl)
{
	unsigned char** tok_it;		// token iterator
	unsigned int i;

	fprintf(tbl->file, "%s\n", tbl->title);

	for (i = 0; i < tbl->num_rows; i++)
	{
		unsigned int col = 0;

		tok_it = tbl->rows_buffer[i];
		if (tok_it == TABLE_ROW_SEPARATOR)
			continue;

		for (col = 0; col < tbl->num_cols; col++)
		{
			if(*tok_it)
				print_csv_field(tbl->file, *tok_it);

			if (col < tbl->num_cols - 1)
				fprintf(tbl->file, ",");

			if(*tok_it)
				tok_it++;
		}

		fprintf(tbl->file, "\r\n");
	}

	return 0;
}

#ifdef TABLE_FMT_EXT

static const char* g_ul = "\311";	// upper left
static const char* g_ur = "\273";	// upper right
static const char* g_bl = "\310";	// bottom left
static const char* g_br = "\274";	// bottom right
static const char* g_h =  "\315";	// horizontal
static const char* g_v =  "\272";	// vertical
static const char* g_ls = "\314";	// left separator
static const char* g_rs = "\271";	// right separator
static const char* g_us = "\313";	// upper separator
static const char* g_bs = "\312";	// bottom separator
static const char* g_cs = "\316";	// center separator

#else

static const char* g_ul = "+";		// upper left
static const char* g_ur = "+";		// upper right
static const char* g_bl = "+";		// bottom left
static const char* g_br = "+";		// bottom right
static const char* g_h =  "-";		// horizontal
static const char* g_v =  "|";		// vertical
static const char* g_ls = "+";		// left separator
static const char* g_rs = "+";		// right separator
static const char* g_us = "+";		// upper separator
static const char* g_bs = "+";		// bottom separator
static const char* g_cs = "+";		// center separator

#endif

static int print_table_title(struct table_t* tbl, unsigned int* colwidths)
{
	int i, j;
	int len = 0;
	fprintf(tbl->file, "%s",  g_ul);

	for (i = 0; i < tbl->num_cols; i++)
	{
		len += colwidths[i] + 2;

		for (j = 0; j < colwidths[i] + 2; j++)
			fprintf(tbl->file, "%s", g_h);

		if(i < tbl->num_cols - 1)
			fprintf(tbl->file, "%s", g_h), len++;
	}

	fprintf(tbl->file, "%s", g_ur);
	fprintf(tbl->file, "\n");

	fprintf(tbl->file, "%s", g_v);
	fprintf(tbl->file, " %s", tbl->title);

	len = len - strlen((char*)tbl->title) - 1;
	if (len < 0)
		len = 0;

	for (i = 0; i < (unsigned int)len; i++)
		fprintf(tbl->file, " ");

	if (len != 0)
		fprintf(tbl->file, "%s", g_v);

	fprintf(tbl->file, "\n");
	return 0;
}
static int print_table_usep(struct table_t* tbl, unsigned int* colwidths)
{
	int i, j;
	fprintf(tbl->file, "%s", g_ls);
	for (i = 0; i < tbl->num_cols; i++)
	{
		for (j = 0; j < colwidths[i] + 2; j++)
			fprintf(tbl->file,"%s", g_h);

		if(i < tbl->num_cols - 1)
			fprintf(tbl->file,"%s", g_us);
	}
	fprintf(tbl->file, "%s", g_rs);
	fprintf(tbl->file, "\n");

	return 0;
}

static int print_table_csep(struct table_t* tbl, unsigned int* colwidths)
{
	int i, j;
	fprintf(tbl->file, "%s", g_ls);
	for (i = 0; i < tbl->num_cols; i++)
	{
		for (j = 0; j < colwidths[i] + 2; j++)
			fprintf(tbl->file, "%s", g_h);

		if(i < tbl->num_cols - 1)
			fprintf(tbl->file, "%s", g_cs);
	}
	fprintf(tbl->file, "%s", g_rs);
	fprintf(tbl->file, "\n");

	return 0;
}

static int print_table_bsep(struct table_t* tbl, unsigned int* colwidths)
{
	int i, j;
	fprintf(tbl->file, "%s", g_bl);
	for (i = 0; i < tbl->num_cols; i++)
	{
		for (j = 0; j < colwidths[i] + 2; j++)
			fprintf(tbl->file, "%s", g_h);

		if(i < tbl->num_cols - 1)
			fprintf(tbl->file, "%s", g_bs);
	}
	fprintf(tbl->file, "%s", g_br);
	fprintf(tbl->file, "\n");

	return 0;
}

int print_table(struct table_t* tbl)
{
	unsigned char** tok_it;		// token iterator
	unsigned int i, j;
	unsigned int* colwidths = 0;
	int len;

	colwidths = (unsigned int*)malloc(tbl->num_cols * sizeof(unsigned int));
	if (colwidths == NULL)
		return 0;

	memset(colwidths, 0, tbl->num_cols * sizeof(unsigned int));

	for (i = 0; i < tbl->num_rows; i++)
	{
		unsigned int col = 0;
		tok_it = tbl->rows_buffer[i];
		if(tok_it == TABLE_ROW_SEPARATOR)
			continue;

		while(*tok_it)
		{
			colwidths[col] = MAX(colwidths[col], strlen((char*)*tok_it));
			col++, tok_it++;
		}
	}

	print_table_title(tbl, colwidths);
	print_table_usep(tbl, colwidths);

	for (i = 0; i < tbl->num_rows; i++)
	{
		unsigned int col = 0;

		tok_it = tbl->rows_buffer[i];
		if (tok_it == TABLE_ROW_SEPARATOR)
		{
			print_table_csep(tbl, colwidths);
			continue;
		}

		fprintf(tbl->file, "%s", g_v);

		for (col = 0; col < tbl->num_cols; col++)
		{
			if(*tok_it)
			{
				fprintf(tbl->file, " %s ", *tok_it);
				len = strlen((const char*)*tok_it);
			}
			else
			{
				fprintf(tbl->file, "  ");
				len = 0;
			}

			for (j = 0; j < colwidths[col] - len; j++)
				fprintf(tbl->file, " ");

			fprintf(tbl->file, "%s", g_v);

			if (*tok_it)
				tok_it++;
		}

		fprintf(tbl->file, "\n");
	}

	print_table_bsep(tbl, colwidths);

	fprintf(tbl->file, "\n");

	if(colwidths)
		free((void*)colwidths);

	return 0;
}

int table_commit(struct table_t* tbl)
{
	if(tbl == NULL)
		return 0;

	if(tbl->num_cols == 0)
		goto out;

	if(tbl->verbose)
		print_table(tbl);
	else
		print_csv(tbl);

out:
	free_table(tbl);

	return 0;
}


int table_abort(struct table_t* tbl)
{
	free_table(tbl);

	return 0;
}

#ifdef STANDALONE

int main(int argc, char* argv[])
{
	struct table_t* table = table_init(stdout, "Test Table", 1);

	table_row(table, "%s%s", "Column 1", "Column 2");

	table_separator(table);

	table_row(table, "%s%s", "String value", "test string");
	table_row(table, "%s%c", "Character value", 'c');
	table_row(table, "%s%d", "Signed integer", -1);
	table_row(table, "%s%u", "Unsigned integer", 42);
	table_row(table, "%s%o", "Octal integer", 511);
	table_row(table, "%s%8x", "Hexadecimal integer", 255);
	table_row(table, "%s%.5f", "Floating point", 3.14159);
	table_row(table, "%s%t", "Timestamp", 0);

	table_commit(table);

	return 0;
}

#endif

