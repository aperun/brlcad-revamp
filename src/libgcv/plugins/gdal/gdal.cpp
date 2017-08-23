/*                             G D A L . C P P
 * BRL-CAD
 *
 * Copyright (c) 2013 Tom Browder
 * Copyright (c) 2017 United States Government as represented by the U.S. Army
 * Research Laboratory.
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
 */
/** @file gdal.cpp
 *
 * Geospatial Data Abstraction Library plugin for libgcv.
 *
 */


#include "common.h"
#include "vmath.h"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

/* GDAL headers */
#include <gdal.h>
#include <gdal_utils.h>
#include <cpl_conv.h>
#include <cpl_string.h>
#include <cpl_multiproc.h>
#include <ogr_spatialref.h>

#include "raytrace.h"
#include "gcv/api.h"
#include "gcv/util.h"

struct gdal_read_options
{
    const char *t_srs;
    const char *fmt;
};

struct conversion_state {
    const struct gcv_opts *gcv_options;
    const struct gdal_read_options *ops;
    const char *input_file;
    struct rt_wdb *wdbp;

    /* GDAL state */
    OGRSpatialReference *sp;
    GDALDatasetH hDataset;
    bool info;
    bool debug;
    int scalex;
    int scaley;
    int scalez;
    double adfGeoTransform[6];
    int az;
    int el;
    int pixsize;
};

HIDDEN void
gdal_state_init(struct conversion_state *gs)
{
    int i = 0;
    if(!gs) return;
    gs->gcv_options = NULL;
    gs->ops = NULL;
    gs->input_file = NULL;
    gs->wdbp = RT_WDB_NULL;

    gs->sp = NULL;
    gs->hDataset = NULL;
    gs->info = false;
    gs->debug = false;
    gs->scalex = 1;
    gs->scaley = 1;
    gs->scalez = 1;
    for(i = 0; i < 6; i++) gs->adfGeoTransform[i] = 0.0;
    gs->az = 35;
    gs->el = 35;
    gs->pixsize = 512 * 3;
}

HIDDEN int
get_dataset_info(struct conversion_state *gs)
{
    char *gdal_info = GDALInfo(gs->hDataset, NULL);
    if (gdal_info) bu_log("%s", gdal_info);
    CPLFree(gdal_info);
    return 0;
}

HIDDEN int
gdal_can_read(const char *data)
{
    GDALDatasetH hDataset;
    GDALAllRegister();

    if (!data) return 0;

    if (!bu_file_exists(data,NULL)) return 0;

    hDataset = GDALOpenEx(data, GDAL_OF_READONLY | GDAL_OF_RASTER | GDAL_OF_VERBOSE_ERROR, NULL, NULL, NULL);

    if (!hDataset) return 0;

    GDALClose(hDataset);

    return 1;
}

HIDDEN int
gdal_read(struct gcv_context *context, const struct gcv_opts *gcv_options,
	               const void *options_data, const char *source_path)
{
    struct conversion_state *state;
    BU_GET(state, struct conversion_state);
    gdal_state_init(state);
    state->gcv_options = gcv_options;
    state->ops = (struct gdal_read_options *)options_data;
    state->input_file = source_path;
    state->wdbp = context->dbip->dbi_wdbp;

    GDALAllRegister();

    state->hDataset = GDALOpenEx(source_path, GDAL_OF_READONLY | GDAL_OF_RASTER | GDAL_OF_VERBOSE_ERROR, NULL, NULL, NULL);
    if (!state->hDataset) {
	bu_log("GDAL Reader: error opening input file %s\n", source_path);
	BU_PUT(state, struct conversion_state);
	return 0;
    }

    (void)get_dataset_info(state);

    /* Read in the data */
    unsigned int xsize = GDALGetRasterXSize(state->hDataset);
    unsigned int ysize = GDALGetRasterYSize(state->hDataset);
    {
	struct rt_binunif_internal *bip;
	BU_ALLOC(bip, struct rt_binunif_internal);
	bip->magic = RT_BINUNIF_INTERNAL_MAGIC;
	bip->type = DB5_MINORTYPE_BINU_16BITINT_U;
	bip->count = xsize * ysize;
	if (bip->count < xsize || bip->count < ysize) {
	    bu_log("Error reading GDAL data\n");
	    bu_free(bip, "bip");
	    return 0;
	}
	bip->u.int8 = (char *)bu_calloc(bip->count, sizeof(unsigned short), "unsigned short array");

	GDALRasterBandH band = GDALGetRasterBand(state->hDataset, 1);

	/*
	 int bmin, bmax;
	 double adfMinMax[2];
	 adfMinMax[0] = GDALGetRasterMinimum(band, &bmin);
	 adfMinMax[1] = GDALGetRasterMaximum(band, &bmax);
	 if (!(bmin && bmax)) GDALComputeRasterMinMax(band, TRUE, adfMinMax);
	 bu_log("Min/Max: %f, %f\n", adfMinMax[0], adfMinMax[1]);
	 */

	/* If we're going to DSP we need the unsigned short read. */
	uint16_t *scanline = (uint16_t *)CPLMalloc(sizeof(uint16_t)*GDALGetRasterBandXSize(band));
	for(int i = 0; i < GDALGetRasterBandYSize(band); i++) {
	    if (GDALRasterIO(band, GF_Read, 0, i, GDALGetRasterBandXSize(band), 1, scanline, GDALGetRasterBandXSize(band), 1, GDT_UInt16, 0, 0) == CPLE_None) {
		for (int j = 0; j < GDALGetRasterBandXSize(band); ++j) {
		    /* This is the critical assignment point - if we get this
		     * indexing wrong, data will not look right in dsp */
		    bip->u.int16[i*ysize+j] = scanline[j];
		}
	    }
	}

	wdb_export(state->wdbp, "test.data", (void *)bip, ID_BINUNIF, 1);

	/* Done reading - close input file */
	CPLFree(scanline);
	GDALClose(state->hDataset);
    }

    /* TODO: if we're going to BoT (3-space mesh, will depend on the transform requested) we will need different logic... */

    /* Write out the dsp */
    {
	struct rt_dsp_internal *dsp;
	BU_ALLOC(dsp, struct rt_dsp_internal);
	dsp->magic = RT_DSP_INTERNAL_MAGIC;

	bu_vls_init(&dsp->dsp_name);
	bu_vls_strcpy(&dsp->dsp_name, "test.data");
	dsp->dsp_datasrc = RT_DSP_SRC_OBJ;
	dsp->dsp_xcnt = xsize;
	dsp->dsp_ycnt = ysize;
	dsp->dsp_smooth = 1;
	dsp->dsp_cuttype = DSP_CUT_DIR_ADAPT;
	MAT_IDN(dsp->dsp_stom);
	bn_mat_inv(dsp->dsp_mtos, dsp->dsp_stom);

	wdb_export(state->wdbp, "test.s", (void *)dsp, ID_DSP, 1);
    }

    return 1;
}

extern "C"
{
    struct gcv_filter gcv_conv_gdal_read =
    {"GDAL Reader", GCV_FILTER_READ, BU_MIME_MODEL_AUTO, gdal_can_read, NULL, NULL, gdal_read};

    static const struct gcv_filter * const filters[] = {&gcv_conv_gdal_read, NULL};
    const struct gcv_plugin gcv_plugin_info_s = { filters };
    GCV_EXPORT const struct gcv_plugin *gcv_plugin_info(){return &gcv_plugin_info_s;}
}

// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
