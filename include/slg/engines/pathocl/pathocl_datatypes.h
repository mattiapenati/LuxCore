/***************************************************************************
 * Copyright 1998-2013 by authors (see AUTHORS.txt)                        *
 *                                                                         *
 *   This file is part of LuxRender.                                       *
 *                                                                         *
 * Licensed under the Apache License, Version 2.0 (the "License");         *
 * you may not use this file except in compliance with the License.        *
 * You may obtain a copy of the License at                                 *
 *                                                                         *
 *     http://www.apache.org/licenses/LICENSE-2.0                          *
 *                                                                         *
 * Unless required by applicable law or agreed to in writing, software     *
 * distributed under the License is distributed on an "AS IS" BASIS,       *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
 * See the License for the specific language governing permissions and     *
 * limitations under the License.                                          *
 ***************************************************************************/

#ifndef _SLG_PATHOCL_DATATYPES_H
#define	_SLG_PATHOCL_DATATYPES_H

#if !defined(LUXRAYS_DISABLE_OPENCL)

#include "slg/slg.h"

namespace slg { namespace ocl { namespace pathocl {
#include "slg/engines/pathocl/kernels/pathocl_datatypes.cl"
} } }

#endif

#endif	/* _SLG_OCLDATATYPES_H */
