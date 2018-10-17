/******************************************************************************
 * $Id: reader_kompsat.h 36501 2016-11-25 14:09:24Z rouault $
 *
 * Project:  GDAL Core
 * Purpose:  Read metadata from Kompsat imagery.
 * Author:   Alexander Lisovenko
 * Author:   Dmitry Baryshnikov, polimax@mail.ru
 *
 ******************************************************************************
 * Copyright (c) 2014-2015 NextGIS <info@nextgis.ru>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#ifndef READER_KOMPSAT_H_INCLUDED
#define READER_KOMPSAT_H_INCLUDED

#include "reader_pleiades.h"

/**
@brief Metadata reader for Kompsat

TIFF filename:      aaaaaaaaaa.tif
Metadata filename:  aaaaaaaaaa.eph aaaaaaaaaa.txt
RPC filename:       aaaaaaaaaa.rpc

Common metadata (from metadata filename):
    SatelliteId:            AUX_SATELLITE_NAME
    AcquisitionDateTime:    IMG_ACQISITION_START_TIME, IMG_ACQISITION_END_TIME
*/

class GDALMDReaderKompsat: public GDALMDReaderBase
{
public:
    GDALMDReaderKompsat(const char *pszPath, char **papszSiblingFiles);
    virtual ~GDALMDReaderKompsat();
    virtual bool HasRequiredFiles() const override;
    virtual char** GetMetadataFiles() const override;
protected:
    virtual void LoadMetadata() override;
    char** ReadTxtToList();
    virtual time_t GetAcquisitionTimeFromString(const char* pszDateTime) override;
protected:
    CPLString m_osIMDSourceFilename;
    CPLString m_osRPBSourceFilename;
};

#endif // READER_KOMPSAT_H_INCLUDED

