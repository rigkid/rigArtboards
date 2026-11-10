#include "SurfaceChrome.h"

#include "CGuide.h"
#include "CPlotDoc.h"
#include "PathEditOps.h"
#include "UiDpi.h"

#include <algorithm>
#include <cmath>

#if __has_include("IconsFontAwesome5.h")
#include "IconsFontAwesome5.h"
#define TOOL_GLYPH(x) x
#else
#define TOOL_GLYPH(x) ""
#endif

namespace rigkit {
namespace artboards {
namespace {

void drawKnob(ImDrawList* dl, const ImVec2& pt, float r, ImU32 fill, bool square) {
	if (square) {
		const ImVec2 a{pt.x - r, pt.y - r};
		const ImVec2 b{pt.x + r, pt.y + r};
		dl->AddRectFilled(a, b, fill);
	} else {
		dl->AddCircleFilled(pt, r, fill);
	}
}

void drawNode(ImDrawList* dl, const ImVec2& pt, float r, ImU32 fill, ImU32 outline, bool square) {
	if (square) {
		const ImVec2 a{pt.x - r, pt.y - r};
		const ImVec2 b{pt.x + r, pt.y + r};
		dl->AddRectFilled(a, b, fill);
		dl->AddRect(a, b, outline, 0.f, 0, uiPx(1.5f));
	} else {
		dl->AddCircleFilled(pt, r, fill);
		dl->AddCircle(pt, r, outline, 0, uiPx(1.5f));
	}
}

bool nodeIsSquare(const ecs::CPath& path, int commandIndex, int nodeTypeDisplay) {
	if (nodeTypeDisplay == 1) {
		return false;
	}
	if (nodeTypeDisplay == 2) {
		return true;
	}
	return !pathEdit::endpointIsSmooth(path, commandIndex);
}

bool showHandlesForNode(const PathChromeStyle& style, int selectedCommand, int nodeIndex) {
	if (!style.showHandles) {
		return false;
	}
	if (!style.handlesOnSelectedOnly) {
		return true;
	}
	return nodeIndex == selectedCommand;
}

} // namespace

void applyToolCursor(ecs::PathEditTool tool, ImDrawList* dl, const ImVec2& mousePos,
					 float brushDiameterMm, float pxPerMm) {
	using Tool = ecs::PathEditTool;
	const char* glyph = "";
	const char* suffix = "";
	bool crosshair = false;
	switch (tool) {
	case Tool::ObjectSelect:
		ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
		glyph = TOOL_GLYPH(ICON_FA_MOUSE_POINTER);
		break;
	case Tool::DirectSelect:
		ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
		glyph = TOOL_GLYPH(ICON_FA_MOUSE_POINTER);
		break;
	case Tool::PenDraw:
		ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
		glyph = TOOL_GLYPH(ICON_FA_PEN);
		crosshair = true;
		break;
	case Tool::PenAdd:
		ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
		glyph = TOOL_GLYPH(ICON_FA_PEN);
		suffix = "+";
		crosshair = true;
		break;
	case Tool::PenRemove:
		ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
		glyph = TOOL_GLYPH(ICON_FA_PEN);
		suffix = "-";
		crosshair = true;
		break;
	case Tool::Text:
		ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
		glyph = TOOL_GLYPH(ICON_FA_FONT);
		break;
	case Tool::Rect:
		ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
		glyph = TOOL_GLYPH(ICON_FA_SQUARE);
		crosshair = true;
		break;
	case Tool::Ellipse:
		ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
		glyph = TOOL_GLYPH(ICON_FA_CIRCLE);
		crosshair = true;
		break;
	case Tool::Spiral:
		ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
		glyph = TOOL_GLYPH(ICON_FA_SYNC);
		crosshair = true;
		break;
	case Tool::Brush:
		ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
		glyph = TOOL_GLYPH(ICON_FA_PAINT_BRUSH);
		break;
	case Tool::Wand:
		ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
		glyph = TOOL_GLYPH(ICON_FA_MAGIC);
		break;
	}
	if (!dl) {
		return;
	}
	if (crosshair) {
		const float arm = uiPx(7.f);
		const float gap = uiPx(2.5f);
		const ImU32 col = IM_COL32(40, 44, 52, 200);
		dl->AddLine(ImVec2(mousePos.x - arm, mousePos.y), ImVec2(mousePos.x - gap, mousePos.y), col,
					1.f);
		dl->AddLine(ImVec2(mousePos.x + gap, mousePos.y), ImVec2(mousePos.x + arm, mousePos.y), col,
					1.f);
		dl->AddLine(ImVec2(mousePos.x, mousePos.y - arm), ImVec2(mousePos.x, mousePos.y - gap), col,
					1.f);
		dl->AddLine(ImVec2(mousePos.x, mousePos.y + gap), ImVec2(mousePos.x, mousePos.y + arm), col,
					1.f);
	}
	if (tool == Tool::Brush && brushDiameterMm > 0.f && pxPerMm > 0.f) {
		dl->AddCircle(mousePos, std::max(1.f, brushDiameterMm * 0.5f * pxPerMm),
					  IM_COL32(64, 200, 255, 220), 0, 1.5f);
	}
	if (glyph[0] == '\0') {
		return;
	}
	// Direct Selection is Illustrator's hollow arrow: same glyph, light fill
	// on dark outline, vs the solid dark badge for Selection.
	const bool hollow = tool == Tool::DirectSelect;
	const ImVec2 at(mousePos.x + uiPx(14.f), mousePos.y + uiPx(14.f));
	const ImU32 shadow = IM_COL32(20, 22, 28, 220);
	const ImU32 ink = hollow ? IM_COL32(245, 245, 250, 255) : IM_COL32(30, 32, 40, 255);
	dl->AddText(ImVec2(at.x + 1.f, at.y + 1.f), shadow, glyph);
	dl->AddText(at, ink, glyph);
	if (suffix[0] != '\0') {
		const float w = ImGui::CalcTextSize(glyph).x;
		dl->AddText(ImVec2(at.x + w + uiPx(2.f) + 1.f, at.y + 1.f), shadow, suffix);
		dl->AddText(ImVec2(at.x + w + uiPx(2.f), at.y), ink, suffix);
	}
}

float pathChromeHitMm(const SurfaceMap& map, const PathChromeStyle& style) {
	const float scale =
		(map.view && map.view->scalePxPerMm > 0.05f) ? map.view->scalePxPerMm : 0.05f;
	return uiPx(style.nodePx + 3.5f) / scale;
}

void drawPathAnchors(ImDrawList* dl, const SurfaceMap& map, const ecs::CPath& path,
					 int selectedCommand, const PathChromeStyle& style) {
	if (!dl || !map.view) {
		return;
	}
	glm::vec2 prev{};
	int prevCi = -1;
	bool havePrev = false;
	const bool squareHandle = style.handleShape == 1;
	const float handleR = uiPx(style.handlePx);
	const float nodeR = uiPx(style.nodePx);
	const float nodeSelR = uiPx(style.nodePx + 1.5f);
	const float stem = uiPx(1.f);
	for (int ci = 0; ci < static_cast<int>(path.commands.size()); ++ci) {
		const auto& c = path.commands[static_cast<size_t>(ci)];
		if (c.type == ecs::CPath::Cmd::Close) {
			continue;
		}
		if ((c.type == ecs::CPath::Cmd::CubicTo || c.type == ecs::CPath::Cmd::QuadTo) && havePrev) {
			const bool showC1 = showHandlesForNode(style, selectedCommand, prevCi);
			const bool showC2 = showHandlesForNode(style, selectedCommand, ci);
			if (c.type == ecs::CPath::Cmd::CubicTo) {
				if (showC1) {
					dl->AddLine(map.toScreen(prev), map.toScreen(c.c1), style.handleCol, stem);
					drawKnob(dl, map.toScreen(c.c1), handleR, style.handleFill, squareHandle);
				}
				if (showC2) {
					dl->AddLine(map.toScreen(c.p), map.toScreen(c.c2), style.handleCol, stem);
					drawKnob(dl, map.toScreen(c.c2), handleR, style.handleFill, squareHandle);
				}
			} else if (showC1 || showC2) {
				dl->AddLine(map.toScreen(prev), map.toScreen(c.c1), style.handleCol, stem);
				drawKnob(dl, map.toScreen(c.c1), handleR, style.handleFill, squareHandle);
			}
		}
		const bool selected = (ci == selectedCommand);
		const float r = selected ? nodeSelR : nodeR;
		const ImU32 fill = selected ? style.nodeSelected : style.nodeFill;
		drawNode(dl, map.toScreen(c.p), r, fill, style.nodeOutline,
				 nodeIsSquare(path, ci, style.nodeTypeDisplay));
		prev = c.p;
		prevCi = ci;
		havePrev = true;
	}
}

void drawPenPreview(ImDrawList* dl, const SurfaceMap& map, const glm::vec2* fromMm,
					bool hasOutgoing, const glm::vec2& outgoingMm, const glm::vec2& downMm,
					const glm::vec2& mouseMm) {
	if (!dl || !map.view) {
		return;
	}
	const ImU32 curveCol = IM_COL32(255, 180, 80, 220);
	const ImU32 handleCol = IM_COL32(100, 160, 255, 200);
	const glm::vec2 drag = mouseMm - downMm;
	const glm::vec2 c2 = downMm - drag;
	const glm::vec2 outHandle = downMm + drag;

	if (fromMm && glm::length(*fromMm - downMm) > 1e-4f) {
		const glm::vec2 c1 =
			hasOutgoing ? outgoingMm : (*fromMm + (downMm - *fromMm) * (1.f / 3.f));
		constexpr int kSteps = 24;
		ImVec2 prev = map.toScreen(*fromMm);
		for (int i = 1; i <= kSteps; ++i) {
			const float t = static_cast<float>(i) / static_cast<float>(kSteps);
			const float u = 1.f - t;
			const glm::vec2 p = u * u * u * (*fromMm) + 3.f * u * u * t * c1 +
								3.f * u * t * t * c2 + t * t * t * downMm;
			const ImVec2 cur = map.toScreen(p);
			dl->AddLine(prev, cur, curveCol, uiPx(1.5f));
			prev = cur;
		}
		if (hasOutgoing) {
			dl->AddLine(map.toScreen(*fromMm), map.toScreen(c1), handleCol, uiPx(1.f));
			dl->AddCircleFilled(map.toScreen(c1), uiPx(3.f), handleCol);
		}
	}

	// Handle stems at the new point (outgoing = mouse, incoming = mirror).
	dl->AddLine(map.toScreen(downMm), map.toScreen(outHandle), handleCol, uiPx(1.f));
	dl->AddLine(map.toScreen(downMm), map.toScreen(c2), handleCol, uiPx(1.f));
	dl->AddCircleFilled(map.toScreen(outHandle), uiPx(3.5f), handleCol);
	dl->AddCircleFilled(map.toScreen(c2), uiPx(3.5f), handleCol);
	dl->AddCircleFilled(map.toScreen(downMm), uiPx(4.f), IM_COL32(255, 180, 80, 255));
}

void drawPenRubberBand(ImDrawList* dl, const SurfaceMap& map, const glm::vec2& fromMm,
					   bool hasOutgoing, const glm::vec2& outgoingMm, const glm::vec2& mouseMm) {
	if (!dl || !map.view) {
		return;
	}
	const ImU32 curveCol = IM_COL32(255, 180, 80, 180);
	const ImU32 handleCol = IM_COL32(100, 160, 255, 200);
	if (hasOutgoing) {
		const glm::vec2 in = fromMm - (outgoingMm - fromMm);
		dl->AddLine(map.toScreen(fromMm), map.toScreen(outgoingMm), handleCol, uiPx(1.f));
		dl->AddLine(map.toScreen(fromMm), map.toScreen(in), handleCol, uiPx(1.f));
		dl->AddCircleFilled(map.toScreen(outgoingMm), uiPx(3.5f), handleCol);
		dl->AddCircleFilled(map.toScreen(in), uiPx(3.5f), handleCol);
	}
	dl->AddLine(map.toScreen(fromMm), map.toScreen(mouseMm), curveCol, uiPx(1.5f));
	dl->AddCircleFilled(map.toScreen(fromMm), uiPx(4.f), IM_COL32(255, 180, 80, 255));
}

void drawMarquee(ImDrawList* dl, const SurfaceMap& map, const glm::vec2& aMm,
				 const glm::vec2& bMm) {
	if (!dl || !map.view) {
		return;
	}
	const ImVec2 a = map.toScreen(aMm);
	const ImVec2 b = map.toScreen(bMm);
	const ImVec2 r0{std::min(a.x, b.x), std::min(a.y, b.y)};
	const ImVec2 r1{std::max(a.x, b.x), std::max(a.y, b.y)};
	dl->AddRectFilled(r0, r1, IM_COL32(80, 160, 255, 40));
	dl->AddRect(r0, r1, IM_COL32(80, 160, 255, 220), 0.f, 0, uiPx(1.5f));
}

void drawGuides(ImDrawList* dl, const SurfaceMap& map, MEcs& ecs, uint32_t docId,
				entt::entity selected, float canvasW, float canvasH, bool show, bool dragActive,
				bool dragVertical, float dragPosMm) {
	if (!dl || !map.view || !show) {
		return;
	}
	const ImU32 col = IM_COL32(80, 180, 255, 160);
	const ImU32 selCol = IM_COL32(255, 200, 80, 230);
	const ImVec2 canvasPos = map.canvasPos;
	for (auto e : ecs.view<ecs::CGuide>()) {
		if (ecs.hasComponent<ecs::CPlotDocRef>(e) &&
			ecs.getComponent<ecs::CPlotDocRef>(e).docId != docId) {
			continue;
		}
		const auto& g = ecs.getComponent<ecs::CGuide>(e);
		const bool isSel = (e == selected);
		const ImU32 lineCol = isSel ? selCol : col;
		const float thickness = isSel ? 2.f : 1.f;
		if (g.vertical) {
			const float x =
				canvasPos.x + map.view->originPx.x + g.position * map.view->scalePxPerMm;
			if (x < canvasPos.x || x > canvasPos.x + canvasW) {
				continue;
			}
			dl->AddLine(ImVec2(x, canvasPos.y), ImVec2(x, canvasPos.y + canvasH), lineCol,
						thickness);
		} else {
			const float y = canvasPos.y + map.view->mmToPx({0.f, g.position}).y;
			if (y < canvasPos.y || y > canvasPos.y + canvasH) {
				continue;
			}
			dl->AddLine(ImVec2(canvasPos.x, y), ImVec2(canvasPos.x + canvasW, y), lineCol,
						thickness);
		}
	}
	if (dragActive) {
		const ImU32 dragCol = IM_COL32(255, 200, 80, 200);
		if (dragVertical) {
			const float x = canvasPos.x + map.view->originPx.x + dragPosMm * map.view->scalePxPerMm;
			dl->AddLine(ImVec2(x, canvasPos.y), ImVec2(x, canvasPos.y + canvasH), dragCol, 1.5f);
		} else {
			const float y = canvasPos.y + map.view->mmToPx({0.f, dragPosMm}).y;
			dl->AddLine(ImVec2(canvasPos.x, y), ImVec2(canvasPos.x + canvasW, y), dragCol, 1.5f);
		}
	}
}

entt::entity pickGuide(MEcs& ecs, uint32_t docId, const glm::vec2& mouseMm, float tolMm) {
	entt::entity hit = entt::null;
	float bestDist = tolMm;
	for (auto e : ecs.view<ecs::CGuide>()) {
		if (ecs.hasComponent<ecs::CPlotDocRef>(e) &&
			ecs.getComponent<ecs::CPlotDocRef>(e).docId != docId) {
			continue;
		}
		const auto& g = ecs.getComponent<ecs::CGuide>(e);
		const float d =
			g.vertical ? std::abs(mouseMm.x - g.position) : std::abs(mouseMm.y - g.position);
		if (d <= bestDist) {
			bestDist = d;
			hit = e;
		}
	}
	return hit;
}

} // namespace artboards
} // namespace rigkit
