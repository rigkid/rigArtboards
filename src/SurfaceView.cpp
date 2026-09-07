#include "SurfaceView.h"

#include <algorithm>
#include <cmath>

namespace rigkit {
namespace artboards {

void SurfaceView::handleZoom(ecs::CBedView& view, const glm::vec2& localPx, float factor) const {
	if (factor == 1.f || factor <= 0.f || view.scalePxPerMm <= 0.f) {
		return;
	}
	const float oldScale = view.scalePxPerMm;
	const glm::vec2 mm = view.pxToMm(localPx); // old scale - keep under cursor
	view.scalePxPerMm = std::clamp(oldScale * factor, minScale, maxScale);
	if (view.yUp) {
		view.originPx.x = localPx.x - mm.x * view.scalePxPerMm;
		view.originPx.y = localPx.y + mm.y * view.scalePxPerMm;
	} else {
		view.originPx = localPx - mm * view.scalePxPerMm;
	}
}

void SurfaceView::handleWheel(ecs::CBedView& view, const glm::vec2& localPx, float wheel) const {
	if (wheel == 0.f) {
		return;
	}
	handleZoom(view, localPx, wheel > 0.f ? wheelFactor : (1.f / wheelFactor));
}

void SurfaceView::handlePan(ecs::CBedView& view, const glm::vec2& deltaPx) const {
	view.originPx.x += deltaPx.x;
	view.originPx.y += deltaPx.y;
}

void SurfaceView::fit(ecs::CBedView& view, const ImVec2& canvasSize, const FitRectMm& world,
					  float marginPx) const {
	if (canvasSize.x < 40.f || canvasSize.y < 40.f) {
		return;
	}
	const float worldW = std::max(1.f, world.maxX - world.minX);
	const float worldH = std::max(1.f, world.maxY - world.minY);
	const float availW = std::max(1.f, canvasSize.x - 2.f * marginPx);
	const float availH = std::max(1.f, canvasSize.y - 2.f * marginPx);
	const float sx = availW / worldW;
	const float sy = availH / worldH;
	view.scalePxPerMm = std::clamp(std::min(sx, sy), minScale, maxScale);
	const float midX = (world.minX + world.maxX) * 0.5f;
	const float midY = (world.minY + world.maxY) * 0.5f;
	view.originPx.x = canvasSize.x * 0.5f - midX * view.scalePxPerMm;
	if (view.yUp) {
		view.originPx.y = canvasSize.y * 0.5f + midY * view.scalePxPerMm;
	} else {
		view.originPx.y = canvasSize.y * 0.5f - midY * view.scalePxPerMm;
	}
}

void SurfaceView::drawArtboards(ImDrawList* dl, const ImVec2& canvasPos, const ecs::CBedView& view,
								const std::vector<const ecs::CArtboard*>& boards,
								bool fillPage) const {
	if (!dl || view.scalePxPerMm <= 0.f) {
		return;
	}
	auto toScreen = [&](float x, float y) -> ImVec2 {
		const glm::vec2 px = view.mmToPx({x, y});
		return {canvasPos.x + px.x, canvasPos.y + px.y};
	};
	for (const ecs::CArtboard* board : boards) {
		if (!board || !board->visible) {
			continue;
		}
		const ImVec2 a = toScreen(board->pos.x, board->pos.y);
		const ImVec2 b = toScreen(board->pos.x + board->size.x, board->pos.y + board->size.y);
		const ImU32 fill = IM_COL32(static_cast<int>(board->color.r * 255.f),
									static_cast<int>(board->color.g * 255.f),
									static_cast<int>(board->color.b * 255.f),
									static_cast<int>(board->color.a * (fillPage ? 255.f : 90.f)));
		const ImU32 stroke = IM_COL32(40, 40, 40, 220);
		if (fillPage || board->color.a > 0.01f) {
			dl->AddRectFilled(a, b, fill);
		}
		dl->AddRect(a, b, stroke, 0.f, 0, 1.5f);
	}
}

void SurfaceView::drawGrid(ImDrawList* dl, const ImVec2& canvasPos, const ecs::CBedView& view,
						   const FitRectMm& rectMm, float stepMm, ImU32 col) const {
	if (!dl || view.scalePxPerMm <= 0.f || stepMm < 0.05f) {
		return;
	}
	const float x0 = std::min(rectMm.minX, rectMm.maxX);
	const float x1 = std::max(rectMm.minX, rectMm.maxX);
	const float y0 = std::min(rectMm.minY, rectMm.maxY);
	const float y1 = std::max(rectMm.minY, rectMm.maxY);
	auto toScreen = [&](float x, float y) -> ImVec2 {
		const glm::vec2 px = view.mmToPx({x, y});
		return {canvasPos.x + px.x, canvasPos.y + px.y};
	};
	for (float x = x0 + stepMm; x < x1 - 0.01f; x += stepMm) {
		dl->AddLine(toScreen(x, y0), toScreen(x, y1), col, 1.f);
	}
	for (float y = y0 + stepMm; y < y1 - 0.01f; y += stepMm) {
		dl->AddLine(toScreen(x0, y), toScreen(x1, y), col, 1.f);
	}
}

} // namespace artboards
} // namespace rigkit
