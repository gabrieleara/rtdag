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
 * table.h
 *
 *  Created on: Mar 3, 2009
 *      Author: djrager
 * 
 * Usage Example:
 *   * https://www.fourthwoods.com/utils/tableformatter/ 
 * 
 */
#ifndef _TABLE_H_
#define _TABLE_H_

#include <stdio.h>

#ifdef  __cplusplus
extern "C" {
#endif

/*!
 * This is the table structure.
 *
 * This is the actual table structure. There should be no reason to access
 * the fields of this structure directly.
 */
struct table_t
{
	FILE* file;
	unsigned int verbose;

	unsigned char* title;

	unsigned int rows_buffer_size;
	unsigned char*** rows_buffer;

	unsigned int num_rows;
	unsigned int num_cols;
};

/*!
 * /brief This function initializes a table structure.
 *
 * This function initializes a table structure to be used to format data. The data
 * will be output in either CSV format or a pretty printed table depending on the
 * value of the verbose parameter.
 *
 * Once the table is initialized, rows are added using the table_row() function.
 *
 * When data is done being added to the table, it is printed using the table_commit()
 * function.
 *
 * /param file - The file to which to print the table data. This can be a file opened
 *  with fopen or the special stdout handle.
 * /param title - The title for the table. Cannot be NULL.
 * /param verbose - Specifies how detailed the output will be. A value of 0 will print
 *  the output in CSV format. A value greater than 0 will print the output in a pretty
 *  printed table.
 *
 * /return - Returns 0 if successful, non-zero on error.
 */
struct table_t* table_init(FILE* file, const unsigned char* title, unsigned int verbose);

/*!
 * /brief This function inserts a row into the table.
 *
 * This function inserts formatted data into a row in the table.
 *
 * Each column in the row is specified by a position in the format string. For example
 * the first format specifier is the first column, second column two, etc.
 *
 * The format parameter takes a C style format string. The acceptable format specifiers
 * are a (mostly) subset of the printf format specifiers with the addition of 't' for
 * a timestamp.
 *
 * Format specifiers:
 *
 * specifier     | Output
 * -------------------------------
 *  s or S       | String of characters
 *  c or C       | Character
 *  d or i       | Signed decimal integer
 *  u            | Unsigned decimal integer
 *  o            | Unsigned octal integer
 *  t            | Timestamp specified by a time_t value
 *  x or X       | Unsigned hexadecimal integer
 *  f, F, g or G | Decimal floating point
 *
 *  [width].[precision] are also supported. Note that precision defaults
 *  to 0 so with floating point values it must always be specified or the
 *  value will be truncated to a whole number.
 *
 * /param tbl - a pointer to the initialized table structure.
 * /param format - C style format specifiers
 * /param ... - the list of parameters to be formatted.
 *
 * /return - Returns 0 if successful, non-zero on error.
 */
int table_row(struct table_t* tbl, const unsigned char* format, ...);

/*!
 * This function inserts a separator into the table.
 *
 * This function inserts a separator line into the table to separate rows of
 * data. This separator is ignored when printing CSV output.
 *
 * /param tbl - a pointer to the initialized table structure.
 *
 * /return - Returns 0 if successful, non-zero on error.
 */
int table_separator(struct table_t* tbl);

/*!
 * This function prints the table to the output file.
 *
 * This file prints the table to the output file. Once the table is printed
 * the structure is free'd and the passed in table pointer becomes invalid.
 * Subsequent calls to the table_* functions are undefined.
 *
 * /return - Returns 0 if successful, non-zero on error.
 */
int table_commit(struct table_t* tbl);

/*!
 * This function frees the table structure without printing it to the output file.
 *
 * This function frees the table structure without printing it to the output
 * file. Call this function to release table resources if the table will not be
 * printed.
 *
 * /return - Returns 0 if successful, non-zero on error.
 */
int table_abort(struct table_t* tbl);

void free_table(struct table_t* tbl);

int print_table(struct table_t* tbl);

int print_csv(struct table_t* tbl);

#ifdef  __cplusplus
}
#endif

#endif//_TABLE_H_
