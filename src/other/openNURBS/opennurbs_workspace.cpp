/* $NoKeywords: $ */
/*
//
// Copyright (c) 1993-2007 Robert McNeel & Associates. All rights reserved.
// Rhinoceros is a registered trademark of Robert McNeel & Assoicates.
//
// THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY.
// ALL IMPLIED WARRANTIES OF FITNESS FOR ANY PARTICULAR PURPOSE AND OF
// MERCHANTABILITY ARE HEREBY DISCLAIMED.
//				
// For complete openNURBS copyright information see <http://www.opennurbs.org>.
//
////////////////////////////////////////////////////////////////
*/

#include "opennurbs.h"

ON_Workspace::ON_Workspace() : m_pFileBlk(0), m_pMemBlk(0)
{}

ON_Workspace::~ON_Workspace()
{
  struct FBLK* pFileBlk = m_pFileBlk;
  while ( pFileBlk ) {
    if ( pFileBlk->pFile )
      fclose( pFileBlk->pFile );
    pFileBlk = pFileBlk->pNext;
  }

  struct MBLK* pNext = m_pMemBlk;
  struct MBLK* p = NULL;
  while ( pNext ) {
    p = pNext;
    pNext = pNext->pNext;
    if ( p->pMem ) {
      onfree(p->pMem);
      p->pMem = NULL;
    }
    onfree( p );
  }
}

void* ON_Workspace::GetMemory( size_t size )
{
  void* p = NULL;
  if ( size > 0 ) 
  {
    struct MBLK* pBlk = (struct MBLK*)onmalloc(sizeof(*pBlk));
    if ( pBlk ) 
    {
      pBlk->pMem = p = onmalloc(size);
      pBlk->pNext = m_pMemBlk;
      m_pMemBlk = pBlk;
    }
  }
  return p;
}

void* ON_Workspace::GrowMemory( void* p, size_t size )
{
  void* newp = NULL;
  if ( !p ) {
    newp = GetMemory(size);
  }
  else {
    struct MBLK* pBlk = m_pMemBlk;
    while ( pBlk ) {
      if ( pBlk->pMem == p ) {
        if ( size > 0 ) {
          newp = onrealloc(p,size);
        }
        else {
          newp = p;
        }
        pBlk->pMem = newp;
        break;
      }
      pBlk = pBlk->pNext;
    }
  }
  return newp;
}

int ON_Workspace::KeepMemory( void* p )
{
  int rc = false;
  if ( p ) {
    struct MBLK* pPrevBlk = NULL;
    struct MBLK* pBlk = m_pMemBlk;
    while ( pBlk ) {
      if ( pBlk->pMem == p ) {
        // Remove pBlk from list so ~ON_Workspace() won't onfree() its memory
        // and any future GrowMemory...() or KeepMemory() calls won't have
        // to search past it.
        pBlk->pMem = NULL;
        if ( pPrevBlk ) {
          pPrevBlk->pNext = pBlk->pNext;
        }
        else {
          m_pMemBlk = pBlk->pNext;
        }
        onfree( pBlk );
        rc = true;
        break;
      }
      pPrevBlk = pBlk;
      pBlk = pBlk->pNext;
    }
  }
  return rc;
}

int* ON_Workspace::GetIntMemory( size_t size )
{
  int* pi = (int*)(GetMemory(size*sizeof(*pi)));
  return pi;
}

double* ON_Workspace::GetDoubleMemory( size_t size )
{
  double* pd = (double*)(GetMemory(size*sizeof(*pd)));
  return pd;
}

ON_3dPoint* ON_Workspace::GetPointMemory( size_t size )
{
  ON_3dPoint* p3d = (ON_3dPoint*)(GetMemory(size*sizeof(*p3d)));
  return p3d;
}

ON_3dVector* ON_Workspace::GetVectorMemory( size_t size )
{
  ON_3dVector* v3d = (ON_3dVector*)(GetMemory(size*sizeof(*v3d)));
  return v3d;
}

int** ON_Workspace::GetIntMemory( size_t row_count, size_t col_count )
{
  int** p = 0;
  size_t i;
  if ( row_count > 0 && col_count > 0 )
  {
    p = (int**)GetMemory(row_count*(sizeof(*p) + col_count*sizeof(**p)));
    if ( p )
    {
      p[0] = (int*)(p+row_count);
      for( i = 1; i < row_count; i++ )
      {
        p[i] = p[i-1] + col_count;
      }
    }
  }
  return p;
}

double** ON_Workspace::GetDoubleMemory( size_t row_count, size_t col_count )
{
  double** p = 0;
  size_t i;
  if ( row_count > 0 && col_count > 0 )
  {
    // i keeps doubles aligned
    i = (sizeof(*p) < sizeof(**p))
      ? (row_count + (row_count%2))
      : row_count;
    p = (double**)GetMemory(i*sizeof(*p) + row_count*col_count*sizeof(**p));
    if ( p )
    {
      p[0] = (double*)(p+i);
      for( i = 1; i < row_count; i++ )
      {
        p[i] = p[i-1] + col_count;
      }
    }
  }
  return p;
}


int* ON_Workspace::GrowIntMemory( int* p, size_t size )
{
  int* pi = (int*)(GrowMemory(p,size*sizeof(*pi)));
  return pi;
}

double* ON_Workspace::GrowDoubleMemory( double* p, size_t size )
{
  double* pd = (double*)(GrowMemory(p,size*sizeof(*pd)));
  return pd;
}

ON_3dPoint* ON_Workspace::GrowPointMemory( ON_3dPoint* p, size_t size )
{
  ON_3dPoint* p3d = (ON_3dPoint*)(GrowMemory(p,size*sizeof(*p3d)));
  return p3d;
}

ON_3dVector* ON_Workspace::GrowVectorMemory( ON_3dVector* p, size_t size )
{
  ON_3dVector* v3d = (ON_3dVector*)(GrowMemory(p,size*sizeof(*v3d)));
  return v3d;
}


FILE* ON_Workspace::OpenFile( const char* sFileName, const char* sMode ) 
{
  FILE* pFile = ON::OpenFile( sFileName, sMode );
  if ( pFile ) 
  {
    struct FBLK* pFileBlk = (struct FBLK*)GetMemory( sizeof(*pFileBlk) );
    pFileBlk->pNext = m_pFileBlk;
    pFileBlk->pFile = pFile;
    m_pFileBlk = pFileBlk;
  }
  return pFile;
}

FILE* ON_Workspace::OpenFile( const wchar_t* sFileName, const wchar_t* sMode ) 
{
  FILE* pFile = ON::OpenFile( sFileName, sMode );
  if ( pFile ) 
  {
    struct FBLK* pFileBlk = (struct FBLK*)GetMemory( sizeof(*pFileBlk) );
    pFileBlk->pNext = m_pFileBlk;
    pFileBlk->pFile = pFile;
    m_pFileBlk = pFileBlk;
  }
  return pFile;
}

int ON_Workspace::KeepFile( FILE* pFile )
{
  int rc = false;
  if ( pFile ) {
    struct FBLK* pFileBlk = m_pFileBlk;
    while ( pFileBlk ) {
      if ( pFileBlk->pFile == pFile ) {
        pFileBlk->pFile = NULL;
        rc = true;
        break;
      }
      pFileBlk = pFileBlk->pNext;
    }
  }
  return rc;
}
