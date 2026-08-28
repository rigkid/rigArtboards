#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <imgui.h>

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

/** @brief Draw Bezier anchors + handles (svgEditor-grade outlined dark knobs). */
void drawPathAnchors(ImDrawList* dl, const SurfaceMap& map, const ecs::CPath& path,
					 int selectedCommand);

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
void drawMarquee(ImDrawList* dl, const SurfaceMap& map, const glm::vec2& aMm,
				 const glm::vec2& bMm);

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
