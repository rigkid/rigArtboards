#pragma once

/**
 * @brief Shared path pick / marquee / pen helpers for the editing surface.
 * @details Geometry and hit logic live here; shells wrap undo and paint.
 * Prefer svgEditor's visibility-aware cross-layer pick, and PaintPlotter's
 * curve-aware `segmentHitT`.
 */

#include <cstdint>
#include <glm/glm.hpp>
#include <vector>
#include "CPath.h"
#include "CPathEditSelection.h"
#include "CPaths.h"
#include "CPlotLayer.h"
#include "PathBounds.h"
#include "PathEditOps.h"
#include "ecs/MEcs.h"

namespace rigkit {
namespace artboards {

/** @brief Parameter t along a segment for Pen-Add insert (0.05–0.95). */
float segmentHitT(const ecs::CPath& path, int commandIndex, const glm::vec2& mm);

/**
 * @brief Top-most path hit across layers (high index first).
 * @details Skips invisible / locked layers when `CPlotLayer` is present.
 */
bool pickHitAcrossLayers(MEcs& ecs, const std::vector<entt::entity>& layers, const glm::vec2& mm,
						 float tolMm, int& outLayer, int& outPath, int& outCmd,
						 ecs::PathEditPart& outPart, bool includeTravels = false);

/** @brief Axis-aligned rect vs path bounds. */
bool boundsIntersectRect(const plot::PathBounds& b, float x0, float y0, float x1, float y1);

/**
 * @brief Paths whose bounds intersect the marquee (visible unlocked layers).
 * @param includeTravels when false, travel paths are skipped.
 */
std::vector<ecs::PathSelRef> marqueeHits(MEcs& ecs, const std::vector<entt::entity>& layers,
										 const glm::vec2& a, const glm::vec2& b,
										 float flattenTolMm = 0.5f, bool includeTravels = false);

/** @brief Apply marquee hits to selection (Shift = add). */
void applyMarqueeHits(ecs::CPathEditSelection& sel, const std::vector<ecs::PathSelRef>& hits,
					  bool shiftAdd);

enum class PenClickAction : uint8_t { None = 0, Close, BeginNew, StartPreview };

/**
 * @brief Decide pen-tool click action from bag + selection + mouse.
 * @param closeTolMm distance to path start that closes the path.
 */
PenClickAction penClickAction(const ecs::CPaths& bag, int pathIndex, const glm::vec2& mouseMm,
							  float closeTolMm);

enum class PenCommitKind : uint8_t { Line = 0, Cubic, FirstHandle };

/**
 * @brief Commit pen rubber-band: click = line, drag = cubic (SVG c1/c2 order).
 * @details `c1` is the outgoing handle of the previous point (@p ioOutgoing when set);
 * `c2` is the incoming handle at @p down (mirror of the drag). Drag also writes the
 * next outgoing into @p ioOutgoing. When the path is only a MoveTo, drag sets the
 * first outgoing handle and appends nothing (`FirstHandle`).
 * @return false if append failed; @p outKind set on success.
 */
bool penCommitPreview(ecs::CPaths& bag, int pathIndex, const glm::vec2& down,
					  const glm::vec2& mouseMm, float clickTolMm, bool& ioHasOutgoing,
					  glm::vec2& ioOutgoing, PenCommitKind& outKind);

/** @brief Last non-Close endpoint of an open path. */
bool pathEndPoint(const ecs::CPaths& bag, int pathIndex, glm::vec2& out);

} // namespace artboards
} // namespace rigkit
