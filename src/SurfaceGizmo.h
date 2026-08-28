#pragma once

#include <functional>
#include <glm/glm.hpp>
#include <imgui.h>

#include "SelectionBounds.h"
#include "SurfaceSeam.h"
#include "SurfaceXform.h"

namespace rigkit {
namespace artboards {

/**
 * @brief 8-handle + rotate-stem selection gizmo (svgEditor-grade).
 * @details Shift = proportional on corners; Alt = scale about centre;
 * Shift while rotating snaps to 15°. Shell applies path ops via `SurfaceApply`.
 */
struct SurfaceGizmo {
	float handleRadiusPx = 4.f;
	float scaleHitPadPx = 2.f;
	float rotateStemMm = 18.f;
	float rotateCornerOuterPx = 30.f;

	SurfaceXform xform;

	/** @brief Draw bbox + handles + rotate stem. Updates hover cursor. */
	void draw(ImDrawList* dl, const SurfaceMap& map, const SelBounds& bounds, bool active) const;

	/**
	 * @brief Begin scale/rotate/move from a click. Returns true if a handle/body hit.
	 * @param ctrlRotate when true, corner hits start Rotate instead of Scale (PaintPlotter).
	 */
	bool beginClick(const SurfaceMap& map, const SelBounds& bounds, const ImVec2& mousePx,
					const glm::vec2& mouseMm, bool shift, bool alt, bool ctrlRotate);

	/** @brief Begin translating the selection (click inside bbox, not on handle). */
	void beginMove(const SelBounds& bounds, const glm::vec2& mouseMm);

	/**
	 * @brief Live drag update. Restores from @p restoreSnap then applies absolute xform.
	 * @param restoreSnap re-applies the gesture-start geometry, then the new transform.
	 */
	bool drag(const SurfaceMap& map, const glm::vec2& mouseMm, bool shift, bool alt,
			  const SurfaceApply& apply,
			  const std::function<void()>& restoreSnap);

	void end();

	int hitScaleHandle(const SurfaceMap& map, const SelBounds& bounds, const ImVec2& mousePx) const;
	bool hitRotate(const SurfaceMap& map, const SelBounds& bounds, const ImVec2& mousePx) const;

	/** @brief Live box during scale/move (for drawing). */
	void liveBox(float& r, float& t, float& w, float& h) const;
};

/** @brief Set ImGui cursor for a scale handle index (0-7). */
void setScaleCursor(int handle);

} // namespace artboards
} // namespace rigkit
