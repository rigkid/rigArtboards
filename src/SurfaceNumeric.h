#pragma once

#include <functional>
#include <glm/glm.hpp>
#include "SelectionBounds.h"
#include "SurfaceSeam.h"

namespace rigkit {
namespace artboards {

/**
 * @brief ImGui X / Y / W / H / rotate fields for the current selection.
 * @details Edits are absolute against the current union bounds; the shell
 * supplies restore+apply the same way as the gizmo.
 */
struct SurfaceNumeric {
	float rotateDeg = 0.f;

	/**
	 * @brief Draw fields; returns true when geometry changed this frame.
	 * @param restoreSnap restore selection geometry before applying a new size/pos.
	 */
	bool drawFields(SelBounds& bounds, const SurfaceApply& apply,
					const std::function<void()>& restoreSnap);
};

} // namespace artboards
} // namespace rigkit
