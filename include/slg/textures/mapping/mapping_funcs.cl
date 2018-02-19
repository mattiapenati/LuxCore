#line 2 "mapping_funcs.cl"

/***************************************************************************
 * Copyright 1998-2018 by authors (see AUTHORS.txt)                        *
 *                                                                         *
 *   This file is part of LuxCoreRender.                                   *
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

//------------------------------------------------------------------------------
// 2D mapping
//------------------------------------------------------------------------------

float2 UVMapping2D_Map(__global const TextureMapping2D *mapping, __global HitPoint *hitPoint) {
	// Scale
	const float uScaled = hitPoint->uv.u * mapping->uvMapping2D.uScale;
	const float vScaled = hitPoint->uv.v * mapping->uvMapping2D.vScale;

	// Rotate
	const float sinTheta = mapping->uvMapping2D.sinTheta;
	const float cosTheta = mapping->uvMapping2D.cosTheta;
	const float uRotated = uScaled * cosTheta - vScaled * sinTheta;
	const float vRotated = vScaled * cosTheta + uScaled * sinTheta;

	// Translate
	const float uTranslated = uRotated + mapping->uvMapping2D.uDelta;
	const float vTranslated = vRotated + mapping->uvMapping2D.vDelta;

	return (float2)(uTranslated, vTranslated);
}

float2 UVMapping2D_MapDuv(__global const TextureMapping2D *mapping, __global HitPoint *hitPoint, float2 *ds, float2 *dt) {
	(*ds).xy = VLOAD2F(&mapping->uvMapping2D.uScale);
	(*dt).xy = (float2)(0.f, (*ds).y);
	(*ds).y = 0.f;
	return UVMapping2D_Map(mapping, hitPoint);
}

float2 TextureMapping2D_Map(__global const TextureMapping2D *mapping, __global HitPoint *hitPoint) {
	switch (mapping->type) {
		case UVMAPPING2D:
			return UVMapping2D_Map(mapping, hitPoint);
		default:
			return 0.f;
	}
}

float2 TextureMapping2D_MapDuv(__global const TextureMapping2D *mapping, __global HitPoint *hitPoint, float2 *ds, float2 *dt) {
	switch (mapping->type) {
		case UVMAPPING2D:
			return UVMapping2D_MapDuv(mapping, hitPoint, ds, dt);
		default:
			return 0.f;
	}
}

//------------------------------------------------------------------------------
// 3D mapping
//------------------------------------------------------------------------------

float3 UVMapping3D_Map(__global const TextureMapping3D *mapping, __global HitPoint *hitPoint) {
	const float2 uv = VLOAD2F(&hitPoint->uv.u);
	return Transform_ApplyPoint(&mapping->worldToLocal, (float3)(uv.xy, 0.f));
}

float3 GlobalMapping3D_Map(__global const TextureMapping3D *mapping, __global HitPoint *hitPoint) {
	const float3 p = VLOAD3F(&hitPoint->p.x);
	return Transform_ApplyPoint(&mapping->worldToLocal, p);
}

float3 LocalMapping3D_Map(__global const TextureMapping3D *mapping, __global HitPoint *hitPoint) {
	const Matrix4x4 m = Matrix4x4_Mul(&mapping->worldToLocal.m, &hitPoint->worldToLocal);
	const float3 p = VLOAD3F(&hitPoint->p.x);

	return Matrix4x4_ApplyPoint_Private(&m, p);
}

float3 TextureMapping3D_Map(__global const TextureMapping3D *mapping, __global HitPoint *hitPoint) {
	switch (mapping->type) {
		case UVMAPPING3D:
			return UVMapping3D_Map(mapping, hitPoint);
		case GLOBALMAPPING3D:
			return GlobalMapping3D_Map(mapping, hitPoint);
		case LOCALMAPPING3D:
			return LocalMapping3D_Map(mapping, hitPoint);
		default:
			return 0.f;
	}
}
