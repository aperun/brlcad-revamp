/*
Bullet Continuous Collision Detection and Physics Library, Copyright (c) 2007 Erwin Coumans

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

*/

#ifndef MINICL__TASK_H
#define MINICL__TASK_H

#include "BulletMultiThreaded/PlatformDefinitions.h"
#include "LinearMath/btScalar.h"

#include "LinearMath/btAlignedAllocator.h"


#define MINICL_MAX_ARGLENGTH (sizeof(void*))
#define MINI_CL_MAX_ARG 16
#define MINI_CL_MAX_KERNEL_NAME 256

struct MiniCLKernel;

ATTRIBUTE_ALIGNED16(struct) MiniCLTaskDesc
{
	BT_DECLARE_ALIGNED_ALLOCATOR();

	MiniCLTaskDesc()
	{
		for (int i=0;i<MINI_CL_MAX_ARG;i++)
		{
			m_argSizes[i]=0;
		}
	}

	uint32_t		m_taskId;

	uint32_t		m_firstWorkUnit;
	uint32_t		m_lastWorkUnit;

	MiniCLKernel*	m_kernel;

	void*			m_argData[MINI_CL_MAX_ARG];
	int				m_argSizes[MINI_CL_MAX_ARG];
};

extern "C" int gMiniCLNumOutstandingTasks;


void	processMiniCLTask(void* userPtr, void* lsMemory);
void*	createMiniCLLocalStoreMemory();


#endif //MINICL__TASK_H

