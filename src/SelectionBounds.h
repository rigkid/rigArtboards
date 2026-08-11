#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "CPathEditSelection.h"
#include "CPaths.h"
#include "PathBounds.h"
#include "ecs/MEcs.h"

namespace rigkit {
namespace artboards {

struct SelBounds {
	float minX = 0.f;
	float minY = 0.f;
	float maxX = 0.f;
	float maxY = 0.f;
	bool valid = false;

	glm::vec2 min() const { return {minX, minY}; }
	glm::vec2 max() const { return {maxX, maxY}; }
	float width() const { return maxX - minX; }
	float height() const { return maxY - minY; }
};

/**
 * @brief Union bounds of selected paths across layers.
 * @param layers Entities in `PlotDoc::layerEntities()` order.
 */
SelBounds selectionBounds(MEcs& ecs, const std::vector<entt::entity>& layers,
						  const ecs::CPathEditSelection& sel);

} // namespace artboards
} // namespace rigkit
