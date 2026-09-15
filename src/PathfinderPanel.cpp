#include "PathfinderPanel.h"

#include "CPathEditSelection.h"
#include "CPaths.h"
#include "PathBooleanOps.h"
#include "PathEditOps.h"
#include "PlotDoc.h"

#include <algorithm>
#include <imgui.h>
#include <vector>

namespace rigkit {
namespace {

int copyPathEntry(const ecs::CPaths& src, size_t i, ecs::CPaths& dst) {
	const int idx = static_cast<int>(dst.paths.size());
	dst.paths.push_back(src.paths[i]);
	pathEdit::ensurePathPaintSlots(dst, idx);
	if (i < src.pathColors.size()) {
		dst.pathColors[idx] = src.pathColors[i];
	}
	dst.pathFilled[idx] = src.isFilled(i) ? 1 : 0;
	dst.pathStroked[idx] = src.isStroked(i) ? 1 : 0;
	if (i < src.pathStrokeColors.size()) {
		dst.pathStrokeColors[idx] = src.pathStrokeColors[i];
	}
	dst.pathStrokeWidths[idx] = src.strokeWidth(i);
	dst.pathFillPaint[idx] = src.fillPaint(i);
	dst.pathStrokePaint[idx] = src.strokePaint(i);
	if (src.isEvenOdd(i)) {
		dst.pathFillRule.resize(static_cast<size_t>(idx) + 1, 0);
		dst.pathFillRule[static_cast<size_t>(idx)] = 1;
	}
	return idx;
}

} // namespace

PathfinderPanel::PathfinderPanel(const std::string& title) : IWindow(title) {
	setCategory("Artboards");
}

void PathfinderPanel::renderContents() {
	if (!m_doc) {
		ImGui::TextUnformatted("No PlotDoc");
		return;
	}
	auto& sel = m_doc->pathEdit();
	const auto layers = m_doc->layerEntities();

	auto bagOf = [&](int li) -> ecs::CPaths* {
		if (li < 0 || li >= static_cast<int>(layers.size())) {
			return nullptr;
		}
		auto e = layers[static_cast<size_t>(li)];
		if (!m_doc->ecs().hasComponent<ecs::CPaths>(e)) {
			return nullptr;
		}
		return &m_doc->ecs().getComponent<ecs::CPaths>(e);
	};

	const auto refs = sel.selectedRefs();
	ImGui::TextWrapped("Shift+click 2+ paths on the artboard or Layers.");
	ImGui::Text("Selected: %zu", refs.size());
	const bool can = refs.size() >= 2;

	auto& flatten = m_doc->meta().flattenTolMm;
	if (ImGui::DragFloat("Flatten", &flatten, 0.005f, 0.01f, 2.f, "%.3f mm")) {
		m_doc->markPathsGpuDirty();
		m_doc->setDirty(true);
		m_doc->bumpToolpathPreviewEpoch();
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("How far a chord may stray from a curve (mm).\n"
						  "Pathfinder bakes this into the result paths.");
	}

	auto run = [&](pathEdit::PathBooleanOp op, const char* label) {
		if (!can) {
			return;
		}
		auto body = [&] {
			const int primary = sel.layerIndex >= 0 ? sel.layerIndex : refs.front().layerIndex;
			auto* dst = bagOf(primary);
			if (!dst) {
				m_status = "No paths on layer";
				return;
			}

			std::vector<int> opIndices;
			std::vector<ecs::PathSelRef> foreign;
			const int pathCountBeforeCopy = static_cast<int>(dst->paths.size());
			for (const auto& r : refs) {
				if (!r.valid()) {
					continue;
				}
				if (r.layerIndex == primary) {
					opIndices.push_back(r.pathIndex);
					continue;
				}
				const auto* src = bagOf(r.layerIndex);
				if (!src || r.pathIndex >= static_cast<int>(src->paths.size())) {
					m_status = "Stale selection";
					return;
				}
				foreign.push_back(r);
				opIndices.push_back(copyPathEntry(*src, static_cast<size_t>(r.pathIndex), *dst));
			}
			if (opIndices.size() < 2) {
				while (static_cast<int>(dst->paths.size()) > pathCountBeforeCopy) {
					pathEdit::deletePath(*dst, static_cast<int>(dst->paths.size()) - 1);
				}
				m_status = "Select 2+ paths";
				return;
			}

			if (!pathEdit::applyPathBoolean(*dst, opIndices, op, m_doc->flattenTolMm())) {
				while (static_cast<int>(dst->paths.size()) > pathCountBeforeCopy) {
					pathEdit::deletePath(*dst, static_cast<int>(dst->paths.size()) - 1);
				}
				m_status = "Failed";
				return;
			}
			std::sort(foreign.begin(), foreign.end(),
					  [](const ecs::PathSelRef& a, const ecs::PathSelRef& b) {
						  return a.layerIndex != b.layerIndex ? a.layerIndex > b.layerIndex
															  : a.pathIndex > b.pathIndex;
					  });
			for (const auto& r : foreign) {
				if (auto* src = bagOf(r.layerIndex)) {
					pathEdit::deletePath(*src, r.pathIndex);
				}
			}
			sel.clearPaths();
			const int last = static_cast<int>(dst->paths.size()) - 1;
			if (last >= 0) {
				sel.selectOnly(primary, last);
			}
			m_doc->setDirty(true);
			m_status = label;
		};

		if (m_mutate) {
			m_mutate(label, body);
		} else {
			body();
		}
	};

	if (!can) {
		ImGui::BeginDisabled();
	}

	ImGui::TextDisabled("Shape Modes");
	if (ImGui::Button("Unite", ImVec2(-1, 0))) {
		run(pathEdit::PathBooleanOp::Unite, "Unite");
	}
	if (ImGui::Button("Minus Front", ImVec2(-1, 0))) {
		run(pathEdit::PathBooleanOp::MinusFront, "Minus Front");
	}
	if (ImGui::Button("Minus Back", ImVec2(-1, 0))) {
		run(pathEdit::PathBooleanOp::MinusBack, "Minus Back");
	}
	if (ImGui::Button("Intersect", ImVec2(-1, 0))) {
		run(pathEdit::PathBooleanOp::Intersect, "Intersect");
	}
	if (ImGui::Button("Exclude", ImVec2(-1, 0))) {
		run(pathEdit::PathBooleanOp::Exclude, "Exclude");
	}

	ImGui::Spacing();
	ImGui::TextDisabled("Pathfinders");
	if (ImGui::Button("Divide", ImVec2(-1, 0))) {
		run(pathEdit::PathBooleanOp::Divide, "Divide");
	}
	if (ImGui::Button("Trim", ImVec2(-1, 0))) {
		run(pathEdit::PathBooleanOp::Trim, "Trim");
	}
	if (ImGui::Button("Merge", ImVec2(-1, 0))) {
		run(pathEdit::PathBooleanOp::Merge, "Merge");
	}
	if (ImGui::Button("Crop", ImVec2(-1, 0))) {
		run(pathEdit::PathBooleanOp::Crop, "Crop");
	}
	if (ImGui::Button("Outline", ImVec2(-1, 0))) {
		run(pathEdit::PathBooleanOp::Outline, "Outline");
	}

	if (!can) {
		ImGui::EndDisabled();
	}

	ImGui::Spacing();
	ImGui::TextDisabled("Offset");
	const bool canOffset = !refs.empty() && m_offsetPath;
	if (!canOffset) {
		ImGui::BeginDisabled();
	}
	if (ImGui::Button("Offset Path...", ImVec2(-1, 0)) && m_offsetPath) {
		m_offsetPath();
	}
	if (!canOffset) {
		ImGui::EndDisabled();
	}
	if (!m_status.empty()) {
		ImGui::Separator();
		ImGui::TextUnformatted(m_status.c_str());
	}
}

} // namespace rigkit
