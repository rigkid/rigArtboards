#include "app.h"

#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>
#include <vector>

#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "imgui_internal.h"
#include "packs/rigArtboards/src/SelectionBounds.h"
#include "packs/rigArtboards/src/rigArtboards.h"
#include "packs/rigComponent/src/rigComponent.h"
#include "packs/rigImGui/src/MWindow.h"
#include "packs/rigImGui/src/Mui.h"
#include "packs/rigImGui/src/UiDpi.h"
#include "packs/rigImGui/src/rigImGui.h"
#include "packs/rigPlotComponent/src/PathBounds.h"
#include "packs/rigPlotComponent/src/PathFlatten.h"
#include "packs/rigPlotComponent/src/rigPlotComponent.h"
#include "packs/rigSystems/src/rigSystems.h"
#include "packs/rigVectorEditor/src/rigVectorEditor.h"

namespace {

/** @brief Union mm bounds of selected paths in a single-layer bag (hero has no ECS layers). */
rigkit::artboards::SelBounds selectionBoundsOfBag(const rigkit::ecs::CPaths& bag,
												   const rigkit::ecs::CPathEditSelection& sel) {
	rigkit::artboards::SelBounds out;
	bool have = false;
	for (const auto& ref : sel.selectedRefs()) {
		if (ref.layerIndex != 0 || ref.pathIndex < 0 ||
			ref.pathIndex >= static_cast<int>(bag.paths.size())) {
			continue;
		}
		const auto pb = rigkit::plot::boundsOfPath(bag.paths[static_cast<size_t>(ref.pathIndex)]);
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

} // namespace

SurfaceHeroApp::SurfacePanel::SurfacePanel(rigkit::ecs::CBedView* view,
										   rigkit::ecs::CArtboard* board,
										   rigkit::ecs::CPaths* bag,
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
	const float footerH = ImGui::GetFrameHeightWithSpacing() + style.ItemSpacing.y;
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

	if (hovered && io.MouseWheel != 0.f) {
		m_surface.handleWheel(*m_view, {io.MousePos.x - canvasPos.x, io.MousePos.y - canvasPos.y},
							  io.MouseWheel);
	}
	if (active && ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
		m_surface.handlePan(*m_view, {io.MouseDelta.x, io.MouseDelta.y});
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

	rigkit::artboards::SurfaceMap map;
	map.canvasPos = canvasPos;
	map.view = m_view;
	const auto bounds = selectionBoundsOfBag(*m_bag, *m_sel);
	if (bounds.valid) {
		m_gizmo.draw(dl, map, bounds, true);
	}

	dl->PopClipRect();

	ImGui::SetCursorScreenPos(
		ImVec2(canvasPos.x, canvasPos.y + height + style.ItemSpacing.y));
	ImGui::TextDisabled("Wheel zoom   Middle-drag pan   A4 artboard");
}

SurfaceHeroApp::SurfaceHeroApp() {
	window().width = 780;
	window().height = 560;
	window().title = "rigArtboards - surface";
	settings().appName = "surface";
}

void SurfaceHeroApp::setup() {
	spdlog::info("surface - artboard view + selection gizmo");
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
