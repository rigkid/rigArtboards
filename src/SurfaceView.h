#pragma once

#include <glm/glm.hpp>
#include <imgui.h>
#include <vector>

#include "CArtboard.h"
#include "CBedView.h"

namespace rigkit {
namespace artboards {

struct FitRectMm {
	float minX = 0.f;
	float minY = 0.f;
	float maxX = 210.f;
	float maxY = 297.f;
};

/**
 * @brief Pan / zoom-under-cursor / fit over `CBedView`.
 * @details Call `handleWheel` before any code that writes scale, so `pxToMm`
 * still reads the old scale (fixes zoom-to-origin).
 */
struct SurfaceView {
	float minScale = 0.05f;
	float maxScale = 40.f;
	float wheelFactor = 1.1f;

	/** @brief Zoom keeping @p localPx (canvas-local) fixed in mm. */
	void handleWheel(ecs::CBedView& view, const glm::vec2& localPx, float wheel) const;

	/** @brief Pan by screen-pixel delta. */
	void handlePan(ecs::CBedView& view, const glm::vec2& deltaPx) const;

	/** @brief Fit @p world mm rect into @p canvasSize with margin. */
	void fit(ecs::CBedView& view, const ImVec2& canvasSize, const FitRectMm& world,
			 float marginPx = 24.f) const;

	/** @brief Draw filled/stroked artboard rects (page + fixtures). */
	void drawArtboards(ImDrawList* dl, const ImVec2& canvasPos, const ecs::CBedView& view,
					   const std::vector<const ecs::CArtboard*>& boards,
					   bool fillPage = true) const;

	/**
	 * @brief Background-pass grid in document mm (axis-aligned rect).
	 * @details Caller converts machine-space zones via machineToDocument first.
	 */
	void drawGrid(ImDrawList* dl, const ImVec2& canvasPos, const ecs::CBedView& view,
				  const FitRectMm& rectMm, float stepMm, ImU32 col) const;
};

} // namespace artboards
} // namespace rigkit
