#pragma once

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	//used in a couple places for GFXObjects

	struct V2Vector3 {
		fixed_point_t x;
		fixed_point_t y;
		fixed_point_t z;

		//V2Vector3(fixed_point_t new_x = 0, fixed_point_t new_y = 0, fixed_point_t new_z = 0);
		V2Vector3(fixed_point_t new_x = 0, fixed_point_t new_y = 0, fixed_point_t new_z = 0) :
		x { new_x }, y { new_y }, z { new_z } {}

		//V2Vector3(V2Vector3&&) = default; //move constructor
		virtual ~V2Vector3() = default;
	};
	//used for lightTypes
	struct V2Vector4 {
		fixed_point_t x;
		fixed_point_t y;
		fixed_point_t z;
		fixed_point_t w;

		V2Vector4(fixed_point_t new_x = 0, fixed_point_t new_y = 0, fixed_point_t new_z = 0, fixed_point_t new_w = 0);

		//V2Vector4(V2Vector4&&) = default; //move constructor
		virtual ~V2Vector4() = default;
	};
}

//TODO: Should we bother with more functions (to bring this in line with Vector.hpp)
//or are we just immediately converting this stuff to godot::Vector3, Vector4 anyways?

