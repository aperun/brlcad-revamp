
/*
* NIST Utils Class Library
* clutils/gennodearray.h
* April 1997
* David Sauder
* K. C. Morris

* Development of this software was funded by the United States Government,
* and is not subject to copyright.
*/

/* $Id: gennodearray.cc,v 3.0.1.4 1997/11/05 22:33:48 sauderd DP3.1 $  */

#ifdef HAVE_CONFIG_H
# include <scl_cf.h>
#endif

#include <gennode.h>
#include <gennodelist.h>
#include <gennodearray.h>

#ifndef HAVE_MEMMOVE
extern "C" {
    extern void * memmove(void *, const void *, size_t);
}
#endif

GenericNode::GenericNode()
{
    next = 0; 
    prev = 0; 
}

GenericNode::~GenericNode()
{
    Remove();
}

GenNodeArray::GenNodeArray (int defaultSize)
{
    _bufsize = defaultSize;
    _buf = new GenericNode*[_bufsize];
    memset(_buf, 0, _bufsize*sizeof(GenericNode*));
    _count = 0;
}

GenNodeArray::~GenNodeArray ()
{

//    int i;
	// this is dangerous because several things point at these nodes
	// also whatever is derived from this thing might do this
//    for(i = 0; i < _count; i++)
//	delete _buf[i];
    delete [] _buf;
}

int GenNodeArray::Index (GenericNode** gn)
{
    return ((gn - _buf) / sizeof(GenericNode*));
}

void GenNodeArray::Append(GenericNode* gn)
{    
    Insert(gn, _count); 
}

int GenNodeArray::Insert(GenericNode* gn)
{
    return Insert(gn, _count); 
}

void 
GenNodeArray::Check (int index)
{
    GenericNode** newbuf;

    if (index >= _bufsize) {
        _bufsize = (index+1) * 2;
        newbuf = new GenericNode*[_bufsize];

	memset(newbuf, 0, _bufsize*sizeof(GenericNode*));
        memmove(newbuf, _buf, _count*sizeof(GenericNode*));
	delete [] _buf;
        _buf = newbuf;
    }
}

int 
GenNodeArray::Insert (GenericNode* gn, int index) 
{
    const GenericNode** spot;
    index = (index < 0) ? _count : index;

    if (index < _count) {
        Check(_count+1);
        spot = (const GenericNode**)&_buf[index];
//        bcopy(spot, spot+1, (_count - index)*sizeof(GenericNode*));
// Josh L, 5/2/95
//        memcpy(spot+1, spot, (_count - index)*sizeof(GenericNode*));
// Dave memcpy is not working since memory areas overlap
        memmove(spot+1, spot, (_count - index)*sizeof(GenericNode*));

    } else {
        Check(index);
        spot = (const GenericNode**)&_buf[index];
    }
    *spot = gn;
    ++_count;
    return index;
}

void 
GenNodeArray::Remove (int index) 
{
    if (0 <= index && index < _count) {
        --_count;
        const GenericNode** spot = (const GenericNode**)&_buf[index];
//        bcopy(spot+1, spot, (_count - index)*sizeof(GenericNode*));
// Josh L, 5/2/95
//        memcpy(spot, spot+1, (_count - index)*sizeof(GenericNode*));
// Dave memcpy is not working since memory areas overlap
        memmove(spot, spot+1, (_count - index)*sizeof(GenericNode*));
	_buf[_count] = 0;
    }
}

void GenNodeArray::ClearEntries ()
{
//    if(debug_level >= PrintFunctionTrace)
//	cout << "GenNodeArray::Clear()\n";
    int i;
    for(i = 0 ; i < _count; i++)
	_buf[i] = 0;
    _count = 0;
}

void GenNodeArray::DeleteEntries()
{
//    if(debug_level >= PrintFunctionTrace)
//	cout << "GenNodeArray::DeleteEntries()\n";
    int i;
    for(i = 0 ; i < _count; i++)
	delete (_buf[i]);
    _count = 0;
}


int GenNodeArray::Index (GenericNode* gn)
{
//    if(debug_level >= PrintFunctionTrace)
//	cout << "GenNodeArray::Index()\n";
    for (int i = 0; i < _count; ++i) {
        if (_buf[i] == gn) {
            return i;
        }
    }
    return -1;
}

