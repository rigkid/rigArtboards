#pragma once

#include <entt/entt.hpp>
#include <functional>
#include <glm/glm.hpp>
#include <imgui.h>
#include <vector>
#include "CBedView.h"
#include "CPaths.h"

namespace rigkit {
namespace artboards {

/** @brief Hit priority for shell-registered pickables (higher wins earlier). */
enum class PickPriority : int {
	Empty = 0,
	Artboard = 10,
	Path = 20,
	Shell = 30,
	Handle = 40
};

struct SurfacePickable {
	PickPriority priority = PickPriority::Shell;
	entt::entity entity = entt::null;
	glm::vec2 minMm{0.f};
	glm::vec2 maxMm{0.f};
	const char* label = "";
};

struct SurfaceMap {
	ImVec2 canvasPos{0.f, 0.f};
	ecs::CBedView* view = nullptr;

	ImVec2 toScreen(const glm::vec2& mm) const {
		if (!view) {
			return canvasPos;
		}
		const glm::vec2 px = view->mmToPx(mm);
		return {canvasPos.x + px.x, canvasPos.y + px.y};
	}

	glm::vec2 toMm(const ImVec2& screen) const {
		if (!view) {
			return {};
		}
		return view->pxToMm({screen.x - canvasPos.x, screen.y - canvasPos.y});
	}
};

/** @brief Shell applies path transforms; owns undo. */
using ApplyPathFn = std::function<bool(ecs::CPaths& bag, int pathIndex)>;

struct SurfaceApply {
	/** @brief Called once per selected path; shell records undo around the gesture. */
	std::function<void(const ApplyPathFn&)> applyToSelection;
};

using OverlayFn = std::function<void(ImDrawList* dl, const SurfaceMap& map)>;

struct SurfaceSeam {
	std::vector<SurfacePickable> pickables;
	OverlayFn beforeContent;
	OverlayFn afterContent;
	SurfaceApply apply;
};

} // namespace artboards
} // namespace rigkit
