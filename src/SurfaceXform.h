#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace rigkit {
namespace artboards {

/** @brief Active gesture on the editing surface. */
enum class SurfaceDrag : uint8_t {
	None = 0,
	Marquee,
	Move,
	Scale,
	Rotate
};

/**
 * @brief Transient transform-gizmo state (not an ECS component).
 * @details Handles: 0-3 corners (TL TR BR BL), 4-7 mid-edges (T R B L).
 */
struct SurfaceXform {
	SurfaceDrag kind = SurfaceDrag::None;
	int handle = -1;
	glm::vec2 startMm{0.f};
	float x0 = 0.f;
	float y0 = 0.f;
	float w0 = 0.f;
	float h0 = 0.f;
	float rot0 = 0.f;
	float liveAngle = 0.f;
	float liveX = 0.f;
	float liveY = 0.f;
	float liveW = 0.f;
	float liveH = 0.f;
	bool proportional = false; ///< Shift
	bool aboutCentre = false;  ///< Alt
};

} // namespace artboards
} // namespace rigkit
