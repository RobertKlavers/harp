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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PATH_LENGTH 256

typedef struct ingest_info_struct
{
    coda_product *product;
    long num_time;
    long num_vertical;
    coda_cursor science_data_cursor;
    int resolution;     /* 0: default, 1: medium, 2: low */
} ingest_info;


static int read_dimensions(void *user_data, long dimension[HARP_NUM_DIM_TYPES])
{
    dimension[harp_dimension_time] = ((ingest_info *)user_data)->num_time;
    dimension[harp_dimension_vertical] = ((ingest_info *)user_data)->num_vertical;
    return 0;
}

static int read_array(coda_cursor cursor, const char *path, harp_data_type data_type, long num_elements,
                      harp_array data)
{
    long coda_num_elements;

    if (coda_cursor_goto(&cursor, path) != 0)
    {
        harp_set_error(HARP_ERROR_CODA, NULL);
        return -1;
    }
    if (coda_cursor_get_num_elements(&cursor, &coda_num_elements) != 0)
    {
        harp_set_error(HARP_ERROR_CODA, NULL);
        return -1;
    }
    if (coda_num_elements != num_elements)
    {
        harp_set_error(HARP_ERROR_INGESTION, "variable has %ld elements; expected %ld", coda_num_elements,
                       num_elements);
        harp_add_coda_cursor_path_to_error_message(&cursor);
        return -1;
    }

    switch (data_type)
    {
        case harp_type_int8:
            if (coda_cursor_read_int8_array(&cursor, data.int8_data, coda_array_ordering_c) != 0)
            {
                harp_set_error(HARP_ERROR_CODA, NULL);
                return -1;
            }
            break;
        case harp_type_int32:
            if (coda_cursor_read_int32_array(&cursor, data.int32_data, coda_array_ordering_c) != 0)
            {
                harp_set_error(HARP_ERROR_CODA, NULL);
                return -1;
            }
            break;
        case harp_type_float:
            if (coda_cursor_read_float_array(&cursor, data.float_data, coda_array_ordering_c) != 0)
            {
                harp_set_error(HARP_ERROR_CODA, NULL);
                return -1;
            }
            break;
        case harp_type_double:
            if (coda_cursor_read_double_array(&cursor, data.double_data, coda_array_ordering_c) != 0)
            {
                harp_set_error(HARP_ERROR_CODA, NULL);
                return -1;
            }
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

static int init_cursors(ingest_info *info)
{
    coda_cursor cursor;

    if (coda_cursor_set_product(&cursor, info->product) != 0)
    {
        harp_set_error(HARP_ERROR_CODA, NULL);
        return -1;
    }
    if (coda_cursor_goto_record_field_by_name(&cursor, "ScienceData") != 0)
    {
        harp_set_error(HARP_ERROR_CODA, NULL);
        return -1;
    }
    info->science_data_cursor = cursor;

    if (coda_cursor_goto_record_field_by_name(&cursor, "time") != 0)
    {
        harp_set_error(HARP_ERROR_CODA, NULL);
        return -1;
    }
    if (coda_cursor_get_num_elements(&cursor, &info->num_time) != 0)
    {
        harp_set_error(HARP_ERROR_CODA, NULL);
        return -1;
    }
    coda_cursor_goto_parent(&cursor);

    if (coda_cursor_goto_record_field_by_name(&cursor, "height") != 0)
    {
        harp_set_error(HARP_ERROR_CODA, NULL);
        return -1;
    }
    if (coda_cursor_get_num_elements(&cursor, &info->num_vertical) != 0)
    {
        harp_set_error(HARP_ERROR_CODA, NULL);
        return -1;
    }
    info->num_vertical /= info->num_time;

    return 0;
}

static int read_355nm(void *user_data, harp_array data)
{
    (void)user_data;

    *data.float_data = 355;

    return 0;
}

static int read_aerosol_classification(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "aerosol_classification", harp_type_int8,
                      info->num_time * info->num_vertical, data);
}

static int read_aerosol_extinction(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "aerosol_extinction", harp_type_float,
                      info->num_time * info->num_vertical, data);
}

static int read_aerosol_mass_content(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "aerosol_mass_content", harp_type_float,
                      info->num_time * info->num_vertical, data);
}

static int read_aerosol_number_concentration(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "aerosol_number_concentration", harp_type_float,
                      info->num_time * info->num_vertical, data);
}

static int read_aerosol_optical_depth(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "aerosol_optical_depth", harp_type_float, info->num_time, data);
}

static int read_classification(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "classification", harp_type_int8, info->num_time * info->num_vertical,
                      data);
}

static int read_elevation(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "elevation", harp_type_float, info->num_time, data);
}

static int read_height(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "height", harp_type_float, info->num_time * info->num_vertical, data);
}

static int read_ice_effective_radius(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "ice_effective_radius", harp_type_float,
                      info->num_time * info->num_vertical, data);
}

static int read_ice_mass_flux(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "ice_mass_flux", harp_type_float, info->num_time * info->num_vertical,
                      data);
}

static int read_ice_water_content(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "ice_water_content", harp_type_float,
                      info->num_time * info->num_vertical, data);
}

static int read_ice_water_path(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "ice_water_path", harp_type_float, info->num_time, data);
}

static int read_latitude(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "latitude", harp_type_double, info->num_time, data);
}

static int read_lidar_ratio_355nm(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "lidar_ratio_355nm", harp_type_float,
                      info->num_time * info->num_vertical, data);
}

static int read_lidar_ratio_355nm_error(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "lidar_ratio_355nm_error", harp_type_float,
                      info->num_time * info->num_vertical, data);
}

static int read_liquid_effective_radius(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "liquid_effective_radius", harp_type_float,
                      info->num_time * info->num_vertical, data);
}

static int read_liquid_extinction(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "liquid_extinction", harp_type_float,
                      info->num_time * info->num_vertical, data);
}

static int read_liquid_water_content(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "liquid_water_content", harp_type_float,
                      info->num_time * info->num_vertical, data);
}

static int read_longitude(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "longitude", harp_type_double, info->num_time, data);
}

static int read_orbit_index(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;
    coda_cursor cursor;

    if (coda_cursor_set_product(&cursor, info->product) != 0)
    {
        harp_set_error(HARP_ERROR_CODA, NULL);
        return -1;
    }
    return read_array(cursor, "/HeaderData/VariableProductHeader/MainProductHeader/orbitNumber", harp_type_int32, 1,
                      data);
    if (coda_cursor_goto(&cursor, "/HeaderData/VariableProductHeader/MainProductHeader/orbitNumber") != 0)
    {
        harp_set_error(HARP_ERROR_CODA, NULL);
        return -1;
    }
    if (coda_cursor_read_int32(&cursor, data.int32_data) != 0)
    {
        harp_set_error(HARP_ERROR_CODA, NULL);
        return -1;
    }

    return 0;
}

static int read_particle_backscatter_coefficient_355nm(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "particle_backscatter_coefficient_355nm", harp_type_float,
                      info->num_time * info->num_vertical, data);
}

static int read_particle_backscatter_coefficient_355nm_error(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "particle_backscatter_coefficient_355nm_error", harp_type_float,
                      info->num_time * info->num_vertical, data);
}

static int read_particle_extinction_coefficient_355nm(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "particle_extinction_coefficient_355nm", harp_type_float,
                      info->num_time * info->num_vertical, data);
}

static int read_particle_extinction_coefficient_355nm_error(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "particle_extinction_coefficient_355nm_error", harp_type_float,
                      info->num_time * info->num_vertical, data);
}

static int read_particle_linear_depolarization_ratio_355nm(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "particle_linear_depolarization_ratio_355nm", harp_type_float,
                      info->num_time * info->num_vertical, data);
}

static int read_particle_linear_depolarization_ratio_355nm_error(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "particle_linear_depolarization_ratio_355nm_error", harp_type_float,
                      info->num_time * info->num_vertical, data);
}

static int read_quality_status(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "quality_status", harp_type_int8, info->num_time, data);
}

static int read_quality_status_2d(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "quality_status", harp_type_int8, info->num_time * info->num_vertical,
                      data);
}

static int read_rain_rate(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "rain_rate", harp_type_float, info->num_time * info->num_vertical,
                      data);
}

static int read_rain_water_content(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "rain_water_content", harp_type_float,
                      info->num_time * info->num_vertical, data);
}

static int read_synergetic_target_classification(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;
    const char *name = "synergetic_target_classification";

    if (info->resolution == 1)
    {
        name = "synergetic_target_classification_medium_resolution";
    }
    else if (info->resolution == 2)
    {
        name = "synergetic_target_classification_low_resolution";
    }

    return read_array(info->science_data_cursor, name, harp_type_int8, info->num_time * info->num_vertical, data);
}

static int read_time(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "time", harp_type_double, info->num_time, data);
}

static int read_tropopause_height(void *user_data, harp_array data)
{
    ingest_info *info = (ingest_info *)user_data;

    return read_array(info->science_data_cursor, "tropopause_height", harp_type_float, info->num_time, data);
}

static void ingestion_done(void *user_data)
{
    ingest_info *info = (ingest_info *)user_data;

    free(info);
}

static int ingestion_init(const harp_ingestion_module *module, coda_product *product,
                          const harp_ingestion_options *options, harp_product_definition **definition, void **user_data)
{
    const char *option_value;
    ingest_info *info;

    info = malloc(sizeof(ingest_info));
    if (info == NULL)
    {
        harp_set_error(HARP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(ingest_info), __FILE__, __LINE__);
        return -1;
    }
    info->product = product;
    info->resolution = 0;
    *definition = module->product_definition[0];

    if (harp_ingestion_options_has_option(options, "resolution"))
    {
        if (harp_ingestion_options_get_option(options, "resolution", &option_value) != 0)
        {
            ingestion_done(info);
            return -1;
        }
        if (strcmp(option_value, "medium") == 0)
        {
            info->resolution = 1;
        }
        else
        {
            /* option_value == "low" */
            info->resolution = 2;
        }
    }

    if (init_cursors(info) != 0)
    {
        ingestion_done(info);
        return -1;
    }

    *user_data = info;

    return 0;
}

static void register_common_variables(harp_product_definition *product_definition)
{
    harp_variable_definition *variable_definition;
    harp_dimension_type dimension_type[2];

    dimension_type[0] = harp_dimension_time;
    dimension_type[1] = harp_dimension_vertical;

    /* datetime */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "datetime", harp_type_double,
                                                                     1, dimension_type, NULL, "UTC time",
                                                                     "seconds since 2000-01-01", NULL, read_time);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/time", NULL);

    /* latitude */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "latitude", harp_type_double,
                                                                     1, dimension_type, NULL, "Geodetic latitude",
                                                                     "degree_north", NULL, read_latitude);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/latitude", NULL);

    /* longitude */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "longitude", harp_type_double,
                                                                     1, dimension_type, NULL, "Geodetic longitude",
                                                                     "degree_east", NULL, read_longitude);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/longitude", NULL);

    /* orbit_index */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "orbit_index", harp_type_int32,
                                                                     0, NULL, NULL, "absolute orbit number", NULL, NULL,
                                                                     read_orbit_index);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL,
                                         "/HeaderData/VariableProductHeader/MainProductHeader/orbitNumber", NULL);
}

static void register_ac__tc__2b_product()
{
    harp_ingestion_module *module;
    harp_product_definition *product_definition;
    harp_variable_definition *variable_definition;
    harp_dimension_type dimension_type[2];
    const char *resolution_option_values[3] = { "medium", "low" };
    const char *description;

    description = "ATLID/CPR synergetic lidar/radar classification";
    module = harp_ingestion_register_module("ECA_AC__TC__2B", "EarthCARE", "EARTHCARE", "AC__TC__2B", description,
                                            ingestion_init, ingestion_done);

    description = "classification resolution: normal (default), medium (resolution=medium), or low (resolution=low)";
    harp_ingestion_register_option(module, "resolution", description, 2, resolution_option_values);

    product_definition = harp_ingestion_register_product(module, "ECA_AC__TC__2B", NULL, read_dimensions);

    register_common_variables(product_definition);

    dimension_type[0] = harp_dimension_time;
    dimension_type[1] = harp_dimension_vertical;

    /* altitude */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "altitude", harp_type_float,
                                                                     2, dimension_type, NULL,
                                                                     "joint standard grid height", "m", NULL,
                                                                     read_height);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/height", NULL);

    /* surface_height */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "surface_height",
                                                                     harp_type_float, 1, dimension_type, NULL,
                                                                     "elevation ", "m", NULL,
                                                                     read_elevation);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/elevation", NULL);

    /* scene_type */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "scene_type",
                                                                     harp_type_int8, 2, dimension_type, NULL,
                                                                     "synergetic target classification", NULL, NULL,
                                                                     read_synergetic_target_classification);
    harp_variable_definition_add_mapping(variable_definition, "resolution unset", NULL,
                                         "/ScienceData/synergetic_target_classification", NULL);
    harp_variable_definition_add_mapping(variable_definition, "resolution=medium", NULL,
                                         "/ScienceData/synergetic_target_classification_medium_resolution", NULL);
    harp_variable_definition_add_mapping(variable_definition, "resolution=low", NULL,
                                         "/ScienceData/synergetic_target_classification_low_resolution", NULL);

}

static void register_acm_cap_2b_product()
{
    harp_ingestion_module *module;
    harp_product_definition *product_definition;
    harp_variable_definition *variable_definition;
    harp_dimension_type dimension_type[2];
    const char *description;

    description = "ATLID/CPR/MSI cloud and aerosol properties";
    module = harp_ingestion_register_module("ECA_ACM_CAP_2B", "EarthCARE", "EARTHCARE", "ACM_CAP_2B", description,
                                            ingestion_init, ingestion_done);

    product_definition = harp_ingestion_register_product(module, "ECA_ACM_CAP_2B", NULL, read_dimensions);

    register_common_variables(product_definition);

    dimension_type[0] = harp_dimension_time;
    dimension_type[1] = harp_dimension_vertical;

    /* altitude */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "altitude", harp_type_float,
                                                                     2, dimension_type, NULL,
                                                                     "joint standard grid height", "m", NULL,
                                                                     read_height);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/height", NULL);

    /* liquid_water_density */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "liquid_water_density",
                                                                     harp_type_float, 2, dimension_type, NULL,
                                                                     "liquid water content", "kg/m3", NULL,
                                                                     read_liquid_water_content);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/liquid_water_content", NULL);

    /* liquid_water_extinction_coefficient */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition,
                                                                     "liquid_water_extinction_coefficient",
                                                                     harp_type_float, 2, dimension_type, NULL,
                                                                     "liquid extinction", "1/m", NULL,
                                                                     read_liquid_extinction);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/liquid_extinction", NULL);

    /* liquid_particle_effective_radius */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition,
                                                                     "liquid_particle_effective_radius",
                                                                     harp_type_float, 2, dimension_type, NULL,
                                                                     "liquid optical depth", "m",
                                                                     NULL, read_liquid_effective_radius);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/liquid_effective_radius", NULL);

    /* ice_water_density */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "ice_water_density",
                                                                     harp_type_float, 2, dimension_type, NULL,
                                                                     "ice water content", "kg/m3", NULL,
                                                                     read_ice_water_content);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/ice_water_content", NULL);

    /* ice_particle_effective_radius */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition,
                                                                     "ice_particle_effective_radius",
                                                                     harp_type_float, 2, dimension_type, NULL,
                                                                     "ice effective radius", "m", NULL,
                                                                     read_ice_effective_radius);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/ice_effective_radius", NULL);

    /* ice_water_mass_flux */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "ice_water_mass_flux",
                                                                     harp_type_float, 2, dimension_type, NULL,
                                                                     "ice mass flux", "kg/m2/s", NULL,
                                                                     read_ice_mass_flux);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/ice_mass_flux", NULL);

    /* ice_water_column_density */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "ice_water_column_density",
                                                                     harp_type_float, 1, dimension_type, NULL,
                                                                     "ice water path", "kg/m2", NULL,
                                                                     read_ice_water_path);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/ice_water_path", NULL);

    /* rain_rate */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "rain_rate", harp_type_float,
                                                                     2, dimension_type, NULL, "rain rate", "mm/h", NULL,
                                                                     read_rain_rate);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/rain_rate", NULL);

    /* rain_water_density */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "rain_water_density",
                                                                     harp_type_float, 2, dimension_type, NULL,
                                                                     "rain water content", "kg/m3", NULL,
                                                                     read_rain_water_content);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/rain_water_content", NULL);

    /* aerosol_number_density */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "aerosol_number_density",
                                                                     harp_type_float, 2, dimension_type, NULL,
                                                                     "aerosol number concentration", "1/m3", NULL,
                                                                     read_aerosol_number_concentration);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/aerosol_number_concentration",
                                         NULL);

    /* aerosol_extinction_coefficient */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition,
                                                                     "aerosol_extinction_coefficient", harp_type_float,
                                                                     2, dimension_type, NULL, "aerosol extinction",
                                                                     "1/m", NULL, read_aerosol_extinction);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/aerosol_extinction", NULL);

    /* aerosol_optical_depth */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "aerosol_optical_depth",
                                                                     harp_type_float, 1, dimension_type, NULL,
                                                                     "aerosol optical depth", HARP_UNIT_DIMENSIONLESS,
                                                                     NULL, read_aerosol_optical_depth);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/aerosol_optical_depth", NULL);

    /* aerosol_density */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "aerosol_density",
                                                                     harp_type_float, 2, dimension_type, NULL,
                                                                     "aerosol mass content", "kg/m3", NULL,
                                                                     read_aerosol_mass_content);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/aerosol_mass_content", NULL);

    /* validity */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "validity", harp_type_int8, 1,
                                                                     dimension_type, NULL, "quality status", NULL, NULL,
                                                                     read_quality_status);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/quality_status", NULL);
}

static void register_atl_aer_2a_product()
{
    harp_ingestion_module *module;
    harp_product_definition *product_definition;
    harp_variable_definition *variable_definition;
    harp_dimension_type dimension_type[2];
    const char *description;

    description = "ATLID aerosol inversion";
    module = harp_ingestion_register_module("ECA_ATL_AER_2A", "EarthCARE", "EARTHCARE", "ATL_AER_2A", description,
                                            ingestion_init, ingestion_done);

    product_definition = harp_ingestion_register_product(module, "ECA_ATL_AER_2A", NULL, read_dimensions);

    register_common_variables(product_definition);

    dimension_type[0] = harp_dimension_time;
    dimension_type[1] = harp_dimension_vertical;

    /* altitude */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "altitude", harp_type_float,
                                                                     2, dimension_type, NULL,
                                                                     "joint standard grid height", "m", NULL,
                                                                     read_height);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/height", NULL);

    /* surface_height */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "surface_height",
                                                                     harp_type_float, 1, dimension_type, NULL,
                                                                     "elevation ", "m", NULL,
                                                                     read_elevation);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/elevation", NULL);

    /* aerosol_extinction_coefficient */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition,
                                                                     "aerosol_extinction_coefficient",
                                                                     harp_type_float, 2, dimension_type, NULL,
                                                                     "particle extinction coefficient 355nm", "1/m",
                                                                     NULL, read_particle_extinction_coefficient_355nm);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL,
                                         "/ScienceData/particle_extinction_coefficient_355nm", NULL);

    /* aerosol_extinction_coefficient_uncertainty */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition,
                                                                     "aerosol_extinction_coefficient_uncertainty",
                                                                     harp_type_float, 2, dimension_type, NULL,
                                                                     "particle extinction coefficient 355nm error",
                                                                     "1/m", NULL,
                                                                     read_particle_extinction_coefficient_355nm_error);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL,
                                         "/ScienceData/particle_extinction_coefficient_355nm_error", NULL);

    /* aerosol_backscatter_coefficient */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition,
                                                                     "aerosol_backscatter_coefficient",
                                                                     harp_type_float, 2, dimension_type, NULL,
                                                                     "particle backscatter coefficient 355nm", "1/m/sr",
                                                                     NULL, read_particle_backscatter_coefficient_355nm);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL,
                                         "/ScienceData/particle_backscatter_coefficient_355nm", NULL);

    /* aerosol_backscatter_coefficient_uncertainty */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition,
                                                                     "aerosol_backscatter_coefficient_uncertainty",
                                                                     harp_type_float, 2, dimension_type, NULL,
                                                                     "particle backscatter coefficient 355nm error",
                                                                     "1/m/sr", NULL,
                                                                     read_particle_backscatter_coefficient_355nm_error);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL,
                                         "/ScienceData/particle_backscatter_coefficient_355nm_error", NULL);

    /* linear_depolarization_ratio */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "linear_depolarization_ratio",
                                                                     harp_type_float, 2, dimension_type, NULL,
                                                                     "particle linear depolarization ratio 355nm",
                                                                     HARP_UNIT_DIMENSIONLESS, NULL,
                                                                     read_particle_linear_depolarization_ratio_355nm);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL,
                                         "/ScienceData/particle_linear_depolarization_ratio_355nm", NULL);

    /* linear_depolarization_ratio_uncertainty */
    variable_definition =
        harp_ingestion_register_variable_full_read(product_definition, "linear_depolarization_ratio_uncertainty",
                                                   harp_type_float, 2, dimension_type, NULL,
                                                   "particle linear depolarization ratio 355nm error",
                                                   HARP_UNIT_DIMENSIONLESS, NULL,
                                                   read_particle_linear_depolarization_ratio_355nm_error);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL,
                                         "/ScienceData/particle_linear_depolarization_ratio_355nm_error", NULL);

    /* lidar_ratio */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "lidar_ratio", harp_type_float,
                                                                     2, dimension_type, NULL, "lidar ratio 355nm", "sr",
                                                                     NULL, read_lidar_ratio_355nm);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/lidar_ratio_355nm", NULL);

    /* lidar_ratio_uncertainty */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "lidar_ratio_uncertainty",
                                                                     harp_type_float, 2, dimension_type, NULL,
                                                                     "lidar ratio 355nm error", "sr",
                                                                     NULL, read_lidar_ratio_355nm_error);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/lidar_ratio_355nm_error", NULL);

    /* tropopause_height */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "tropopause_height",
                                                                     harp_type_float, 1, dimension_type, NULL,
                                                                     "tropopause height", "m", NULL,
                                                                     read_tropopause_height);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/tropopause_height", NULL);

    /* aerosol_type */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "aerosol_type", harp_type_int8,
                                                                     2, dimension_type, NULL, "aerosol classification",
                                                                     NULL, NULL, read_aerosol_classification);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/aerosol_classification", NULL);

    /* scene_type */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "scene_type", harp_type_int8,
                                                                     2, dimension_type, NULL, "classification", NULL,
                                                                     NULL, read_classification);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/classification", NULL);

    /* validity */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "validity", harp_type_int8, 2,
                                                                     dimension_type, NULL, "quality status", NULL, NULL,
                                                                     read_quality_status_2d);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, "/ScienceData/quality_status", NULL);

    /* wavelength */
    variable_definition = harp_ingestion_register_variable_full_read(product_definition, "wavelength", harp_type_float,
                                                                     0, NULL, NULL, "lidar wavelength", "nm", NULL,
                                                                     read_355nm);
    harp_variable_definition_add_mapping(variable_definition, NULL, NULL, NULL, "set to fixed value of 355nm");

}

int harp_ingestion_module_earthcare_l2_init(void)
{
    register_ac__tc__2b_product();
    register_acm_cap_2b_product();
    register_atl_aer_2a_product();

    return 0;
}
