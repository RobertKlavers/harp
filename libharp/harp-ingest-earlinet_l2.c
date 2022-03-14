/*
 * Copyright (C) 2015-2022 S[&]T, The Netherlands.
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

#include "coda.h"
#include "harp-ingestion.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ------------------- Defines ------------------ */

#define DEFAULT_FILL_VALUE              9.9692099683868690e+36
#define FILL_VALUE_NO_DATA              -999.0

#define SECONDS_FROM_1970_TO_2000    946684800

#define CHECKED_MALLOC(v, s) v = malloc(s); if (v == NULL) { harp_set_error(HARP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)", s, __FILE__, __LINE__); return -1;}

/* ------------------ Typedefs ------------------ */

typedef struct ingest_info_struct
{
    coda_product *product;
    long num_times;
    long num_altitudes;
    double *values_buffer;
    int is_time_series;
} ingest_info;

/* -------------- Global variables --------------- */

static double nan;

/* -------------------- Code -------------------- */

static void ingestion_done(void *user_data)
{
    ingest_info *info = (ingest_info *)user_data;

    if (info != NULL)
    {
        if (info->values_buffer != NULL)
        {
            free(info->values_buffer);
        }
        free(info);
    }
}

/* General read functions */

static int read_scalar_attribute(ingest_info *info, const char *name, harp_data_type type, harp_array data)
{
    coda_cursor cursor;

    if (coda_cursor_set_product(&cursor, info->product) != 0)
    {
        harp_set_error(HARP_ERROR_CODA, NULL);
        return -1;
    }
    if (coda_cursor_goto_attributes(&cursor) != 0)
    {
        harp_set_error(HARP_ERROR_CODA, NULL);
        return -1;
    }
    if (coda_cursor_goto_record_field_by_name(&cursor, name) != 0)
    {
        harp_set_error(HARP_ERROR_CODA, NULL);
        return -1;
    }
    if (type == harp_type_double)
    {
        if (coda_cursor_read_double(&cursor, data.double_data) != 0)
        {
            harp_set_error(HARP_ERROR_CODA, NULL);
            return -1;
        }
    }
    else if (type == harp_type_int32)
    {
        if (coda_cursor_read_int32(&cursor, data.int32_data) != 0)
        {
            harp_set_error(HARP_ERROR_CODA, NULL);
            return -1;
        }
    }

    return 0;
}

static int read_scalar_variable(ingest_info *info, const char *name, harp_array data)
{
    coda_cursor cursor;
    double *double_data;

    if (coda_cursor_set_product(&cursor, info->product) != 0)
    {
        harp_set_error(HARP_ERROR_CODA, NULL);
        return -1;
    }
    if (coda_cursor_goto_record_field_by_name(&cursor, name) != 0)
    {
        harp_set_error(HARP_ERROR_CODA, NULL);
        return -1;
    }
    if (coda_cursor_read_double(&cursor, data.double_data) != 0)
    {
        harp_set_error(HARP_ERROR_CODA, NULL);
        return -1;
    }
    double_data = data.double_data;
    if (*double_data == FILL_VALUE_NO_DATA || *double_data == DEFAULT_FILL_VALUE)
    {
        *double_data = nan;
    }

    return 0;
}

static int read_array_variable(ingest_info *info, const char *name, long num_elements, harp_array data,
                               short *unit_is_percent)
{
    coda_cursor cursor;
    double *double_data;
    long actual_num_elements, l;

    if (coda_cursor_set_product(&cursor, info->product) != 0)
    {
        harp_set_error(HARP_ERROR_CODA, NULL);
        return -1;
    }
    if (coda_cursor_goto_record_field_by_name(&cursor, name) != 0)
    {
        harp_set_error(HARP_ERROR_CODA, NULL);
        return -1;
    }
    if (coda_cursor_get_num_elements(&cursor, &actual_num_elements) != 0)
    {
        harp_set_error(HARP_ERROR_CODA, NULL);
        return -1;
    }
    if (actual_num_elements != num_elements)
    {
        harp_set_error(HARP_ERROR_INGESTION, "variable %s has %ld elements (expected %ld)", name, actual_num_elements,
                       num_elements);
        return -1;
    }
    if (coda_cursor_read_double_array(&cursor, data.double_data, coda_array_ordering_c) != 0)
    {
        harp_set_error(HARP_ERROR_CODA, NULL);
        return -1;
    }
    double_data = data.double_data;
    for (l = 0; l < num_elements; l++)
    {
        if (*double_data == FILL_VALUE_NO_DATA)
        {
            *double_data = nan;
        }
        double_data++;
    }
    if (unit_is_percent != NULL)
    {
        char units[81];

        if (coda_cursor_goto_attributes(&cursor) != 0)
        {
            harp_set_error(HARP_ERROR_CODA, NULL);
            return -1;
        }
        if (coda_cursor_goto_record_field_by_name(&cursor, "units") != 0)
        {
            harp_set_error(HARP_ERROR_CODA, NULL);
            return -1;
        }
        if (coda_cursor_read_string(&cursor, units, sizeof(units)) != 0)
        {
            harp_set_error(HARP_ERROR_CODA, NULL);
            return -1;
        }
        *unit_is_percent = (strstr(units, "percent") != NULL);
    }

    return 0;
}

/* Specific read functions */

static int read_latitude(void *user_data, harp_array data)
{
    return read_scalar_attribute((ingest_info *)user_data, "Latitude_degrees_north", harp_type_double, data);
}

static int read_longitude(void *user_data, harp_array data)
{
    return read_scalar_attribute((ingest_info *)user_data, "Longitude_degrees_east", harp_type_double, data);
}

static int read_sensor_altitude(void *user_data, harp_array data)
{
    return read_scalar_attribute((ingest_info *)user_data, "Altitude_meter_asl", harp_type_double, data);
}

static int read_viewing_zenith_angle(void *user_data, harp_array data)
{
    return read_scalar_attribute((ingest_info *)user_data, "ZenithAngle_degrees", harp_type_double, data);
}

static int read_wavelength(void *user_data, harp_array data)
{
    return read_scalar_attribute((ingest_info *)user_data, "DetectionWavelength_nm", harp_type_double, data);
}

static int get_start_stop_time(ingest_info *info, double *start, double *stop)
{
    harp_array int32_array;
    int hour, minute, second;
    int32_t value;

    int32_array.int32_data = &value;
    if (read_scalar_attribute(info, "StartTime_UT", harp_type_int32, int32_array) != 0)
    {
        return -1;
    }
    hour = value / 10000;
    minute = (value - (10000 * hour)) / 100;
    second = value - (10000 * hour) - (100 * minute);
    *start = ((hour * 60.0) + minute) * 60.0 + second;

    if (read_scalar_attribute(info, "StopTime_UT", harp_type_int32, int32_array) != 0)
    {
        return -1;
    }
    hour = value / 10000;
    minute = (value - (10000 * hour)) / 100;
    second = value - (10000 * hour) - (100 * minute);
    *stop = ((hour * 60.0) + minute) * 60.0 + second;

    return 0;
}

static int read_datetime(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;
    double *double_data;
    long i;

    if (read_array_variable(info, "Time", info->num_times, data, NULL) != 0)
    {
        harp_array int32_array;
        int year, month, day;
        double datetime, start, stop;
        int32_t value;

        int32_array.int32_data = &value;
        if (read_scalar_attribute(info, "StartDate", harp_type_int32, int32_array) != 0)
        {
            return -1;
        }
        year = value / 10000;
        month = (value - (10000 * year)) / 100;
        day = value - (10000 * year) - (100 * month);
        coda_time_parts_to_double(year, month, day, 0, 0, 0, 0, &datetime);

        if (get_start_stop_time(info, &start, &stop) != 0)
        {
            return -1;
        }

        *(data.double_data) = datetime + (start + stop) / 2.0;

        return 0;
    }

    double_data = data.double_data;
    for (i = 0; i < info->num_times; i++)
    {
        *double_data = *double_data - SECONDS_FROM_1970_TO_2000;
        double_data++;
    }
    return 0;
}

static int read_datetime_length(void *user_data, harp_array data)
{
    double start, stop;

    if (get_start_stop_time((ingest_info *)user_data, &start, &stop) != 0)
    {
        return -1;
    }

    *(data.double_data) = stop - start;

    return 0;
}

static int read_altitude(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array_variable(info, "Altitude", info->num_altitudes, data, NULL);
}

static int read_backscatter(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array_variable(info, "Backscatter", info->num_times * info->num_altitudes, data, NULL);
}

static int read_backscatter_uncertainty(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;
    harp_array backscatter_values;
    double *value, *uncertainty;
    long l;
    short units_is_percent;

    if (read_array_variable(info, "ErrorBackscatter", info->num_times * info->num_altitudes, data, &units_is_percent) !=
        0)
    {
        return -1;
    }
    if (units_is_percent)
    {
        backscatter_values.double_data = info->values_buffer;
        if (read_array_variable(info, "Backscatter", info->num_times * info->num_altitudes, backscatter_values, NULL) !=
            0)
        {
            return -1;
        }
        value = backscatter_values.double_data;
        uncertainty = data.double_data;
        for (l = 0; l < (info->num_times * info->num_altitudes); l++)
        {
            if (harp_isnan(*value) || harp_isnan(*uncertainty))
            {
                *uncertainty = nan;
            }
            else
            {
                /* Calculate from the uncertainty as a percentage the uncertainty as a backscatter value */
                *uncertainty = (*value * *uncertainty / 100.0);
            }
            value++;
            uncertainty++;
        }
    }
    return 0;
}

static int read_dust_layer_height(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    if (info->is_time_series)
    {
        return read_array_variable((ingest_info *)user_data, "DustLayerHeight", info->num_times, data, NULL);
    }
    return read_scalar_variable((ingest_info *)user_data, "DustLayerHeight", data);
}

static int read_extinction(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array_variable(info, "Extinction", info->num_times * info->num_altitudes, data, NULL);
}

static int read_extinction_uncertainty(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;
    harp_array extinction_values;
    double *value, *uncertainty;
    long l;
    short units_is_percent;

    if (read_array_variable(info, "ErrorExtinction", info->num_times * info->num_altitudes, data, &units_is_percent) !=
        0)
    {
        return -1;
    }
    if (units_is_percent)
    {
        extinction_values.double_data = info->values_buffer;
        if (read_array_variable(info, "Extinction", info->num_times * info->num_altitudes, extinction_values, NULL) !=
            0)
        {
            return -1;
        }
        value = extinction_values.double_data;
        uncertainty = data.double_data;
        for (l = 0; l < (info->num_times * info->num_altitudes); l++)
        {
            if (harp_isnan(*value) || harp_isnan(*uncertainty))
            {
                *uncertainty = nan;
            }
            else
            {
                /* Calculate from the uncertainty as a percentage the uncertainty as an extinction value */
                *uncertainty = (*value * *uncertainty / 100.0);
            }
            value++;
            uncertainty++;
        }
    }
    return 0;
}

static int read_h2o_mass_mixing_ratio(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array_variable(info, "WaterVaporMixingRatio", info->num_times * info->num_altitudes, data, NULL);
}

static int read_h2o_mass_mixing_ratio_uncertainty(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array_variable(info, "ErrorWaterVapor", info->num_times * info->num_altitudes, data, NULL);
}

/* Include functions */

static int include_field_if_exists(void *user_data, const char *field_name)
{
    coda_cursor cursor;
    ingest_info *info = (ingest_info *)user_data;

    if (coda_cursor_set_product(&cursor, info->product) != 0)
    {
        return 0;
    }
    if (coda_cursor_goto_record_field_by_name(&cursor, field_name) != 0)
    {
        return 0;
    }
    return 1;
}

static int include_datetime_length(void *user_data)
{
    /* we include datetime_length if the Time variable does _not_ exist */
    return !include_field_if_exists(user_data, "Time");
}

static int include_dust_layer_height(void *user_data)
{
    return include_field_if_exists(user_data, "DustLayerHeight");
}

static int include_backscatter(void *user_data)
{
    return include_field_if_exists(user_data, "Backscatter");
}

static int include_backscatter_uncertainty(void *user_data)
{
    return include_field_if_exists(user_data, "ErrorBackscatter");
}

static int include_extinction(void *user_data)
{
    return include_field_if_exists(user_data, "Extinction");
}

static int include_extinction_uncertainty(void *user_data)
{
    return include_field_if_exists(user_data, "ErrorExtinction");
}

static int include_h2o_mass_mixing_ratio(void *user_data)
{
    return include_field_if_exists(user_data, "WaterVaporMixingRatio");
}

static int include_h2o_mass_mixing_ratio_uncertainty(void *user_data)
{
    return include_field_if_exists(user_data, "ErrorWaterVapor");
}

/* General functions to define fields and dimensions */

static int read_dimensions(void *user_data, long dimension[HARP_NUM_DIM_TYPES])
{
    ingest_info *info = (ingest_info *)user_data;

    dimension[harp_dimension_time] = info->num_times;
    dimension[harp_dimension_vertical] = info->num_altitudes;

    return 0;
}

static int get_dimensions(ingest_info *info)
{
    coda_cursor cursor;
    long coda_dimension[CODA_MAX_NUM_DIMS];
    int num_coda_dimensions;

    if (coda_cursor_set_product(&cursor, info->product) != 0)
    {
        harp_set_error(HARP_ERROR_CODA, NULL);
        return -1;
    }

    if (coda_cursor_goto(&cursor, "Time") != 0)
    {
        /* This is a single profile file (i.e. all measurements are taken at one time) */
        info->num_times = 1;
        info->is_time_series = 0;
    }
    else if (coda_cursor_get_array_dim(&cursor, &num_coda_dimensions, coda_dimension) != 0)
    {
        /* This productfile does not contain data */
        harp_set_error(HARP_ERROR_CODA, NULL);
        return -1;
    }
    else
    {
        info->num_times = coda_dimension[0];
        info->is_time_series = 1;
    }

    if (coda_cursor_set_product(&cursor, info->product) != 0)
    {
        harp_set_error(HARP_ERROR_CODA, NULL);
        return -1;
    }
    if (coda_cursor_goto(&cursor, "Altitude") != 0)
    {
        /* This productfile does not contain data */
        info->num_altitudes = 0;
        return 0;
    }
    if (coda_cursor_get_array_dim(&cursor, &num_coda_dimensions, coda_dimension) != 0)
    {
        /* This productfile does not contain data */
        info->num_altitudes = 0;
        return 0;
    }
    info->num_altitudes = coda_dimension[0];

    return 0;
}

static int ingestion_init(const harp_ingestion_module *module, coda_product *product,
                          const harp_ingestion_options *options, harp_product_definition **definition, void **user_data)
{
    ingest_info *info;

    (void)options;

    nan = harp_nan();
    info = malloc(sizeof(ingest_info));
    if (info == NULL)
    {
        harp_set_error(HARP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(ingest_info), __FILE__, __LINE__);
        return -1;
    }
    memset(info, '\0', sizeof(ingest_info));
    info->product = product;

    if (get_dimensions(info) != 0)
    {
        ingestion_done(info);
        return -1;
    }

    CHECKED_MALLOC(info->values_buffer, info->num_times * info->num_altitudes * sizeof(double));

    *definition = *module->product_definition;
    *user_data = info;

    return 0;
}

int harp_ingestion_module_earlinet_l2_aerosol_init(void)
{
    harp_ingestion_module *module;
    harp_product_definition *product_definition;
    harp_variable_definition *variable_definition;
    harp_dimension_type dimension_type[2] = { harp_dimension_time, harp_dimension_vertical };
    const char *description;
    const char *path;

    module = harp_ingestion_register_module("EARLINET", "EARLINET", "EARLINET", "EARLINET",
                                            "EARLINET aerosol backscatter and extinction profiles", ingestion_init,
                                            ingestion_done);
    product_definition = harp_ingestion_register_product(module, "EARLINET", NULL, read_dimensions);

    /* datetime */
    description = "time of measurement";
    variable_definition =
        harp_ingestion_register_variable_full_read(product_definition, "datetime", harp_type_double, 1, dimension_type,
                                                   NULL, description, "seconds since 2000-01-01", NULL, read_datetime);
    path = "/Time";
    description = "converted from seconds sinds 1970 to seconds since 2000";
    harp_variable_definition_add_mapping(variable_definition, NULL, "variable 'Time' available", path, description);

    path = "/@StartDate, /@StartTime_UT, /@StopTime_UT";
    description = "convert yymmdd encoded integer value for StartDate to seconds since 2000; "
        "convert hhmmss encoded integer values for StartTime_UT and StopTime_UT to time-of-day values; "
        "use: date + (start_time + stop_time) / 2";
    harp_variable_definition_add_mapping(variable_definition, NULL, "variable 'Time' unavailable", path, description);

    /* datetime_length */
    description = "length of the measurement";
    variable_definition =
        harp_ingestion_register_variable_full_read(product_definition, "datetime_length", harp_type_double, 1,
                                                   dimension_type, NULL, description, "s", include_datetime_length,
                                                   read_datetime_length);
    path = "/@StartTime_UT, /@StopTime_UT";
    description = "convert 'hhmmss' encoded integer values for StartTime_UT and StopTime_UT to time-of-day values; "
        "use: stop_time - start_time";
    harp_variable_definition_add_mapping(variable_definition, NULL, "variable 'Time' unavailable", path, description);

    /* latitude */
    description = "latitude";
    variable_definition =
        harp_ingestion_register_variable_full_read(product_definition, "latitude", harp_type_double, 0, dimension_type,
                                                   NULL, description, "degrees", NULL, read_latitude);
    path = "/@Latitude_degrees_north";
    harp_variable_definition_set_valid_range_double(variable_definition, -90.0, 90.0);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, path, NULL);

    /* longitude */
    description = "longitude";
    variable_definition =
        harp_ingestion_register_variable_full_read(product_definition, "longitude", harp_type_double, 0, dimension_type,
                                                   NULL, description, "degrees", NULL, read_longitude);
    path = "/@Longitude_degrees_east";
    harp_variable_definition_set_valid_range_double(variable_definition, -180.0, 180.0);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, path, NULL);

    /* sensor_altitude */
    description = "sensor altitude";
    variable_definition =
        harp_ingestion_register_variable_full_read(product_definition, "sensor_altitude", harp_type_double, 0,
                                                   dimension_type, NULL, description, "m", NULL, read_sensor_altitude);
    path = "/@Altitude_meter_asl";
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, path, NULL);

    /* viewing_zenith_angle */
    description = "viewing zenith angle";
    variable_definition =
        harp_ingestion_register_variable_full_read(product_definition, "viewing_zenith_angle", harp_type_double, 0,
                                                   dimension_type, NULL, description, "degrees", NULL,
                                                   read_viewing_zenith_angle);
    path = "/@ZenithAngle_degrees";
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, path, NULL);

    /* wavelength */
    description = "wavelength";
    variable_definition =
        harp_ingestion_register_variable_full_read(product_definition, "wavelength", harp_type_double, 0,
                                                   dimension_type, NULL, description, "nm", NULL, read_wavelength);
    path = "/@DetectionWavelength_nm";
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, path, NULL);

    /* altitude */
    description = "altitude";
    variable_definition =
        harp_ingestion_register_variable_full_read(product_definition, "altitude", harp_type_double, 1,
                                                   &(dimension_type[1]), NULL, description, "m", NULL, read_altitude);
    path = "/Altitude";
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, path, NULL);

    /* dust_aerosol_top_height */
    description = "dust layer top height";
    variable_definition =
        harp_ingestion_register_variable_full_read(product_definition, "dust_aerosol_top_height", harp_type_double, 1,
                                                   dimension_type, NULL, description, "m", include_dust_layer_height,
                                                   read_dust_layer_height);
    path = "/DustLayerHeight";
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, path, NULL);

    /* backscatter_coefficient */
    description = "backscatter coefficient";
    variable_definition =
        harp_ingestion_register_variable_full_read(product_definition, "backscatter_coefficient", harp_type_double, 2,
                                                   dimension_type, NULL, description, "1/(m*sr)", include_backscatter,
                                                   read_backscatter);
    path = "/Backscatter";
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, path, NULL);

    /* backscatter_coefficient_uncertainty */
    description = "backscatter coefficient uncertainty";
    variable_definition =
        harp_ingestion_register_variable_full_read(product_definition, "backscatter_coefficient_uncertainty",
                                                   harp_type_double, 2, dimension_type, NULL, description, "1/(m*sr)",
                                                   include_backscatter_uncertainty, read_backscatter_uncertainty);
    path = "/ErrorBackscatter";
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, path, NULL);

    /* extinction_coefficient */
    description = "extinction coefficient";
    variable_definition =
        harp_ingestion_register_variable_full_read(product_definition, "extinction_coefficient", harp_type_double, 2,
                                                   dimension_type, NULL, description, "1/m", include_extinction,
                                                   read_extinction);
    path = "/Extinction";
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, path, NULL);

    /* extinction_coefficient_uncertainty */
    description = "extinction coefficient uncertainty";
    variable_definition =
        harp_ingestion_register_variable_full_read(product_definition, "extinction_coefficient_uncertainty",
                                                   harp_type_double, 2, dimension_type, NULL, description, "1/m",
                                                   include_extinction_uncertainty, read_extinction_uncertainty);
    path = "/ErrorExtinction";
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, path, NULL);

    /* H2O_mass_mixing_ratio */
    description = "water mass mixing ratio";
    variable_definition =
        harp_ingestion_register_variable_full_read(product_definition, "H2O_mass_mixing_ratio", harp_type_double, 2,
                                                   dimension_type, NULL, description, "g/kg",
                                                   include_h2o_mass_mixing_ratio, read_h2o_mass_mixing_ratio);
    path = "/WaterVaporMixingRatio";
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, path, NULL);

    /* H2O_mass_mixing_ratio_uncertainty */
    description = "water mass mixing ratio uncertainty";
    variable_definition =
        harp_ingestion_register_variable_full_read(product_definition, "H2O_mass_mixing_ratio_uncertainty",
                                                   harp_type_double, 2, dimension_type, NULL, description, "g/kg",
                                                   include_h2o_mass_mixing_ratio_uncertainty,
                                                   read_h2o_mass_mixing_ratio_uncertainty);
    path = "/ErrorWaterVapor";
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, path, NULL);

    return 0;
}
