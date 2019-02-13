/******************************************************************************
 *
 * Project:  EDIGEO Translator
 * Purpose:  Implements OGREDIGEODriver.
 * Author:   Even Rouault, even dot rouault at mines dash paris dot org
 *
 ******************************************************************************
 * Copyright (c) 2011, Even Rouault <even dot rouault at mines-paris dot org>
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

#include "ogr_edigeo.h"
#include "cpl_conv.h"

CPL_CVSID("$Id: ogredigeodriver.cpp 34819 2016-07-28 22:32:18Z goatbar $");

extern "C" void RegisterOGREDIGEO();

// g++ -fPIC -g -Wall ogr/ogrsf_frmts/edigeo/*.cpp -shared -o ogr_EDIGEO.so -Iport -Igcore -Iogr -Iogr/ogrsf_frmts -Iogr/ogrsf_frmts/generic -Iogr/ogrsf_frmts/edigeo -L. -lgdal

/************************************************************************/
/*                        OGREDIGEODriverIdentify()                     */
/************************************************************************/

static int OGREDIGEODriverIdentify( GDALOpenInfo * poOpenInfo )

{
    return poOpenInfo->fpL != NULL &&
           EQUAL(CPLGetExtension(poOpenInfo->pszFilename), "thf");
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

static GDALDataset *OGREDIGEODriverOpen( GDALOpenInfo * poOpenInfo )

{
    if( poOpenInfo->eAccess == GA_Update ||
        !OGREDIGEODriverIdentify(poOpenInfo) )
        return NULL;

    OGREDIGEODataSource   *poDS = new OGREDIGEODataSource();

    if( !poDS->Open( poOpenInfo->pszFilename ) )
    {
        delete poDS;
        poDS = NULL;
    }

    return poDS;
}

/************************************************************************/
/*                           RegisterOGREDIGEO()                        */
/************************************************************************/

void RegisterOGREDIGEO()

{
    if( GDALGetDriverByName( "EDIGEO" ) != NULL )
        return;

    GDALDriver  *poDriver = new GDALDriver();

    poDriver->SetDescription( "EDIGEO" );
    poDriver->SetMetadataItem( GDAL_DCAP_VECTOR, "YES" );
    poDriver->SetMetadataItem( GDAL_DMD_LONGNAME,
                               "French EDIGEO exchange format" );
    poDriver->SetMetadataItem( GDAL_DMD_EXTENSION, "thf" );
    poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC, "drv_edigeo.html" );

    poDriver->SetMetadataItem( GDAL_DCAP_VIRTUALIO, "YES" );

    poDriver->pfnOpen = OGREDIGEODriverOpen;
    poDriver->pfnIdentify = OGREDIGEODriverIdentify;

    GetGDALDriverManager()->RegisterDriver( poDriver );
}
