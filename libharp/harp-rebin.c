/*
 * Copyright (C) 2015-2020 S[&]T, The Netherlands.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "harp-internal.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef enum binning_type_enum
{
    binning_skip,
    binning_remove,
    binning_average,
    binning_sum
} binning_type;

static binning_type get_binning_type(harp_variable *variable, harp_dimension_type dimension_type)
{
    long variable_name_length = (long)strlen(variable->name);
    int num_matching_dims;
    int i;

    /* remove any count/weight variables */
    if ((variable_name_length >= 5 && strcmp(&variable->name[variable_name_length - 5], "count") == 0) ||
        (variable_name_length >= 6 && strcmp(&variable->name[variable_name_length - 6], "weight") == 0))
    {
        return binning_remove;
    }

    /* ensure that there is only 1 dimension of the given type */
    for (i = 0, num_matching_dims = 0; i < variable->num_dimensions; i++)
    {
        if (variable->dimension_type[i] == dimension_type)
        {
            num_matching_dims++;
        }
    }

    if (num_matching_dims == 0)
    {
        /* if the variable has no matching dimension, we should always skip */
        return binning_skip;
    }

    if (num_matching_dims != 1)
    {
        /* remove all variables with more than one matching dimension */
        return binning_remove;
    }

    /* any variable with a time dimension that is not the first dimension gets removed */
    for (i = 1; i < variable->num_dimensions; i++)
    {
        if (variable->dimension[i] == harp_dimension_time)
        {
            return binning_remove;
        }
    }

    /* we can't bin string values */
    if (variable->data_type == harp_type_string)
    {
        return binning_remove;
    }

    /* we can't bin values that have no unit */
    if (variable->unit == NULL)
    {
        return binning_remove;
    }

    /* variables with enumeration values get removed */
    if (variable->num_enum_values > 0)
    {
        return binning_remove;
    }

    /* remove all existing axis variables for the binned dimension */
    switch (dimension_type)
    {
        case harp_dimension_time:
            if (strcmp(variable->name, "datetime") == 0 || strcmp(variable->name, "datetime_bounds") == 0 ||
                strcmp(variable->name, "datetime_start") == 0 || strcmp(variable->name, "datetime_stop") == 0 ||
                strcmp(variable->name, "datetime_length") == 0)
            {
                return binning_remove;
            }
            break;
        case harp_dimension_latitude:
            if (strcmp(variable->name, "latitude") == 0 || strcmp(variable->name, "latitude_bounds") == 0)
            {
                return binning_remove;
            }
            break;
        case harp_dimension_longitude:
            if (strcmp(variable->name, "longitude") == 0 || strcmp(variable->name, "longitude_bounds") == 0)
            {
                return binning_remove;
            }
            break;
        case harp_dimension_vertical:
            if (strcmp(variable->name, "altitude") == 0 || strcmp(variable->name, "altitude_bounds") == 0 |
                strcmp(variable->name, "altitude_gph") == 0 || strcmp(variable->name, "altitude_gph_bounds") == 0 ||
                strcmp(variable->name, "pressure") == 0 || strcmp(variable->name, "pressure_bounds") == 0)
            {
                return binning_remove;
            }
            /* use integrated quantity rebinning for vertical regridding of 1D column AVKs */
            if (strstr(variable->name, "_avk") != NULL)
            {
                return binning_sum;
            }
            /* use integrated quantity rebinning for vertical regridding of partial column profiles */
            if (strstr(variable->name, "_column_") != NULL)
            {
                return binning_sum;
            }
            break;
        case harp_dimension_spectral:
            if (strcmp(variable->name, "wavelength") == 0 || strcmp(variable->name, "wavelength_bounds") == 0 ||
                strcmp(variable->name, "wavenumber") == 0 || strcmp(variable->name, "wavenumber_bounds") == 0 ||
                strcmp(variable->name, "frequency") == 0 || strcmp(variable->name, "frequency_bounds") == 0)
            {
                return binning_remove;
            }
            break;
        case harp_dimension_independent:
            assert(0);
            exit(1);
    }

    /* we can't bin averaging kernels (unless 1D column AVKs (see above)) */
    if (strstr(variable->name, "_avk") != NULL)
    {
        return binning_remove;
    }

    /* use average by default */
    return binning_average;
}

static int resize_dimension(harp_product *product, harp_dimension_type dimension_type, long num_elements)
{
    int i;

    for (i = 0; i < product->num_variables; i++)
    {
        harp_variable *var = product->variable[i];
        int j;

        for (j = 0; j < var->num_dimensions; j++)
        {
            if (var->dimension_type[j] == dimension_type)
            {
                if (harp_variable_resize_dimension(var, j, num_elements) != 0)
                {
                    return -1;
                }
            }
        }
    }

    product->dimension[dimension_type] = num_elements;

    return 0;
}

static int filter_binnable_variables(harp_product *product, harp_dimension_type dimension_type)
{
    int i;

    for (i = product->num_variables - 1; i >= 0; i--)
    {
        if (get_binning_type(product->variable[i], dimension_type) == binning_remove)
        {
            if (harp_product_remove_variable(product, product->variable[i]) != 0)
            {
                return -1;
            }
        }
    }

    return 0;
}

static int add_source_index(long source_index, double weight, long *cumsum_index, long **map_source_index,
                            double **map_source_weight)
{
    if ((*cumsum_index) % BLOCK_SIZE == 0)
    {
        long *new_map_source_index;
        double *new_map_source_weight;

        new_map_source_index = realloc(*map_source_index, ((*cumsum_index) + BLOCK_SIZE) * sizeof(long));
        if (new_map_source_index == NULL)
        {
            harp_set_error(HARP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           ((*cumsum_index) + BLOCK_SIZE) * sizeof(long), __FILE__, __LINE__);
            return -1;
        }
        *map_source_index = new_map_source_index;
        new_map_source_weight = realloc(*map_source_weight, ((*cumsum_index) + BLOCK_SIZE) * sizeof(double));
        if (new_map_source_weight == NULL)
        {
            harp_set_error(HARP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           ((*cumsum_index) + BLOCK_SIZE) * sizeof(double), __FILE__, __LINE__);
            return -1;
        }
        *map_source_weight = new_map_source_weight;
    }
    (*map_source_index)[(*cumsum_index)] = source_index;
    (*map_source_weight)[(*cumsum_index)] = weight;
    (*cumsum_index)++;

    return 0;
}

static int find_matching_intervals_for_bounds(harp_variable *target_bounds, harp_variable *source_bounds,
                                              long **map_source_index, double **map_source_weight, long *map_offset,
                                              long *map_length)
{
    double *source_data = source_bounds->data.double_data;
    double *target_data = target_bounds->data.double_data;
    long bounds_num_time_elements = 1;
    long source_num_dim_elements;
    long target_num_dim_elements;
    long cumsum_index = 0;
    long i, j, k;

    if (target_bounds->num_dimensions == 3)
    {
        bounds_num_time_elements = target_bounds->dimension[0];
    }
    else if (source_bounds->num_dimensions == 3)
    {
        bounds_num_time_elements = source_bounds->dimension[0];
    }
    source_num_dim_elements = source_bounds->dimension[source_bounds->num_dimensions - 2];
    target_num_dim_elements = target_bounds->dimension[target_bounds->num_dimensions - 2];

    for (i = 0; i < bounds_num_time_elements; i++)
    {
        long source_bounds_offset = 0;
        long target_bounds_offset = 0;

        if (source_bounds->num_dimensions == 3)
        {
            source_bounds_offset = i * source_num_dim_elements;
        }
        if (target_bounds->num_dimensions == 3)
        {
            target_bounds_offset = i * target_num_dim_elements;
        }

        for (j = 0; j < target_num_dim_elements; j++)
        {
            long target_bounds_index = target_bounds_offset + 2 * j;
            long map_index = i * target_num_dim_elements + j;
            double xminb, xmaxb;

            map_length[map_index] = 0;

            if (target_data[target_bounds_index] < target_data[target_bounds_index + 1])
            {
                xminb = target_data[target_bounds_index];
                xmaxb = target_data[target_bounds_index + 1];
            }
            else
            {
                xminb = target_data[target_bounds_index + 1];
                xmaxb = target_data[target_bounds_index];
            }

            for (k = 0; k < source_num_dim_elements; k++)
            {
                long source_bounds_index = source_bounds_offset + 2 * k;
                double xmina, xmaxa;

                if (source_data[source_bounds_index] < source_data[source_bounds_index + 1])
                {
                    xmina = source_data[source_bounds_index];
                    xmaxa = source_data[source_bounds_index + 1];
                }
                else
                {
                    xmina = source_data[source_bounds_index + 1];
                    xmaxa = source_data[source_bounds_index];
                }

                if (!(xmina >= xmaxb || xminb >= xmaxa))
                {
                    double xminc, xmaxc, weight;

                    /* there is overlap and interval A is not empty */

                    /* calculate intersection interval C of intervals A and B */
                    xminc = xmina < xminb ? xminb : xmina;
                    xmaxc = xmaxa > xmaxb ? xmaxb : xmaxa;

                    weight = (xmaxc - xminc) / (xmaxa - xmina);

                    if (map_length[map_index] == 0)
                    {
                        map_offset[map_index] = cumsum_index;
                    }
                    if (add_source_index(k, weight, &cumsum_index, map_source_index, map_source_weight) != 0)
                    {
                        return -1;
                    }
                    map_length[map_index]++;
                }
            }
        }
    }

    return 0;
}

/** \addtogroup harp_product
 * @{
 */

/**
 * Rebin all variables in the product to a specified interval grid.
 * The target bounds variable should be an axis bounds variable containing the interval edges (bins) of the target grid (as 'double' values).
 * It should be a two-dimensional variable (for a time independent grid) or a three-dimensional variable (for a time dependent grid).
 * The last dimension should be an independent dimension of length 2 (for the lower/upper bound of each interval).
 * The dimension to use for regridding is based on the type of the second to last dimenion of the target grid variable.
 * This function cannot be used to rebin an independent dimension or rebin the time dimension.
 *
 * For each variable in the product, a dimension-specific rule based on the variable name will determine how to rebin
 * the variable.
 * For most variables the result will be the interval weighted average of all values overlapping the target interval.
 * Variables that represent an integrated quantity for the rebinned dimension will use an interval weighted sum.
 * For uncertainty variables the first order propagation rules are used (assuming full correlation).
 *
 * Variables that depend on the rebinned dimenion but have no unit (or use a string data type) will be removed.
 * Any existing count or weight variables will also be removed.
 *
 * All variables that are rebinned are converted to a double data type.
 * Bins that have no overlapping source boundaries will end up with a NaN value.
 *
 * \param product Product to rebin.
 * \param target_bounds Target grid boundaries variable.
 *
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #harp_errno).
 */
LIBHARP_API int harp_product_rebin_with_axis_bounds_variable(harp_product *product, harp_variable *target_bounds)
{
    harp_dimension_type dimension_type;
    long bounds_num_time_elements = 1;
    long source_num_dim_elements;
    long target_num_dim_elements;
    harp_variable *variable;
    long variable_name_length = (long)strlen(target_bounds->name);
    long i;

    /* owned memory */
    harp_variable *source_bounds = NULL;
    harp_variable *local_target_bounds = NULL;
    long *map_source_index = NULL;      /* [num_map_elements] flat index into [dim] source array */
    double *map_source_weight = NULL;   /* [num_map_elements] weight for each [dim] source array */
    long *map_offset = NULL;    /* [(time,) dim] offset index into map_source_index/map_weight for target array */
    long *map_length = NULL;    /* [(time,) dim] number of elements in map_source_index/map_weight for target array */
    double *buffer = NULL;

    if (variable_name_length < 7 || strcmp(&target_bounds->name[variable_name_length - 7], "_bounds") != 0)
    {
        harp_set_error(HARP_ERROR_INVALID_ARGUMENT, "axis variable is not a boundaries variable");
        goto error;
    }
    if (target_bounds->data_type != harp_type_double)
    {
        harp_set_error(HARP_ERROR_INVALID_ARGUMENT, "invalid data type for axis bounds variable");
        goto error;
    }
    if (target_bounds->num_dimensions != 2 && target_bounds->num_dimensions != 3)
    {
        harp_set_error(HARP_ERROR_INVALID_ARGUMENT, "invalid dimensions for axis bounds variable");
        goto error;
    }
    dimension_type = target_bounds->dimension_type[target_bounds->num_dimensions - 2];
    if (dimension_type == harp_dimension_independent)
    {
        harp_set_error(HARP_ERROR_INVALID_ARGUMENT, "invalid dimensions for axis variable");
        goto error;
    }
    target_num_dim_elements = target_bounds->dimension[target_bounds->num_dimensions - 2];
    if (target_bounds->num_dimensions == 3)
    {
        if (target_bounds->dimension_type[0] != harp_dimension_time || dimension_type == harp_dimension_time)
        {
            harp_set_error(HARP_ERROR_INVALID_ARGUMENT, "invalid dimensions for axis bounds variable");
            goto error;
        }
        if (target_bounds->dimension[0] != product->dimension[harp_dimension_time])
        {
            harp_set_error(HARP_ERROR_INVALID_ARGUMENT,
                           "time dimension of axis bounds variable does not match product");
            goto error;
        }
    }
    dimension_type = target_bounds->dimension_type[target_bounds->num_dimensions - 2];
    if (dimension_type == harp_dimension_independent)
    {
        harp_set_error(HARP_ERROR_INVALID_ARGUMENT, "invalid dimensions for axis bounds variable");
        goto error;
    }
    if (target_bounds->dimension_type[target_bounds->num_dimensions - 1] != harp_dimension_independent ||
        target_bounds->dimension[target_bounds->num_dimensions - 1] != 2)
    {
        harp_set_error(HARP_ERROR_INVALID_ARGUMENT, "invalid independent dimension for axis bounds variable");
        goto error;
    }

    if (harp_variable_copy(target_bounds, &local_target_bounds) != 0)
    {
        goto error;
    }

    if (dimension_type == harp_dimension_time)
    {
        /* Derive the source grid */
        if (harp_product_get_derived_variable(product, target_bounds->name, &target_bounds->data_type,
                                              target_bounds->unit, 2, target_bounds->dimension_type, &source_bounds) !=
            0)
        {
            goto error;
        }
        source_num_dim_elements = source_bounds->dimension[0];
    }
    else
    {
        harp_dimension_type bounds_dim_type[3];

        bounds_dim_type[0] = harp_dimension_time;
        bounds_dim_type[1] = dimension_type;
        bounds_dim_type[2] = harp_dimension_independent;

        /* Derive the source bounds */
        /* Try time independent */
        if (harp_product_get_derived_variable(product, target_bounds->name, &target_bounds->data_type,
                                              target_bounds->unit, 2, &bounds_dim_type[1], &source_bounds) != 0)
        {
            /* Failed to derive time independent. Try time dependent. */
            if (harp_product_get_derived_variable(product, target_bounds->name, &target_bounds->data_type,
                                                  target_bounds->unit, 3, bounds_dim_type, &source_bounds) != 0)
            {
                goto error;
            }
        }
        source_num_dim_elements = source_bounds->dimension[source_bounds->num_dimensions - 2];

        if (target_bounds->num_dimensions == 3 || source_bounds->num_dimensions == 3)
        {
            bounds_num_time_elements = product->dimension[harp_dimension_time];
        }
    }

    /* remove grid variables if they exists (since we don't want to rebin them) */
    /* this won't affect the source_bounds variables that we already derived */
    if (harp_product_has_variable(product, source_bounds->name))
    {
        if (harp_product_remove_variable_by_name(product, source_bounds->name) != 0)
        {
            goto error;
        }
    }

    /* remove variables that can't be rebinned */
    if (filter_binnable_variables(product, dimension_type) != 0)
    {
        goto error;
    }

    /* Use logarithmic axis if vertical pressure grid */
    if (dimension_type == harp_dimension_vertical && strcmp(local_target_bounds->name, "pressure_bounds") == 0)
    {
        for (i = 0; i < source_bounds->num_elements; i++)
        {
            source_bounds->data.double_data[i] = log(source_bounds->data.double_data[i]);
        }
        for (i = 0; i < local_target_bounds->num_elements; i++)
        {
            local_target_bounds->data.double_data[i] = log(local_target_bounds->data.double_data[i]);
        }
    }

    /* allocate memory for map_offset, map_length, and buffer */
    map_offset = (long *)malloc(bounds_num_time_elements * target_num_dim_elements * (size_t)sizeof(double));
    if (map_offset == NULL)
    {
        harp_set_error(HARP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       bounds_num_time_elements * target_num_dim_elements * sizeof(double), __FILE__, __LINE__);
        goto error;
    }
    map_length = (long *)malloc(bounds_num_time_elements * target_num_dim_elements * (size_t)sizeof(double));
    if (map_length == NULL)
    {
        harp_set_error(HARP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       bounds_num_time_elements * target_num_dim_elements * sizeof(double), __FILE__, __LINE__);
        goto error;
    }
    buffer = (double *)malloc(target_num_dim_elements * (size_t)sizeof(double));
    if (buffer == NULL)
    {
        harp_set_error(HARP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       target_num_dim_elements * sizeof(double), __FILE__, __LINE__);
        goto error;
    }

    /* determine which source intervals match each target interval (and associated weight) */
    if (find_matching_intervals_for_bounds(local_target_bounds, source_bounds, &map_source_index, &map_source_weight,
                                           map_offset, map_length) != 0)
    {
        goto error;
    }

    /* Resize the dimension in the target product to make room for the rebinned data */
    if (target_num_dim_elements > source_num_dim_elements)
    {
        if (resize_dimension(product, dimension_type, target_num_dim_elements) != 0)
        {
            goto error;
        }
        source_num_dim_elements = target_num_dim_elements;
    }

    /* regrid each variable */
    for (i = product->num_variables - 1; i >= 0; i--)
    {
        binning_type type;
        long target_bounds_time_index;
        long num_blocks;
        long num_elements;
        long j;

        variable = product->variable[i];

        /* Check if we can rebin this kind of variable */
        type = get_binning_type(variable, dimension_type);

        assert(type != binning_remove);
        if (type == binning_skip)
        {
            continue;
        }

        /* Ensure that the variable data consists of doubles */
        if (variable->data_type != harp_type_double && harp_variable_convert_data_type(variable, harp_type_double) != 0)
        {
            goto error;
        }

        /* Make time independent variables time dependent if source grid or target grid is 2D (i.e. time dependent) */
        if (target_bounds->num_dimensions == 3 || source_bounds->num_dimensions == 3)
        {
            if (variable->dimension_type[0] != harp_dimension_time)
            {
                if (harp_variable_add_dimension(variable, 0, harp_dimension_time, bounds_num_time_elements) != 0)
                {
                    goto error;
                }
            }
        }
        /* Also make variable time dependent if the grid dimension is time and the variable does not depend on time */
        if (dimension_type == harp_dimension_time &&
            (variable->num_dimensions == 0 || variable->dimension_type[0] != harp_dimension_time))
        {
            if (harp_variable_add_dimension(variable, 0, harp_dimension_time, source_num_dim_elements) != 0)
            {
                goto error;
            }
        }

        /* treat variable as a [num_blocks, source_max_dim_elements, num_elements] array with indices [j,k,l] */
        num_blocks = 1;
        num_elements = 1;
        j = 0;
        assert(variable->num_dimensions > 0);
        while (variable->dimension_type[j] != dimension_type)
        {
            assert(j < variable->num_dimensions - 1);
            num_blocks *= variable->dimension[j];
            j++;
        }
        j++;    /* skip dimension that is going to be regridded */
        while (j < variable->num_dimensions)
        {
            num_elements *= variable->dimension[j];
            j++;
        }

        /* rebin the data of the variable over the given dimension */
        /* keep track of time index separately since num_blocks can capture more than just the time dimension */
        target_bounds_time_index = 0;
        for (j = 0; j < num_blocks; j++)
        {
            long k, l;

            /* keep track of time index for 2D grids */
            if (j % (num_blocks / bounds_num_time_elements) == 0)
            {
                if (target_bounds->num_dimensions == 3 && j > 0)
                {
                    target_bounds_time_index++;
                }
            }

            for (l = 0; l < num_elements; l++)
            {
                for (k = 0; k < target_num_dim_elements; k++)
                {
                    long offset = map_offset[target_bounds_time_index * target_num_dim_elements + k];
                    double weightsum = 0;
                    double valuesum = 0;
                    long m;

                    for (m = 0; m < map_length[target_bounds_time_index * target_num_dim_elements + k]; m++)
                    {
                        long source_index = j * source_num_dim_elements + map_source_index[offset + m];
                        double weight = map_source_weight[offset + m];
                        double value = variable->data.double_data[source_index * num_elements + l];

                        if (!harp_isnan(value))
                        {
                            valuesum += weight * value;
                            weightsum += weight;
                        }
                    }

                    if (weightsum != 0)
                    {
                        buffer[k] = valuesum;
                        if (type == binning_average)
                        {
                            buffer[k] /= weightsum;
                        }
                    }
                    else
                    {
                        buffer[k] = harp_nan();
                    }
                }
                for (k = 0; k < target_num_dim_elements; k++)
                {
                    variable->data.double_data[(j * source_num_dim_elements + k) * num_elements + l] = buffer[k];
                }
            }
        }
    }

    /* Resize the dimension in the target product to minimal size */
    if (target_num_dim_elements < source_num_dim_elements)
    {
        if (resize_dimension(product, dimension_type, target_num_dim_elements) != 0)
        {
            goto error;
        }
    }

    /* ensure consistent axis variables in product */
    if (harp_variable_copy(target_bounds, &variable) != 0)
    {
        goto error;
    }
    if (harp_product_add_variable(product, variable) != 0)
    {
        harp_variable_delete(variable);
        goto error;
    }

    /* cleanup */
    harp_variable_delete(source_bounds);
    harp_variable_delete(local_target_bounds);
    free(map_source_index);
    free(map_source_weight);
    free(map_offset);
    free(map_length);
    free(buffer);

    return 0;

  error:
    harp_variable_delete(source_bounds);
    harp_variable_delete(local_target_bounds);
    if (map_source_index != NULL)
    {
        free(map_source_index);
    }
    if (map_source_weight != NULL)
    {
        free(map_source_weight);
    }
    if (map_offset != NULL)
    {
        free(map_offset);
    }
    if (map_length != NULL)
    {
        free(map_length);
    }
    if (buffer != NULL)
    {
        free(buffer);
    }

    return -1;
}

/**
 * @}
 */
