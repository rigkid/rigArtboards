#include "AlignPanel.h"
#include "CArtboard.h"
#include "CPathEditSelection.h"
#include "PathAlignOps.h"
#include "PlotDoc.h"
#include "SelectionBounds.h"
#include <imgui.h>

namespace rigkit {

AlignPanel::AlignPanel(const std::string& title) : IWindow(title) {
	setCategory("Artboards");
}

void AlignPanel::renderContents() {
	if (!m_doc) {
		ImGui::TextUnformatted("No PlotDoc");
		return;
	}

	auto& sel = m_doc->pathEdit();
	const auto refs = sel.selectedRefs();
	ImGui::Text("Selected: %zu", refs.size());
	const bool canAlign = !refs.empty();
	const bool canDistribute = refs.size() >= 3;

	int target = static_cast<int>(m_target);
	ImGui::RadioButton("Selection", &target, static_cast<int>(Target::Selection));
	ImGui::SameLine();
	ImGui::RadioButton("Artboard", &target, static_cast<int>(Target::Artboard));
	m_target = static_cast<Target>(target);

	pathEdit::AlignFrame frame;
	if (m_target == Target::Selection) {
		const auto b = artboards::selectionBounds(m_doc->ecs(), m_doc->layerEntities(), sel);
		if (b.valid) {
			frame = {b.minX, b.minY, b.maxX, b.maxY};
		} else {
			ImGui::TextDisabled("Select paths to define the frame.");
		}
	} else {
		bool haveBoard = false;
		for (auto e : m_doc->ecs().view<ecs::CArtboard>()) {
			const auto& ab = m_doc->ecs().getComponent<ecs::CArtboard>(e);
			if (!ab.visible) {
				continue;
			}
			frame = {ab.pos.x, ab.pos.y, ab.pos.x + ab.size.x, ab.pos.y + ab.size.y};
			haveBoard = true;
			break;
		}
		if (!haveBoard) {
			const auto& paper = m_doc->meta().sizeMM;
			frame = {0.f, 0.f, paper.x > 1.f ? paper.x : 210.f, paper.y > 1.f ? paper.y : 297.f};
		}
	}

	auto run = [&](pathEdit::PathAlignOp op, const char* label, bool enabled) {
		if (!enabled) {
			return;
		}
		auto body = [&] {
			if (pathEdit::applyPathAlignRefs(m_doc->ecs(), m_doc->layerEntities(), refs, op, frame)) {
				m_doc->setDirty(true);
				m_status = label;
			} else {
				m_status = "No change";
			}
		};
		if (m_mutate) {
			m_mutate(label, body);
		} else {
			body();
		}
	};

	ImGui::Separator();
	ImGui::TextDisabled("Align");
	if (!canAlign) {
		ImGui::BeginDisabled();
	}
	if (ImGui::Button("Left", ImVec2(-1, 0))) {
		run(pathEdit::PathAlignOp::Left, "Align Left", true);
	}
	if (ImGui::Button("H Center", ImVec2(-1, 0))) {
		run(pathEdit::PathAlignOp::HCenter, "Align H Center", true);
	}
	if (ImGui::Button("Right", ImVec2(-1, 0))) {
		run(pathEdit::PathAlignOp::Right, "Align Right", true);
	}
	if (ImGui::Button("Top", ImVec2(-1, 0))) {
		run(pathEdit::PathAlignOp::Top, "Align Top", true);
	}
	if (ImGui::Button("V Center", ImVec2(-1, 0))) {
		run(pathEdit::PathAlignOp::VCenter, "Align V Center", true);
	}
	if (ImGui::Button("Bottom", ImVec2(-1, 0))) {
		run(pathEdit::PathAlignOp::Bottom, "Align Bottom", true);
	}
	if (!canAlign) {
		ImGui::EndDisabled();
	}

	ImGui::Spacing();
	ImGui::TextDisabled("Distribute");
	if (!canDistribute) {
		ImGui::BeginDisabled();
	}
	if (ImGui::Button("Distribute H", ImVec2(-1, 0))) {
		run(pathEdit::PathAlignOp::DistributeH, "Distribute H", true);
	}
	if (ImGui::Button("Distribute V", ImVec2(-1, 0))) {
		run(pathEdit::PathAlignOp::DistributeV, "Distribute V", true);
	}
	if (!canDistribute) {
		ImGui::EndDisabled();
	}

	if (!m_status.empty()) {
		ImGui::Separator();
		ImGui::TextUnformatted(m_status.c_str());
	}
}

} // namespace rigkit
