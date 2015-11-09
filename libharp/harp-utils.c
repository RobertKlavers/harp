/*
 * Copyright (C) 2015 S[&]T, The Netherlands.
 *
 * This file is part of HARP.
 *
 * HARP is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * HARP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HARP; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "harp-internal.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/** Returns the name of a data type.
 * \ingroup harp_general
 * \param data_type HARP basic data type
 * \return if the data type is known a string containing the name of the type, otherwise the string "unknown".
 */
const char *harp_get_data_type_name(harp_data_type data_type)
{
    switch (data_type)
    {
        case harp_type_int8:
            return "int8";
        case harp_type_int16:
            return "int16";
        case harp_type_int32:
            return "int32";
        case harp_type_float:
            return "float";
        case harp_type_double:
            return "double";
        case harp_type_string:
            return "string";
    }
    return "unknown";
}

/** Retrieve the byte size for a HARP data type.
 * \ingroup harp_general
 * \param data_type Data type for which to retrieve the size.
 * \return The size of the data type in bytes.
 */
long harp_get_size_for_type(harp_data_type data_type)
{
    switch (data_type)
    {
        case harp_type_int8:
            return (long)sizeof(int8_t);
        case harp_type_int16:
            return (long)sizeof(int16_t);
        case harp_type_int32:
            return (long)sizeof(int32_t);
        case harp_type_float:
            return (long)sizeof(float);
        case harp_type_double:
            return (long)sizeof(double);
        case harp_type_string:
            return (long)sizeof(char *);
    }

    return (long)(-1);
}

/** Retrieve the fill value for a HARP data type.
 * \param data_type Data type for which to retrieve the fill value.
 * \return The fill value for the data type.
 */
harp_scalar harp_get_fill_value_for_type(harp_data_type data_type)
{
    harp_scalar fill_value;

    switch (data_type)
    {
        case harp_type_int8:
            fill_value.int8_data = 0;
            break;
        case harp_type_int16:
            fill_value.int16_data = 0;
            break;
        case harp_type_int32:
            fill_value.int32_data = 0;
            break;
        case harp_type_float:
            fill_value.float_data = harp_nan();
            break;
        case harp_type_double:
            fill_value.double_data = harp_nan();
            break;
        default:
            assert(0);
            exit(1);
    }

    return fill_value;
}

/** Retrieve the minimum valid value for a HARP data type.
 * \param data_type Data type for which to retrieve the minimum valid value.
 * \return The minimum valid value of the data type.
 */
harp_scalar harp_get_valid_min_for_type(harp_data_type data_type)
{
    harp_scalar valid_min;

    switch (data_type)
    {
        case harp_type_int8:
            valid_min.int8_data = -128;
            break;
        case harp_type_int16:
            valid_min.int16_data = -32768;
            break;
        case harp_type_int32:
            valid_min.int32_data = -2147483647 - 1;
            break;
        case harp_type_float:
            valid_min.float_data = harp_mininf();
            break;
        case harp_type_double:
            valid_min.double_data = harp_mininf();
            break;
        default:
            assert(0);
            exit(1);
    }

    return valid_min;
}

/** Retrieve the maximum valid value for a HARP data type.
 * \param data_type Data type for which to retrieve the maximum valid value.
 * \return The maximum valid value of the data type.
 */
harp_scalar harp_get_valid_max_for_type(harp_data_type data_type)
{
    harp_scalar valid_max;

    switch (data_type)
    {
        case harp_type_int8:
            valid_max.int8_data = 127;
            break;
        case harp_type_int16:
            valid_max.int16_data = 32767;
            break;
        case harp_type_int32:
            valid_max.int32_data = 2147483647;
            break;
        case harp_type_float:
            valid_max.float_data = harp_plusinf();
            break;
        case harp_type_double:
            valid_max.double_data = harp_plusinf();
            break;
        default:
            assert(0);
            exit(1);
    }

    return valid_max;
}

/** Test if \a value equals the fill value for the specified data type.
 * \param  data_type Data type corresponding to the value of \a value.
 * \param  value Value to test.
 * \return
 *   \arg \c 0, Value is not equal to the fill value.
 *   \arg \c 1, Value equals the fill value.
 */
int harp_is_fill_value_for_type(harp_data_type data_type, harp_scalar value)
{
    switch (data_type)
    {
        case harp_type_int8:
            return value.int8_data == 0;
        case harp_type_int16:
            return value.int16_data == 0;
        case harp_type_int32:
            return value.int32_data == 0;
        case harp_type_float:
            return harp_isnan(value.float_data);
        case harp_type_double:
            return harp_isnan(value.double_data);
        default:
            assert(0);
            exit(1);
    }
}

/** Test if \a value equals the minimum valid value for the specified data type.
 * \param  data_type Data type corresponding to the value of \a value.
 * \param  value Value to test.
 * \return
 *   \arg \c 0, Value is not equal to the minimum valid value.
 *   \arg \c 1, Value equals the minimum valid value.
 */
int harp_is_valid_min_for_type(harp_data_type data_type, harp_scalar value)
{
    switch (data_type)
    {
        case harp_type_int8:
            return value.int8_data == -128;
        case harp_type_int16:
            return value.int16_data == -32768;
        case harp_type_int32:
            return value.int32_data == -2147483647 - 1;
        case harp_type_float:
            return harp_ismininf(value.float_data);
        case harp_type_double:
            return harp_ismininf(value.double_data);
        default:
            assert(0);
            exit(1);
    }
}

/** Test if \a value equals the maximum valid value for the specified data type.
 * \param  data_type Data type corresponding to the value of \a value.
 * \param  value Value to test.
 * \return
 *   \arg \c 0, Value is not equal to the maximum valid value.
 *   \arg \c 1, Value equals the maximum valid value.
 */
int harp_is_valid_max_for_type(harp_data_type data_type, harp_scalar value)
{
    switch (data_type)
    {
        case harp_type_int8:
            return value.int8_data == 127;
        case harp_type_int16:
            return value.int16_data == 32767;
        case harp_type_int32:
            return value.int32_data == 2147483647;
        case harp_type_float:
            return harp_isplusinf(value.float_data);
        case harp_type_double:
            return harp_isplusinf(value.double_data);
        default:
            assert(0);
            exit(1);
    }
}

/** Find out whether a double value is a finite number (i.e. not NaN and not infinite).
 * \ingroup harp_general
 * \param x  A double value.
 * \return
 *   \arg \c 1, The double value is a finite number.
 *   \arg \c 0, The double value is not a finite number.
 */
int harp_isfinite(double x)
{
    return (!harp_isnan(x) && !harp_isinf(x));
}

/** Find out whether a double value equals NaN (Not a Number).
 * \ingroup harp_general
 * \param x  A double value.
 * \return
 *   \arg \c 1, The double value equals NaN.
 *   \arg \c 0, The double value does not equal NaN.
 */
int harp_isnan(double x)
{
    uint64_t e_mask, f_mask;

    union
    {
        uint64_t as_int;
        double as_double;
    } mkNaN;

    mkNaN.as_double = x;

    e_mask = 0x7ff0;
    e_mask <<= 48;

    if ((mkNaN.as_int & e_mask) != e_mask)
        return 0;       /* e != 2047 */

    f_mask = 1;
    f_mask <<= 52;
    f_mask--;

    /* number is NaN if f does not equal zero. */
    return (mkNaN.as_int & f_mask) != 0;
}

/** Retrieve a double value that respresents NaN (Not a Number).
 * \ingroup harp_general
 * \return The double value 'NaN'.
 */
double harp_nan(void)
{
    union
    {
        uint64_t as_int;
        double as_double;
    } mkNaN;

    mkNaN.as_int = 0x7ff8;
    mkNaN.as_int <<= 48;

    return mkNaN.as_double;
}

/** Find out whether a double value equals inf (either positive or negative infinity).
 * \ingroup harp_general
 * \param x  A double value.
 * \return
 *   \arg \c 1, The double value equals inf.
 *   \arg \c 0, The double value does not equal inf.
 */
int harp_isinf(double x)
{
    return harp_isplusinf(x) || harp_ismininf(x);
}

/** Find out whether a double value equals +inf (positive infinity).
 * \ingroup harp_general
 * \param x  A double value.
 * \return
 *   \arg \c 1, The double value equals +inf.
 *   \arg \c 0, The double value does not equal +inf.
 */
int harp_isplusinf(double x)
{
    uint64_t plusinf;

    union
    {
        uint64_t as_int;
        double as_double;
    } mkInf;

    mkInf.as_double = x;

    plusinf = 0x7ff0;
    plusinf <<= 48;

    return mkInf.as_int == plusinf;
}

/** Find out whether a double value equals -inf (negative infinity).
 * \ingroup harp_general
 * \param x  A double value.
 * \return
 *   \arg \c 1, The double value equals -inf.
 *   \arg \c 0, The double value does not equal -inf.
 */
int harp_ismininf(double x)
{
    uint64_t mininf;

    union
    {
        uint64_t as_int;
        double as_double;
    } mkInf;

    mkInf.as_double = x;

    mininf = 0xfff0;
    mininf <<= 48;

    return mkInf.as_int == mininf;
}

/** Retrieve a double value that respresents +inf (positive infinity).
 * \ingroup harp_general
 * \return The double value '+inf'.
 */
double harp_plusinf(void)
{
    union
    {
        uint64_t as_int;
        double as_double;
    } mkInf;

    mkInf.as_int = 0x7ff0;
    mkInf.as_int <<= 48;

    return mkInf.as_double;
}

/** Retrieve a double value that respresents -inf (negative infinity).
 * \ingroup harp_general
* \return The double value '-inf'.
*/
double harp_mininf(void)
{
    union
    {
        uint64_t as_int;
        double as_double;
    } mkInf;

    mkInf.as_int = 0xfff0;
    mkInf.as_int <<= 48;

    return mkInf.as_double;
}

/** Write 64 bit signed integer to a string.
 * \ingroup harp_general
 * The string \a s will be 0 terminated.
 * \param a  A signed 64 bit integer value.
 * \param s  A character buffer that is at least 21 bytes long.
 */
void harp_str64(int64_t a, char *s)
{
    if (a < 0)
    {
        s[0] = '-';
        harp_str64u((uint64_t)(-a), &s[1]);
    }
    else
    {
        harp_str64u((uint64_t)a, s);
    }
}

/** Write 64 bit unsigned integer to a string.
 * \ingroup harp_general
 * The string \a s will be 0 terminated.
 * \param a  An unsigned 64 bit integer value.
 * \param s  A character buffer that is at least 21 bytes long.
 */
void harp_str64u(uint64_t a, char *s)
{
    if (a <= 4294967295UL)
    {
        sprintf(s, "%ld", (long)a);
    }
    else
    {
        long a1, a2;

        a1 = (long)(a % 100000000);
        a /= 100000000;
        a2 = (long)(a % 100000000);
        a /= 100000000;
        if (a != 0)
        {
            sprintf(s, "%ld%08ld%08ld", (long)a, a2, a1);
        }
        else
        {
            sprintf(s, "%ld%08ld", a2, a1);
        }
    }
}

/** Compute the number of elements from a list of dimension lengths.
 * \param num_dimensions Number of dimensions.
 * \param dimension Dimension lengths.
 * \return Number of elements (i.e. the product of the specified dimension lengths, or \c 1 if \a num_dimensions equals
 *   \c 0).
 */
long harp_get_num_elements(int num_dimensions, const long *dimension)
{
    long num_elements;
    int i;

    num_elements = 1;
    for (i = 0; i < num_dimensions; i++)
    {
        num_elements *= dimension[i];
    }

    return num_elements;
}

char *harp_array_get_char_array_from_strings(harp_array data, long num_elements, long *max_string_length)
{
    char *buffer;
    long max_length;
    long i;

    max_length = harp_array_get_max_string_length(data, num_elements);
    buffer = malloc((size_t)num_elements * max_length * sizeof(char));
    if (buffer == NULL)
    {
        harp_set_error(HARP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       num_elements * max_length * sizeof(char), __FILE__, __LINE__);
        return NULL;
    }
    memset(buffer, 0, (size_t)(num_elements * max_length) * sizeof(char));
    for (i = 0; i < num_elements; i++)
    {
        if (data.string_data[i] != NULL)
        {
            memcpy(&buffer[i * max_length], data.string_data[i], strlen(data.string_data[i]));
        }
    }

    if (max_string_length != NULL)
    {
        *max_string_length = max_length;
    }

    return buffer;
}

long harp_array_get_max_string_length(harp_array data, long num_elements)
{
    long max_length;
    long i;

    max_length = 0;
    for (i = 0; i < num_elements; i++)
    {
        if (data.string_data[i] != NULL)
        {
            long length = (long)strlen(data.string_data[i]);

            if (length > max_length)
            {
                max_length = length;
            }
        }
    }

    return max_length;
}

static void fill_int8(long num_elements, int8_t *data, int8_t value)
{
    int8_t *last;

    last = data + num_elements;
    while (data != last)
    {
        *data = value;
        ++data;
    }
}

static void fill_int16(long num_elements, int16_t *data, int16_t value)
{
    int16_t *last;

    last = data + num_elements;
    while (data != last)
    {
        *data = value;
        ++data;
    }
}

static void fill_int32(long num_elements, int32_t *data, int32_t value)
{
    int32_t *last;

    last = data + num_elements;
    while (data != last)
    {
        *data = value;
        ++data;
    }
}

static void fill_float(long num_elements, float *data, float value)
{
    float *last;

    last = data + num_elements;
    while (data != last)
    {
        *data = value;
        ++data;
    }
}

static void fill_double(long num_elements, double *data, double value)
{
    double *last;

    last = data + num_elements;
    while (data != last)
    {
        *data = value;
        ++data;
    }
}

static void null_string(long num_elements, char **data)
{
    char **last;

    last = data + num_elements;
    while (data != last)
    {
        if (*data != NULL)
        {
            free(*data);
            *data = NULL;
        }

        ++data;
    }
}

/** Fill an array with the default HARP fill value for the specified data type.
 * \param data_type Data type of the array.
 * \param num_elements Number of elements in the array.
 * \param data Array that should be nulled.
 */
void harp_array_null(harp_data_type data_type, long num_elements, harp_array data)
{
    switch (data_type)
    {
        case harp_type_int8:
            fill_int8(num_elements, data.int8_data, 0);
            break;
        case harp_type_int16:
            fill_int16(num_elements, data.int16_data, 0);
            break;
        case harp_type_int32:
            fill_int32(num_elements, data.int32_data, 0);
            break;
        case harp_type_float:
            fill_float(num_elements, data.float_data, harp_nan());
            break;
        case harp_type_double:
            fill_double(num_elements, data.double_data, harp_nan());
            break;
        case harp_type_string:
            null_string(num_elements, data.string_data);
            break;
        default:
            assert(0);
            exit(1);
    }
}

/** Replace each occurence of a specific (fill) value in an array with the default HARP fill value for the specified
 *  data type.
 * \param data_type Data type of the array.
 * \param num_elements Number of elements in the array.
 * \param data Array to operate on.
 * \param fill_value Value to be replaced by the default HARP fill value for \a data_type.
 */
void harp_array_replace_fill_value(harp_data_type data_type, long num_elements, harp_array data, harp_scalar fill_value)
{
    harp_scalar harp_fill_value;
    long i;

    if (harp_is_fill_value_for_type(data_type, fill_value))
    {
        return;
    }

    harp_fill_value = harp_get_fill_value_for_type(data_type);
    switch (data_type)
    {
        case harp_type_int8:
            for (i = 0; i < num_elements; i++)
            {
                if (data.int8_data[i] == fill_value.int8_data)
                {
                    data.int8_data[i] = harp_fill_value.int8_data;
                }
            }
            break;
        case harp_type_int16:
            for (i = 0; i < num_elements; i++)
            {
                if (data.int16_data[i] == fill_value.int16_data)
                {
                    data.int16_data[i] = harp_fill_value.int16_data;
                }
            }
            break;
        case harp_type_int32:
            for (i = 0; i < num_elements; i++)
            {
                if (data.int32_data[i] == fill_value.int32_data)
                {
                    data.int32_data[i] = harp_fill_value.int32_data;
                }
            }
            break;
        case harp_type_float:
            for (i = 0; i < num_elements; i++)
            {
                if (data.float_data[i] == fill_value.float_data)
                {
                    data.float_data[i] = harp_fill_value.float_data;
                }
            }
            break;
        case harp_type_double:
            for (i = 0; i < num_elements; i++)
            {
                if (data.double_data[i] == fill_value.double_data)
                {
                    data.double_data[i] = harp_fill_value.double_data;
                }
            }
            break;
        default:
            assert(0);
            exit(1);
    }
}

/** Invert the array accross a given dimension
 * \param data_type Data type of the array.
 * \param dim_id Index of the dimension that should be inverted.
 * \param num_dimensions Number of dimensions in the array.
 * \param dimension Dimension sizes of the array.
 * \param data Array that should have a dimension inverted.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #harp_errno).
 */
int harp_array_invert(harp_data_type data_type, int dim_id, int num_dimensions, long dimension[HARP_MAX_NUM_DIMS],
                      harp_array data)
{
    long num_elements;
    long block_size;
    long length;
    long i, j;

    assert(dim_id >= 0 && dim_id < num_dimensions);

    /* total byte size of array = num_elements * length * block_size */

    num_elements = 1;
    for (i = 0; i < dim_id; i++)
    {
        num_elements *= dimension[i];
    }

    length = dimension[dim_id];
    if (length <= 1)
    {
        /* nothing to do */
        return 0;
    }

    block_size = harp_get_size_for_type(data_type);
    for (i = dim_id + 1; i < num_dimensions; i++)
    {
        block_size *= dimension[i];
    }

    if (block_size == 1)
    {
        for (i = 0; i < num_elements; i++)
        {
            int8_t *block = &data.int8_data[i * length];

            for (j = 0; j < length / 2; j++)
            {
                int8_t temp = block[j];

                block[j] = block[length - 1 - j];
                block[length - 1 - j] = temp;
            }
        }
    }
    else if (block_size == 2)
    {
        for (i = 0; i < num_elements; i++)
        {
            int16_t *block = &data.int16_data[i * length];

            for (j = 0; j < length / 2; j++)
            {
                int16_t temp = block[j];

                block[j] = block[length - 1 - j];
                block[length - 1 - j] = temp;
            }
        }
    }
    else if (block_size == 4)
    {
        for (i = 0; i < num_elements; i++)
        {
            int32_t *block = &data.int32_data[i * length];

            for (j = 0; j < length / 2; j++)
            {
                int32_t temp = block[j];

                block[j] = block[length - 1 - j];
                block[length - 1 - j] = temp;
            }
        }
    }
    else if (block_size == 8)
    {
        for (i = 0; i < num_elements; i++)
        {
            double *block = &data.double_data[i * length];

            for (j = 0; j < length / 2; j++)
            {
                double temp = block[j];

                block[j] = block[length - 1 - j];
                block[length - 1 - j] = temp;
            }
        }
    }
    else
    {
        uint8_t *buffer;

        buffer = (uint8_t *)malloc(length * block_size);
        if (buffer == NULL)
        {
            harp_set_error(HARP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           dimension[dim_id] * block_size, __FILE__, __LINE__);
            return -1;
        }
        for (i = 0; i < num_elements; i++)
        {
            int8_t *block = &data.int8_data[i * length * block_size];

            for (j = 0; j < length; j++)
            {
                memcpy(&buffer[j * block_size], &block[(length - 1 - j) * block_size], block_size);
            }
            memcpy(block, buffer, length * block_size);
        }
        free(buffer);
    }

    return 0;
}

/** Transpose the contents of an array
 * \param data_type Data type of the array.
 * \param num_dimensions Number of dimensions in the array.
 * \param dimension Dimension sizes of the array.
 * \param data Array that should be transposed.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #harp_errno).
 */
int harp_array_transpose(harp_data_type data_type, int num_dimensions, long dimension[HARP_MAX_NUM_DIMS],
                         harp_array data)
{
    long num_elements;
    long multiplier[HARP_MAX_NUM_DIMS + 1];
    long rsub[HARP_MAX_NUM_DIMS + 1];   /* reversed index in multi dim array */
    long rdim[HARP_MAX_NUM_DIMS + 1];   /* reversed order of dim[] */
    long element_size;
    long index = 0;
    long i;
    uint8_t *src;
    uint8_t *dst;

    if (num_dimensions <= 1)
    {
        /* nothing to do */
        return 0;
    }

    element_size = harp_get_size_for_type(data_type);

    src = (uint8_t *)data.ptr;

    num_elements = 1;
    for (i = 0; i < num_dimensions; i++)
    {
        num_elements *= dimension[i];
        rsub[i] = 0;
        rdim[i] = dimension[num_dimensions - 1 - i];
    }
    if (num_elements <= 1)
    {
        /* nothing to do */
        return 0;
    }

    multiplier[num_dimensions] = 1;
    rdim[num_dimensions] = 1;
    for (i = num_dimensions; i > 0; i--)
    {
        multiplier[i - 1] = multiplier[i] * rdim[i];
    }
    rdim[num_dimensions] = 0;
    rsub[num_dimensions] = 0;

    dst = (uint8_t *)malloc(num_elements * element_size);
    if (dst == NULL)
    {
        harp_set_error(HARP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       num_elements * element_size, __FILE__, __LINE__);
        return -1;
    }

    switch (element_size)
    {
        case 1:
            for (i = 0; i < num_elements; i++)
            {
                int j = 0;

                dst[index] = src[i];
                index += multiplier[j];
                rsub[j]++;
                while (rsub[j] == rdim[j])
                {
                    rsub[j] = 0;
                    index -= multiplier[j] * rdim[j];
                    j++;
                    index += multiplier[j];
                    rsub[j]++;
                }
            }
            break;
        case 2:
            for (i = 0; i < num_elements; i++)
            {
                int j = 0;

                ((uint16_t *)dst)[index] = ((uint16_t *)src)[i];
                index += multiplier[j];
                rsub[j]++;
                while (rsub[j] == rdim[j])
                {
                    rsub[j] = 0;
                    index -= multiplier[j] * rdim[j];
                    j++;
                    index += multiplier[j];
                    rsub[j]++;
                }
            }
            break;
        case 4:
            for (i = 0; i < num_elements; i++)
            {
                int j = 0;

                ((uint32_t *)dst)[index] = ((uint32_t *)src)[i];
                index += multiplier[j];
                rsub[j]++;
                while (rsub[j] == rdim[j])
                {
                    rsub[j] = 0;
                    index -= multiplier[j] * rdim[j];
                    j++;
                    index += multiplier[j];
                    rsub[j]++;
                }
            }
            break;
        case 8:
            for (i = 0; i < num_elements; i++)
            {
                int j = 0;

                ((uint64_t *)dst)[index] = ((uint64_t *)src)[i];
                index += multiplier[j];
                rsub[j]++;
                while (rsub[j] == rdim[j])
                {
                    rsub[j] = 0;
                    index -= multiplier[j] * rdim[j];
                    j++;
                    index += multiplier[j];
                    rsub[j]++;
                }
            }
            break;
        default:
            assert(0);
            exit(1);
    }

    memcpy(data.ptr, dst, num_elements * element_size);

    free(dst);

    return 0;
}

/** Remove leading directory components from \a path.
 * \param path Path to compute the basename of.
 * \return Pointer to the first character in \a path that is not part of a
 *     leading directory component, or a null pointer if no such character
 *     exists.
 */
const char *harp_basename(const char *path)
{
    if (path == NULL)
    {
        return NULL;
    }
    else
    {
        const char *basename = NULL;

        basename = strrchr(path, '/');
        if (basename == NULL)
        {
            return path;
        }
        else
        {
            return basename + 1;
        }
    }
}

/** Remove extension from \a path.
 * The last period '.' found in \a path will be replaced by a null termination
 * character '\0', thus effectively removing the extension.
 * \param path Path to remove extension from.
 */
void harp_remove_extension(char *path)
{
    if (path != NULL)
    {
        char *extension = NULL;

        extension = strrchr(path, '.');
        if (extension != NULL)
        {
            *extension = '\0';
        }
    }
}
