/***************************************************************************
 *   Copyright (C) 1998-2010 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of LuxRays.                                         *
 *                                                                         *
 *   LuxRays is free software; you can redistribute it and/or modify       *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   LuxRays is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   LuxRays website: http://www.luxrender.net                             *
 ***************************************************************************/

#if !defined(LUXRAYS_DISABLE_OPENCL)

#include "slg.h"
#include "pathocl/rtpathocl.h"
#include "luxrays/opencl/intersectiondevice.h"

using namespace std;
using namespace luxrays;
using namespace luxrays::sdl;
using namespace luxrays::utils;

namespace slg {

// To check if it is still required
// NOTE: WallClockTime() return the time in seconds.
//#ifdef _WIN32
//static double PreciseClockTime()
//{
//	unsigned __int64 t, freq;
//	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
//	QueryPerformanceCounter((LARGE_INTEGER*)&t);
//	return (double)t/freq;
//}
//#else
#define PreciseClockTime WallClockTime
//#endif

//------------------------------------------------------------------------------
// RTPathOCLRenderThread
//------------------------------------------------------------------------------

RTPathOCLRenderThread::RTPathOCLRenderThread(const u_int index,
	OpenCLIntersectionDevice *device, PathOCLRenderEngine *re) : 
	PathOCLRenderThread(index, device, re) {
	assignedIters = ((RTPathOCLRenderEngine*)renderEngine)->renderConfig->GetMinIterationsToShow();
	frameTime = 0.0;
}

RTPathOCLRenderThread::~RTPathOCLRenderThread() {
}

void RTPathOCLRenderThread::Interrupt() {
}

void RTPathOCLRenderThread::BeginEdit() {
	editMutex.lock();
}

void RTPathOCLRenderThread::EndEdit(const EditActionList &editActions) {
	if (editActions.Has(FILM_EDIT) || editActions.Has(MATERIAL_TYPES_EDIT)) {
		editMutex.unlock();
		StopRenderThread();

		updateActions.AddActions(editActions.GetActions());
		UpdateOCLBuffers();
		StartRenderThread();
	} else {
		updateActions.AddActions(editActions.GetActions());
		editMutex.unlock();
	}
}

void RTPathOCLRenderThread::UpdateOCLBuffers() {
	editMutex.lock();
	//--------------------------------------------------------------------------
	// Update OpenCL buffers
	//--------------------------------------------------------------------------

	if (updateActions.Has(FILM_EDIT)) {
		// Resize the Frame Buffer
		InitFrameBuffer();
	}

	if (updateActions.Has(CAMERA_EDIT)) {
		// Update Camera
		InitCamera();
	}

	if (updateActions.Has(GEOMETRY_EDIT)) {
		// Update Scene Geometry
		InitGeometry();
	}

	if (updateActions.Has(MATERIALS_EDIT) || updateActions.Has(MATERIAL_TYPES_EDIT)) {
		// Update Scene Materials
		InitMaterials();
	}

	if  (updateActions.Has(AREALIGHTS_EDIT)) {
		// Update Scene Area Lights
		InitTriangleAreaLights();
	}

	if  (updateActions.Has(INFINITELIGHT_EDIT)) {
		// Update Scene Infinite Light
		InitInfiniteLight();
	}

	if  (updateActions.Has(SUNLIGHT_EDIT)) {
		// Update Scene Sun Light
		InitSunLight();
	}

	if  (updateActions.Has(SKYLIGHT_EDIT)) {
		// Update Scene Sun Light
		InitSkyLight();
	}

	//--------------------------------------------------------------------------
	// Recompile Kernels if required
	//--------------------------------------------------------------------------

	if (updateActions.Has(FILM_EDIT) || updateActions.Has(MATERIAL_TYPES_EDIT))
		InitKernels();

	updateActions.Reset();
	editMutex.unlock();

	SetKernelArgs();

	//--------------------------------------------------------------------------
	// Execute initialization kernels
	//--------------------------------------------------------------------------

	cl::CommandQueue &oclQueue = intersectionDevice->GetOpenCLQueue();

	// Clear the frame buffer
	oclQueue.enqueueNDRangeKernel(*initFBKernel, cl::NullRange,
		cl::NDRange(RoundUp<u_int>(frameBufferPixelCount, initFBWorkGroupSize)),
		cl::NDRange(initFBWorkGroupSize));

	// Initialize the tasks buffer
	oclQueue.enqueueNDRangeKernel(*initKernel, cl::NullRange,
		cl::NDRange(renderEngine->taskCount), cl::NDRange(initWorkGroupSize));

	// Reset statistics in order to be more accurate
	intersectionDevice->ResetPerformaceStats();
}

void RTPathOCLRenderThread::RenderThreadImpl() {
	//SLG_LOG("[RTPathOCLRenderThread::" << threadIndex << "] Rendering thread started");

	cl::CommandQueue &oclQueue = intersectionDevice->GetOpenCLQueue();

	try {
		boost::barrier *frameBarrier = ((RTPathOCLRenderEngine *)renderEngine)->frameBarrier;
		
		while (!boost::this_thread::interruption_requested()) {
			if (updateActions.HasAnyAction())
				UpdateOCLBuffers();

			//------------------------------------------------------------------
			// Render a frame (i.e. taskCount * assignedIters samples)
			//------------------------------------------------------------------
			const double startTime = PreciseClockTime();
			u_int iterations = assignedIters;

			for (u_int i = 0; i < iterations; ++i) {
				// Trace rays
				intersectionDevice->EnqueueTraceRayBuffer(*raysBuff,
					*(hitsBuff), renderEngine->taskCount, NULL, NULL);
				// Advance to next path state
				oclQueue.enqueueNDRangeKernel(*advancePathsKernel, cl::NullRange,
					cl::NDRange(renderEngine->taskCount), cl::NDRange(advancePathsWorkGroupSize));
			}

			// Async. transfer of the frame buffer
			oclQueue.enqueueReadBuffer(*frameBufferBuff, CL_FALSE, 0,
				frameBufferBuff->getInfo<CL_MEM_SIZE>(), frameBuffer);
			// Check if I have to transfer the alpha channel too
			if (alphaFrameBufferBuff) {
				// Async. transfer of the alpha channel
				oclQueue.enqueueReadBuffer(*alphaFrameBufferBuff, CL_FALSE, 0,
					alphaFrameBufferBuff->getInfo<CL_MEM_SIZE>(), alphaFrameBuffer);
			}
			// Async. transfer of GPU task statistics
			oclQueue.enqueueReadBuffer(*(taskStatsBuff), CL_FALSE, 0,
				sizeof(slg::ocl::GPUTaskStats) * renderEngine->taskCount, gpuTaskStats);

			oclQueue.finish();
			frameTime = PreciseClockTime() - startTime;
			frameBarrier->wait();
			// Main thread re-balance assigned iterations to each task and
			// merge all frame buffers
			frameBarrier->wait();
		}
		//SLG_LOG("[RTPathOCLRenderThread::" << threadIndex << "] Rendering thread halted");
	} catch (boost::thread_interrupted) {
		SLG_LOG("[RTPathOCLRenderThread::" << threadIndex << "] Rendering thread halted");
	} catch (cl::Error err) {
		SLG_LOG("[RTPathOCLRenderThread::" << threadIndex << "] Rendering thread ERROR: " << err.what() <<
				"(" << luxrays::utils::oclErrorString(err.err()) << ")");
	}
	oclQueue.finish();
}

}

#endif
