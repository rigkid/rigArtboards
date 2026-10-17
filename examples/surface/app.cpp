#include "app.h"

#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "packs/rigArtboards/src/PathTools.h"
#include "packs/rigArtboards/src/SelectionBounds.h"
#include "packs/rigArtboards/src/SurfaceChrome.h"
#include "packs/rigArtboards/src/SurfaceSeam.h"
#include "packs/rigArtboards/src/rigArtboards.h"
#include "packs/rigComponent/src/rigComponent.h"
#include "packs/rigImGui/src/MWindow.h"
#include "packs/rigImGui/src/Mui.h"
#include "packs/rigImGui/src/UiDpi.h"
#include "packs/rigImGui/src/rigImGui.h"
#include "packs/rigPlotComponent/src/PathFlatten.h"
#include "packs/rigPlotComponent/src/rigPlotComponent.h"
#include "packs/rigSystems/src/rigSystems.h"
#include "packs/rigVectorEditor/src/PathEditOps.h"
#include "packs/rigVectorEditor/src/rigVectorEditor.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>
#include <vector>

namespace {

bool pointInBounds(const rigkit::artboards::SelBounds& b, const glm::vec2& mm) {
	return b.valid && mm.x >= b.minX && mm.x <= b.maxX && mm.y >= b.minY && mm.y <= b.maxY;
}

} // namespace

SurfaceHeroApp::SurfacePanel::SurfacePanel(rigkit::ecs::CBedView* view,
										   rigkit::ecs::CArtboard* board, rigkit::ecs::CPaths* bag,
										   rigkit::ecs::CPathEditSelection* sel)
	: IWindow("Surface", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
							 ImGuiWindowFlags_NoScrollWithMouse),
	  m_view(view), m_board(board), m_bag(bag), m_sel(sel) {}

void SurfaceHeroApp::SurfacePanel::renderContents() {
	if (!m_view || !m_board || !m_bag || !m_sel) {
		ImGui::TextUnformatted("Surface data missing");
		return;
	}

	const ImGuiStyle& style = ImGui::GetStyle();
	const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
	const ImVec2 avail = ImGui::GetContentRegionAvail();
	const float hintH = ImGui::GetTextLineHeightWithSpacing();
	const float fieldsH = ImGui::GetFrameHeightWithSpacing() * 5.f;
	const float footerH = hintH + fieldsH + style.ItemSpacing.y * 2.f;
	const float width = std::max(rigkit::uiPx(80.f), avail.x);
	const float height = std::max(rigkit::uiPx(80.f), avail.y - footerH);

	if (width > rigkit::uiPx(120.f) && height > rigkit::uiPx(120.f) &&
		(std::fabs(width - m_fitW) > 1.f || std::fabs(height - m_fitH) > 1.f)) {
		m_surface.fit(*m_view, ImVec2(width, height),
					  {m_board->pos.x, m_board->pos.y, m_board->pos.x + m_board->size.x,
					   m_board->pos.y + m_board->size.y},
					  style.WindowPadding.x);
		m_fitW = width;
		m_fitH = height;
	}

	ImGui::InvisibleButton("surface_canvas", rigkit::uiHitSize(ImVec2(width, height)),
						   ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle);
	const bool hovered = ImGui::IsItemHovered();
	const bool active = ImGui::IsItemActive();
	ImGuiIO& io = ImGui::GetIO();
	const bool shift = io.KeyShift;
	const bool alt = io.KeyAlt;
	const bool middle = ImGui::IsMouseDown(ImGuiMouseButton_Middle);

	rigkit::artboards::SurfaceMap map;
	map.canvasPos = canvasPos;
	map.view = m_view;
	const glm::vec2 mouseMm = map.toMm(io.MousePos);
	const glm::vec2 localPx{io.MousePos.x - canvasPos.x, io.MousePos.y - canvasPos.y};

	if (hovered && io.MouseWheel != 0.f) {
		m_surface.handleWheel(*m_view, localPx, io.MouseWheel);
	}
	if (active && middle) {
		m_surface.handlePan(*m_view, {io.MouseDelta.x, io.MouseDelta.y});
	}

	auto bounds = rigkit::artboards::selectionBoundsOfBag(*m_bag, *m_sel);
	rigkit::artboards::SurfaceApply apply;
	apply.applyToSelection = [&](const rigkit::artboards::ApplyPathFn& fn) {
		for (const auto& ref : m_sel->selectedRefs()) {
			if (ref.layerIndex != 0 || ref.pathIndex < 0 ||
				ref.pathIndex >= static_cast<int>(m_bag->paths.size())) {
				continue;
			}
			fn(*m_bag, ref.pathIndex);
		}
	};
	const auto restoreSnap = [this]() {
		if (m_haveSnap) {
			*m_bag = m_snap;
		}
	};

	if (hovered && !middle && ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
		m_snap = *m_bag;
		m_haveSnap = true;
		m_marquee = false;
		if (m_gizmo.beginClick(map, bounds, io.MousePos, mouseMm, shift, alt, true)) {
			// scale / rotate from a handle
		} else {
			const float tolMm = std::max(1.f, 8.f / std::max(0.05f, m_view->scalePxPerMm));
			int pi = -1;
			int ci = -1;
			auto part = rigkit::ecs::PathEditPart::None;
			if (rigkit::pathEdit::pickHit(*m_bag, mouseMm, tolMm, pi, ci, part, false)) {
				if (shift) {
					m_sel->togglePath(0, pi);
				} else if (!m_sel->isPathSelected(0, pi)) {
					m_sel->selectOnly(0, pi);
				}
				bounds = rigkit::artboards::selectionBoundsOfBag(*m_bag, *m_sel);
				if (bounds.valid) {
					m_gizmo.beginMove(bounds, mouseMm);
				}
			} else if (pointInBounds(bounds, mouseMm)) {
				m_gizmo.beginMove(bounds, mouseMm);
			} else {
				if (!shift) {
					m_sel->clearPaths();
				}
				m_marquee = true;
				m_marqueeA = mouseMm;
				m_marqueeB = mouseMm;
			}
		}
	}

	if (active && !middle && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		if (m_marquee) {
			m_marqueeB = mouseMm;
		} else if (m_gizmo.xform.kind != rigkit::artboards::SurfaceDrag::None) {
			m_gizmo.drag(map, mouseMm, shift, alt, apply, restoreSnap);
		}
	}

	if (ImGui::IsItemDeactivated()) {
		if (m_marquee) {
			const float dragPx = glm::length((m_marqueeB - m_marqueeA) * m_view->scalePxPerMm);
			if (dragPx > 4.f) {
				const auto hits =
					rigkit::artboards::marqueeHitsBag(*m_bag, 0, m_marqueeA, m_marqueeB);
				rigkit::artboards::applyMarqueeHits(*m_sel, hits, shift);
			}
			m_marquee = false;
		}
		m_gizmo.end();
		m_haveSnap = false;
	}

	ImDrawList* dl = ImGui::GetWindowDrawList();
	const ImVec2 canvasMax(canvasPos.x + width, canvasPos.y + height);
	const float rounding = style.FrameRounding;
	dl->AddRectFilled(canvasPos, canvasMax, ImGui::GetColorU32(ImGuiCol_ChildBg), rounding);
	dl->AddRect(canvasPos, canvasMax, ImGui::GetColorU32(ImGuiCol_Border), rounding, 0,
				style.FrameBorderSize > 0.f ? style.FrameBorderSize : 1.f);
	dl->PushClipRect(canvasPos, canvasMax, true);

	const std::vector<const rigkit::ecs::CArtboard*> boards{m_board};
	m_surface.drawArtboards(dl, canvasPos, *m_view, boards, true);
	if (m_board->showGrid) {
		const float step = m_board->gridStepMm > 0.05f ? m_board->gridStepMm : 10.f;
		m_surface.drawGrid(dl, canvasPos, *m_view,
						   {m_board->pos.x, m_board->pos.y, m_board->pos.x + m_board->size.x,
							m_board->pos.y + m_board->size.y},
						   step, ImGui::GetColorU32(ImGuiCol_Border, 0.35f));
	}

	auto toScreen = [&](const glm::vec2& mm) {
		const glm::vec2 px = m_view->mmToPx(mm);
		return ImVec2(canvasPos.x + px.x, canvasPos.y + px.y);
	};

	const ImU32 pathCol = ImGui::GetColorU32(ImGuiCol_Text, 0.85f);
	const ImU32 pathSelCol = ImGui::GetColorU32(ImGuiCol_SliderGrabActive);
	const float stroke = rigkit::uiPx(2.f);
	const float strokeSel = rigkit::uiPx(2.5f);

	for (size_t pi = 0; pi < m_bag->paths.size(); ++pi) {
		const auto poly = rigkit::plot::flattenPath(m_bag->paths[pi]);
		if (poly.size() < 2) {
			continue;
		}
		const bool selected = m_sel->isPathSelected(0, static_cast<int>(pi));
		const ImU32 col = selected ? pathSelCol : pathCol;
		const float thick = selected ? strokeSel : stroke;
		for (size_t i = 1; i < poly.size(); ++i) {
			dl->AddLine(toScreen(poly[i - 1]), toScreen(poly[i]), col, thick);
		}
	}

	for (const auto& ref : m_sel->selectedRefs()) {
		if (ref.layerIndex != 0 || ref.pathIndex < 0 ||
			ref.pathIndex >= static_cast<int>(m_bag->paths.size())) {
			continue;
		}
		const int cmd = (ref.pathIndex == m_sel->pathIndex) ? m_sel->commandIndex : -1;
		rigkit::artboards::drawPathAnchors(dl, map,
										   m_bag->paths[static_cast<size_t>(ref.pathIndex)], cmd);
	}

	bounds = rigkit::artboards::selectionBoundsOfBag(*m_bag, *m_sel);
	if (bounds.valid) {
		m_gizmo.draw(dl, map, bounds, true);
	}
	if (m_marquee) {
		rigkit::artboards::drawMarquee(dl, map, m_marqueeA, m_marqueeB);
	}

	dl->PopClipRect();

	ImGui::SetCursorScreenPos(ImVec2(canvasPos.x, canvasPos.y + height + style.ItemSpacing.y));
	ImGui::TextDisabled("Click select   Drag move/scale/rotate   Wheel zoom   Middle pan");
	if (m_gizmo.xform.kind == rigkit::artboards::SurfaceDrag::None) {
		m_numeric.drawFields(bounds, apply, [] {});
	} else {
		ImGui::BeginDisabled();
		m_numeric.drawFields(bounds, apply, [] {});
		ImGui::EndDisabled();
	}
}

SurfaceHeroApp::SurfaceHeroApp() {
	window().width = 780;
	window().height = 560;
	window().title = "rigArtboards - surface";
	settings().appName = "surface";
}

void SurfaceHeroApp::setup() {
	spdlog::info("surface - editable artboard (select, gizmo, numeric)");
	m_engine->setClearColor(0.09f, 0.10f, 0.12f, 1.f);

	auto* packs = m_engine->getPackManager();
	if (!packs) {
		return;
	}
	packs->registerPack(std::make_shared<rigkit::rigComponent>());
	packs->registerPack(std::make_shared<rigkit::rigSystems>());
	packs->registerPack(std::make_shared<rigkit::rigPlotComponent>());
	packs->registerPack(std::make_shared<rigkit::rigVectorEditor>());
	packs->registerPack(std::make_shared<rigkit::rigImGui>());
	packs->registerPack(std::make_shared<rigkit::rigArtboards>());
	packs->initAll();
	packs->setupAll();

	m_board.id = "page";
	m_board.name = "Artboard 1";
	m_board.pos = {0.f, 0.f};
	m_board.size = {210.f, 297.f}; // A4 mm
	m_board.color = {0.96f, 0.96f, 0.94f, 1.f};
	m_board.showGrid = true;
	m_board.gridStepMm = 10.f;

	using Cmd = rigkit::ecs::CPath::Cmd;
	rigkit::ecs::CPath rect;
	rect.commands.push_back({Cmd::MoveTo, {40.f, 50.f}, {}, {}});
	rect.commands.push_back({Cmd::LineTo, {140.f, 50.f}, {}, {}});
	rect.commands.push_back({Cmd::LineTo, {140.f, 120.f}, {}, {}});
	rect.commands.push_back({Cmd::LineTo, {40.f, 120.f}, {}, {}});
	rect.commands.push_back({Cmd::LineTo, {40.f, 50.f}, {}, {}});
	m_bag.paths.push_back(std::move(rect));

	rigkit::ecs::CPath curve;
	curve.commands.push_back({Cmd::MoveTo, {50.f, 180.f}, {}, {}});
	curve.commands.push_back({Cmd::CubicTo, {160.f, 180.f}, {80.f, 140.f}, {130.f, 220.f}});
	m_bag.paths.push_back(std::move(curve));

	m_sel.selectOnly(0, 0);

	if (auto* mui = dynamic_cast<rigkit::Mui*>(m_engine->getUiManager())) {
		if (auto* wm = mui->getWindowManager()) {
			wm->createWindow<SurfacePanel>(&m_view, &m_board, &m_bag, &m_sel);
			wm->showWindow("Surface");
		}
		// First-run: Surface fills the central dock. No-ops once imgui.ini has a split.
		mui->setDockLayoutBuilder([mui](ImGuiID dockspaceId) {
			ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspaceId);
			if (!node) {
				return;
			}
			if (node->IsSplitNode()) {
				mui->setDockLayoutBuilder(nullptr);
				return;
			}
			const ImGuiViewport* vp = ImGui::GetMainViewport();
			ImGui::DockBuilderRemoveNode(dockspaceId);
			ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize(dockspaceId, vp->WorkSize);
			ImGui::DockBuilderDockWindow("Surface", dockspaceId);
			ImGui::DockBuilderFinish(dockspaceId);
			mui->setDockLayoutBuilder(nullptr);
		});
	}
}
