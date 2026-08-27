#include "PathTools.h"

#include <algorithm>
#include <cmath>

namespace rigkit {
namespace artboards {

float segmentHitT(const ecs::CPath& path, int commandIndex, const glm::vec2& mm) {
	if (commandIndex <= 0 || commandIndex >= static_cast<int>(path.commands.size())) {
		return 0.5f;
	}
	const auto& c = path.commands[static_cast<size_t>(commandIndex)];
	glm::vec2 p0{};
	for (int i = commandIndex - 1; i >= 0; --i) {
		const auto& prev = path.commands[static_cast<size_t>(i)];
		if (prev.type != ecs::CPath::Cmd::Close) {
			p0 = prev.p;
			break;
		}
	}
	if (c.type == ecs::CPath::Cmd::LineTo) {
		const glm::vec2 ab = c.p - p0;
		const float len2 = glm::dot(ab, ab);
		if (len2 < 1e-12f) {
			return 0.5f;
		}
		return std::clamp(glm::dot(mm - p0, ab) / len2, 0.05f, 0.95f);
	}
	auto at = [&](float t) {
		if (c.type == ecs::CPath::Cmd::CubicTo) {
			const float u = 1.f - t;
			return u * u * u * p0 + 3.f * u * u * t * c.c1 + 3.f * u * t * t * c.c2 +
				   t * t * t * c.p;
		}
		if (c.type == ecs::CPath::Cmd::QuadTo) {
			const float u = 1.f - t;
			return u * u * p0 + 2.f * u * t * c.c1 + t * t * c.p;
		}
		return glm::mix(p0, c.p, t);
	};
	float bestT = 0.5f;
	float bestD2 = 1e12f;
	for (int i = 1; i < 16; ++i) {
		const float t = static_cast<float>(i) / 16.f;
		const glm::vec2 q = at(t);
		const float d2 = glm::dot(q - mm, q - mm);
		if (d2 < bestD2) {
			bestD2 = d2;
			bestT = t;
		}
	}
	return std::clamp(bestT, 0.05f, 0.95f);
}

bool pickHitAcrossLayers(MEcs& ecs, const std::vector<entt::entity>& layers, const glm::vec2& mm,
						 float tolMm, int& outLayer, int& outPath, int& outCmd,
						 ecs::PathEditPart& outPart, bool includeTravels) {
	outLayer = -1;
	outPath = -1;
	outCmd = -1;
	outPart = ecs::PathEditPart::None;
	for (int li = static_cast<int>(layers.size()) - 1; li >= 0; --li) {
		const entt::entity e = layers[static_cast<size_t>(li)];
		if (!ecs.hasComponent<ecs::CPaths>(e)) {
			continue;
		}
		if (ecs.hasComponent<ecs::CPlotLayer>(e)) {
			const auto& layer = ecs.getComponent<ecs::CPlotLayer>(e);
			if (!layer.visible || layer.locked) {
				continue;
			}
		}
		const auto& bag = ecs.getComponent<ecs::CPaths>(e);
		int pi = -1;
		int ci = -1;
		ecs::PathEditPart part = ecs::PathEditPart::None;
		if (pathEdit::pickHit(bag, mm, tolMm, pi, ci, part, includeTravels)) {
			outLayer = li;
			outPath = pi;
			outCmd = ci;
			outPart = part;
			return true;
		}
	}
	return false;
}

bool boundsIntersectRect(const plot::PathBounds& b, float x0, float y0, float x1, float y1) {
	if (b.empty) {
		return false;
	}
	const float minX = std::min(x0, x1);
	const float maxX = std::max(x0, x1);
	const float minY = std::min(y0, y1);
	const float maxY = std::max(y0, y1);
	return !(b.maxX < minX || b.minX > maxX || b.maxY < minY || b.minY > maxY);
}

std::vector<ecs::PathSelRef> marqueeHitsBag(const ecs::CPaths& bag, int layerIndex,
											const glm::vec2& a, const glm::vec2& b,
											float flattenTolMm, bool includeTravels) {
	std::vector<ecs::PathSelRef> hits;
	for (int pi = 0; pi < static_cast<int>(bag.paths.size()); ++pi) {
		if (!includeTravels && bag.isTravel(static_cast<size_t>(pi))) {
			continue;
		}
		const auto pb = plot::boundsOfPath(bag.paths[static_cast<size_t>(pi)], flattenTolMm);
		if (boundsIntersectRect(pb, a.x, a.y, b.x, b.y)) {
			hits.push_back({layerIndex, pi});
		}
	}
	return hits;
}

std::vector<ecs::PathSelRef> marqueeHits(MEcs& ecs, const std::vector<entt::entity>& layers,
										 const glm::vec2& a, const glm::vec2& b,
										 float flattenTolMm, bool includeTravels) {
	std::vector<ecs::PathSelRef> hits;
	for (int li = 0; li < static_cast<int>(layers.size()); ++li) {
		const entt::entity e = layers[static_cast<size_t>(li)];
		if (!ecs.hasComponent<ecs::CPaths>(e)) {
			continue;
		}
		if (ecs.hasComponent<ecs::CPlotLayer>(e)) {
			const auto& layer = ecs.getComponent<ecs::CPlotLayer>(e);
			if (!layer.visible || layer.locked) {
				continue;
			}
		}
		const auto& bag = ecs.getComponent<ecs::CPaths>(e);
		const auto layerHits = marqueeHitsBag(bag, li, a, b, flattenTolMm, includeTravels);
		hits.insert(hits.end(), layerHits.begin(), layerHits.end());
	}
	return hits;
}

void applyMarqueeHits(ecs::CPathEditSelection& sel, const std::vector<ecs::PathSelRef>& hits,
					  bool shiftAdd) {
	if (shiftAdd) {
		for (const auto& h : hits) {
			if (!sel.isPathSelected(h.layerIndex, h.pathIndex)) {
				sel.togglePath(h.layerIndex, h.pathIndex);
			}
		}
		return;
	}
	if (hits.empty()) {
		sel.clearPaths();
		return;
	}
	sel.selectOnly(hits.front().layerIndex, hits.front().pathIndex);
	for (size_t i = 1; i < hits.size(); ++i) {
		sel.togglePath(hits[i].layerIndex, hits[i].pathIndex);
	}
}

PenClickAction penClickAction(const ecs::CPaths& bag, int pathIndex, const glm::vec2& mouseMm,
							  float closeTolMm) {
	glm::vec2 startPt{};
	const bool haveStart = pathIndex >= 0 && pathEdit::pathStartPoint(bag, pathIndex, startPt);
	const bool nearStart =
		haveStart && glm::length(mouseMm - startPt) <= closeTolMm && pathIndex >= 0 &&
		pathIndex < static_cast<int>(bag.paths.size()) &&
		bag.paths[static_cast<size_t>(pathIndex)].commands.size() >= 2;
	if (nearStart) {
		return PenClickAction::Close;
	}
	if (pathIndex < 0 || pathIndex >= static_cast<int>(bag.paths.size()) ||
		(!bag.paths[static_cast<size_t>(pathIndex)].commands.empty() &&
		 bag.paths[static_cast<size_t>(pathIndex)].commands.back().type ==
			 ecs::CPath::Cmd::Close)) {
		return PenClickAction::BeginNew;
	}
	return PenClickAction::StartPreview;
}

bool pathEndPoint(const ecs::CPaths& bag, int pathIndex, glm::vec2& out) {
	if (pathIndex < 0 || pathIndex >= static_cast<int>(bag.paths.size())) {
		return false;
	}
	const auto& cmds = bag.paths[static_cast<size_t>(pathIndex)].commands;
	for (int i = static_cast<int>(cmds.size()) - 1; i >= 0; --i) {
		const auto& c = cmds[static_cast<size_t>(i)];
		if (c.type != ecs::CPath::Cmd::Close) {
			out = c.p;
			return true;
		}
	}
	return false;
}

bool penCommitPreview(ecs::CPaths& bag, int pathIndex, const glm::vec2& down,
					  const glm::vec2& mouseMm, float clickTolMm, bool& ioHasOutgoing,
					  glm::vec2& ioOutgoing, PenCommitKind& outKind) {
	if (pathIndex < 0 || pathIndex >= static_cast<int>(bag.paths.size())) {
		return false;
	}
	auto& cmds = bag.paths[static_cast<size_t>(pathIndex)].commands;
	if (cmds.empty() || cmds.back().type == ecs::CPath::Cmd::Close) {
		return false;
	}

	const glm::vec2 drag = mouseMm - down;
	const bool isClick = glm::length(drag) < clickTolMm;

	// First point: drag only seeds the outgoing handle for the next segment.
	const bool onlyMove =
		cmds.size() == 1 && cmds.front().type == ecs::CPath::Cmd::MoveTo;
	if (onlyMove) {
		if (isClick) {
			ioHasOutgoing = false;
		} else {
			ioHasOutgoing = true;
			ioOutgoing = mouseMm; // outgoing = drag end
		}
		outKind = PenCommitKind::FirstHandle;
		return true;
	}

	glm::vec2 p0{};
	if (!pathEndPoint(bag, pathIndex, p0)) {
		return false;
	}

	const glm::vec2 c1 =
		ioHasOutgoing ? ioOutgoing : (p0 + (down - p0) * (1.f / 3.f));

	if (isClick) {
		if (!pathEdit::appendLineTo(bag, pathIndex, down)) {
			return false;
		}
		ioHasOutgoing = false;
		outKind = PenCommitKind::Line;
		return true;
	}

	// SVG cubic: c1 outgoing from p0, c2 incoming at end (mirror of drag).
	const glm::vec2 c2 = down - drag;
	if (!pathEdit::appendCubicTo(bag, pathIndex, c1, c2, down)) {
		return false;
	}
	ioHasOutgoing = true;
	ioOutgoing = down + drag;
	outKind = PenCommitKind::Cubic;
	return true;
}

} // namespace artboards
} // namespace rigkit
