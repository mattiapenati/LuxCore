#include <string>
namespace luxrays { namespace ocl {
std::string KernelSource_matrix4x4_funcs = 
"#line 2 \"matrix4x4_funcs.cl\"\n"
"\n"
"/***************************************************************************\n"
" * Copyright 1998-2013 by authors (see AUTHORS.txt)                        *\n"
" *                                                                         *\n"
" *   This file is part of LuxRender.                                       *\n"
" *                                                                         *\n"
" * Licensed under the Apache License, Version 2.0 (the \"License\");         *\n"
" * you may not use this file except in compliance with the License.        *\n"
" * You may obtain a copy of the License at                                 *\n"
" *                                                                         *\n"
" *     http://www.apache.org/licenses/LICENSE-2.0                          *\n"
" *                                                                         *\n"
" * Unless required by applicable law or agreed to in writing, software     *\n"
" * distributed under the License is distributed on an \"AS IS\" BASIS,       *\n"
" * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*\n"
" * See the License for the specific language governing permissions and     *\n"
" * limitations under the License.                                          *\n"
" ***************************************************************************/\n"
"\n"
"float3 Matrix4x4_ApplyPoint(__global Matrix4x4 *m, const float3 point) {\n"
"	const float4 point4 = (float4)(point.x, point.y, point.z, 1.f);\n"
"\n"
"	const float4 row3 = VLOAD4F(&m->m[3][0]);\n"
"	const float iw = 1.f / dot(row3, point4);\n"
"\n"
"	const float4 row0 = VLOAD4F(&m->m[0][0]);\n"
"	const float4 row1 = VLOAD4F(&m->m[1][0]);\n"
"	const float4 row2 = VLOAD4F(&m->m[2][0]);\n"
"	return (float3)(\n"
"			iw * dot(row0, point4),\n"
"			iw * dot(row1, point4),\n"
"			iw * dot(row2, point4)\n"
"			);\n"
"}\n"
"\n"
"float3 Matrix4x4_ApplyPoint_Private(Matrix4x4 *m, const float3 point) {\n"
"	const float4 point4 = (float4)(point.x, point.y, point.z, 1.f);\n"
"\n"
"	const float4 row3 = VLOAD4F_Private(&m->m[3][0]);\n"
"	const float iw = 1.f / dot(row3, point4);\n"
"\n"
"	const float4 row0 = VLOAD4F_Private(&m->m[0][0]);\n"
"	const float4 row1 = VLOAD4F_Private(&m->m[1][0]);\n"
"	const float4 row2 = VLOAD4F_Private(&m->m[2][0]);\n"
"	return (float3)(\n"
"			iw * dot(row0, point4),\n"
"			iw * dot(row1, point4),\n"
"			iw * dot(row2, point4)\n"
"			);\n"
"}\n"
"\n"
"float3 Matrix4x4_ApplyVector(__global Matrix4x4 *m, const float3 vector) {\n"
"	const float3 row0 = VLOAD3F(&m->m[0][0]);\n"
"	const float3 row1 = VLOAD3F(&m->m[1][0]);\n"
"	const float3 row2 = VLOAD3F(&m->m[2][0]);\n"
"	return (float3)(\n"
"			dot(row0, vector),\n"
"			dot(row1, vector),\n"
"			dot(row2, vector)\n"
"			);\n"
"}\n"
"\n"
"float3 Matrix4x4_ApplyVector_Private(Matrix4x4 *m, const float3 vector) {\n"
"	const float3 row0 = VLOAD3F_Private(&m->m[0][0]);\n"
"	const float3 row1 = VLOAD3F_Private(&m->m[1][0]);\n"
"	const float3 row2 = VLOAD3F_Private(&m->m[2][0]);\n"
"	return (float3)(\n"
"			dot(row0, vector),\n"
"			dot(row1, vector),\n"
"			dot(row2, vector)\n"
"			);\n"
"}\n"
"\n"
"float3 Matrix4x4_ApplyNormal(__global Matrix4x4 *m, const float3 normal) {\n"
"	const float3 row0 = (float3)(m->m[0][0], m->m[1][0], m->m[2][0]);\n"
"	const float3 row1 = (float3)(m->m[0][1], m->m[1][1], m->m[2][1]);\n"
"	const float3 row2 = (float3)(m->m[0][2], m->m[1][2], m->m[2][2]);\n"
"	return (float3)(\n"
"			dot(row0, normal),\n"
"			dot(row1, normal),\n"
"			dot(row2, normal)\n"
"			);\n"
"}\n"
"\n"
"void Matrix4x4_Identity(Matrix4x4 *m) {\n"
"	for (int j = 0; j < 4; ++j)\n"
"		for (int i = 0; i < 4; ++i)\n"
"			m->m[i][j] = (i == j) ? 1.f : 0.f;\n"
"}\n"
"\n"
"void Matrix4x4_Invert(Matrix4x4 *m) {\n"
"	int indxc[4], indxr[4];\n"
"	int ipiv[4] = {0, 0, 0, 0};\n"
"\n"
"	for (int i = 0; i < 4; ++i) {\n"
"		int irow = -1, icol = -1;\n"
"		float big = 0.;\n"
"		// Choose pivot\n"
"		for (int j = 0; j < 4; ++j) {\n"
"			if (ipiv[j] != 1) {\n"
"				for (int k = 0; k < 4; ++k) {\n"
"					if (ipiv[k] == 0) {\n"
"						if (fabs(m->m[j][k]) >= big) {\n"
"							big = fabs(m->m[j][k]);\n"
"							irow = j;\n"
"							icol = k;\n"
"						}\n"
"					} else if (ipiv[k] > 1) {\n"
"						//throw std::runtime_error(\"Singular matrix in MatrixInvert: \" + ToString(*this));\n"
"						// I can not do very much here\n"
"						Matrix4x4_Identity(m);\n"
"						return;\n"
"					}\n"
"				}\n"
"			}\n"
"		}\n"
"		++ipiv[icol];\n"
"		// Swap rows _irow_ and _icol_ for pivot\n"
"		if (irow != icol) {\n"
"			for (int k = 0; k < 4; ++k) {\n"
"				const float tmp = m->m[irow][k];\n"
"				m->m[irow][k] = m->m[icol][k];\n"
"				m->m[icol][k] = tmp;\n"
"			}\n"
"		}\n"
"		indxr[i] = irow;\n"
"		indxc[i] = icol;\n"
"		if (m->m[icol][icol] == 0.f) {\n"
"			//throw std::runtime_error(\"Singular matrix in MatrixInvert: \" + ToString(*this));\n"
"			// I can not do very much here\n"
"			Matrix4x4_Identity(m);\n"
"			return;\n"
"		}\n"
"		// Set $m[icol][icol]$ to one by scaling row _icol_ appropriately\n"
"		float pivinv = 1.f / m->m[icol][icol];\n"
"		m->m[icol][icol] = 1.f;\n"
"		for (int j = 0; j < 4; ++j)\n"
"			m->m[icol][j] *= pivinv;\n"
"		// Subtract this row from others to zero out their columns\n"
"		for (int j = 0; j < 4; ++j) {\n"
"			if (j != icol) {\n"
"				float save = m->m[j][icol];\n"
"				m->m[j][icol] = 0;\n"
"				for (int k = 0; k < 4; ++k)\n"
"					m->m[j][k] -= m->m[icol][k] * save;\n"
"			}\n"
"		}\n"
"	}\n"
"	// Swap columns to reflect permutation\n"
"	for (int j = 3; j >= 0; --j) {\n"
"		if (indxr[j] != indxc[j]) {\n"
"			for (int k = 0; k < 4; ++k) {\n"
"				const float tmp = m->m[k][indxr[j]];\n"
"				m->m[k][indxr[j]] = m->m[k][indxc[j]];\n"
"				m->m[k][indxc[j]] = tmp;\n"
"			}\n"
"		}\n"
"	}\n"
"}\n"
; } }
