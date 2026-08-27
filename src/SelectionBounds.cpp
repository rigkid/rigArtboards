#include "SelectionBounds.h"

#include <algorithm>

namespace rigkit {
namespace artboards {

SelBounds selectionBoundsOfBag(const ecs::CPaths& bag, const ecs::CPathEditSelection& sel,
							   int layerIndex) {
	SelBounds out;
	bool have = false;
	for (const auto& ref : sel.selectedRefs()) {
		if (ref.layerIndex != layerIndex || ref.pathIndex < 0 ||
			ref.pathIndex >= static_cast<int>(bag.paths.size())) {
			continue;
		}
		const auto pb = plot::boundsOfPath(bag.paths[static_cast<size_t>(ref.pathIndex)]);
		if (pb.empty) {
			continue;
		}
		if (!have) {
			out.minX = pb.minX;
			out.minY = pb.minY;
			out.maxX = pb.maxX;
			out.maxY = pb.maxY;
			have = true;
		} else {
			out.minX = std::min(out.minX, pb.minX);
			out.minY = std::min(out.minY, pb.minY);
			out.maxX = std::max(out.maxX, pb.maxX);
			out.maxY = std::max(out.maxY, pb.maxY);
		}
	}
	out.valid = have;
	return out;
}

SelBounds selectionBounds(MEcs& ecs, const std::vector<entt::entity>& layers,
						  const ecs::CPathEditSelection& sel) {
	SelBounds out;
	bool have = false;
	for (int li = 0; li < static_cast<int>(layers.size()); ++li) {
		const entt::entity le = layers[static_cast<size_t>(li)];
		if (!ecs.hasComponent<ecs::CPaths>(le)) {
			continue;
		}
		const auto& bag = ecs.getComponent<ecs::CPaths>(le);
		const auto one = selectionBoundsOfBag(bag, sel, li);
		if (!one.valid) {
			continue;
		}
		if (!have) {
			out = one;
			have = true;
		} else {
			out.minX = std::min(out.minX, one.minX);
			out.minY = std::min(out.minY, one.minY);
			out.maxX = std::max(out.maxX, one.maxX);
			out.maxY = std::max(out.maxY, one.maxY);
		}
	}
	out.valid = have;
	return out;
}

} // namespace artboards
} // namespace rigkit
