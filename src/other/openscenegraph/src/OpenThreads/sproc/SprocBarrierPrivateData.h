/* -*-c++-*- OpenThreads library, Copyright (C) 2002 - 2007  The Open Thread Group
 *
 * This library is open source and may be redistributed and/or modified under  
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or 
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * OpenSceneGraph Public License for more details.
*/


// SprocBarrierPrivateData.h - private data structure for barrier
// ~~~~~~~~~~~~~~~~~~~~~~~~~

#ifndef _SPROCBARRIERPRIVATEDATA_H_
#define _SPROCBARRIERPRIVATEDATA_H_

#include <ulocks.h>
#include <OpenThreads/Barrier>

#ifndef USE_IRIX_NATIVE_BARRIER

#include <OpenThreads/Condition>
#include <OpenThreads/Mutex>

#endif

namespace OpenThreads {

class SprocBarrierPrivateData {

    friend class Barrier;

private:

    SprocBarrierPrivateData() {};
    
    virtual ~SprocBarrierPrivateData() {};

#ifdef USE_IRIX_NATIVE_BARRIER

    barrier_t *barrier;

    unsigned int numThreads;

#else

    OpenThreads::Condition _cond;
    
    OpenThreads::Mutex _mutex;
    
    volatile int maxcnt;
    
    volatile int cnt;

    volatile int phase;

#endif

};

}

#endif //_SPROCBARRIERPRIVATEDATA_H_
