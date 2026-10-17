#pragma once

#include <glm/glm.hpp>
#include <imgui.h>
#include <entt/entt.hpp>

#include "ecs/MEcs.h"
#include "CPath.h"
#include "SurfaceSeam.h"

namespace rigkit {
namespace artboards {

/**
 * @brief Chrome-pass drawing - guides, anchors, pen preview, marquee.
 * @details Call after the shell's content pass. Background grid lives on
 * `SurfaceView::drawGrid`.
 */

/** @brief Bezier knob / path-node chrome. Defaults match svgEditor knobs. */
struct PathChromeStyle {
	bool showHandles = true;
	bool handlesOnSelectedOnly = false;
	/** @brief Handle knob radius in 1x design px. Draw / hit via uiPx(). */
	float handlePx = 3.5f;
	/** @brief Path-node radius in 1x design px. Draw / hit via uiPx(). */
	float nodePx = 3.5f;
	/// 0 = Circle, 1 = Square
	int handleShape = 0;
	/// 0 = Auto (smooth circle, corner square), 1 = Circles, 2 = Squares
	int nodeTypeDisplay = 0;
	ImU32 handleCol = IM_COL32(100, 160, 255, 180);
	ImU32 handleFill = IM_COL32(100, 160, 255, 230);
	ImU32 nodeFill = IM_COL32(32, 32, 40, 255);
	ImU32 nodeOutline = IM_COL32(255, 255, 255, 230);
	ImU32 nodeSelected = IM_COL32(255, 140, 60, 255);
};

/**
 * @brief Draw Bezier anchors + handles.
 * @param style Omit for svgEditor knobs (dark fill, white ring, blue stems).
 */
void drawPathAnchors(ImDrawList* dl, const SurfaceMap& map, const ecs::CPath& path,
					 int selectedCommand, const PathChromeStyle& style = {});

/**
 * @brief Screen-constant pick slop in mm for nodes / handles.
 * @details `uiPx(nodePx + pad)` over the view scale so HiDPI and zoom match the draw.
 */
float pathChromeHitMm(const SurfaceMap& map, const PathChromeStyle& style = {});

/**
 * @brief Pen drag preview: cubic (or line) from previous point + handle stems at down.
 * @param fromMm previous path endpoint; null draws first-point handle stems only.
 * @param hasOutgoing when true, @p outgoingMm is `c1` of the preview cubic.
 */
void drawPenPreview(ImDrawList* dl, const SurfaceMap& map, const glm::vec2* fromMm,
					bool hasOutgoing, const glm::vec2& outgoingMm, const glm::vec2& downMm,
					const glm::vec2& mouseMm);

/**
 * @brief Idle pen chrome: last-point handles stay, plus a rubber-band to the cursor.
 * @details Preview is only live while the mouse is down. After release the outgoing
 * handle still lives on the selection - draw it here so the point does not vanish.
 */
void drawPenRubberBand(ImDrawList* dl, const SurfaceMap& map, const glm::vec2& fromMm,
					   bool hasOutgoing, const glm::vec2& outgoingMm, const glm::vec2& mouseMm);

/** @brief Semi-transparent marquee rectangle in mm space. */
void drawMarquee(ImDrawList* dl, const SurfaceMap& map, const glm::vec2& aMm, const glm::vec2& bMm);

/**
 * @brief Draw CGuide lines for @p docId (or all if docId == 0 and no filter needed).
 * @param selected highlighted guide entity, or entt::null.
 * @param dragVertical / dragPosMm optional in-progress ruler drag preview.
 */
void drawGuides(ImDrawList* dl, const SurfaceMap& map, MEcs& ecs, uint32_t docId,
				entt::entity selected, float canvasW, float canvasH, bool show,
				bool dragActive = false, bool dragVertical = true, float dragPosMm = 0.f);

/**
 * @brief Nearest guide within @p tolMm; respects CPlotDocRef when present.
 * @return entt::null when none hit.
 */
entt::entity pickGuide(MEcs& ecs, uint32_t docId, const glm::vec2& mouseMm, float tolMm);

} // namespace artboards
} // namespace rigkit
