#include "SurfaceNumeric.h"

#include <cmath>
#include <imgui.h>
#include "PathEditOps.h"

namespace rigkit {
namespace artboards {

bool SurfaceNumeric::drawFields(SelBounds& bounds, const SurfaceApply& apply,
								const std::function<void()>& restoreSnap) {
	if (!bounds.valid || !apply.applyToSelection || !restoreSnap) {
		ImGui::BeginDisabled();
		float z = 0.f;
		ImGui::DragFloat("X", &z);
		ImGui::DragFloat("Y", &z);
		ImGui::DragFloat("W", &z);
		ImGui::DragFloat("H", &z);
		ImGui::DragFloat("Rotate", &z);
		ImGui::EndDisabled();
		return false;
	}

	float x = bounds.minX;
	float y = bounds.minY;
	float w = bounds.width();
	float h = bounds.height();
	bool changed = false;

	const float x0 = x, y0 = y, w0 = w, h0 = h;
	if (ImGui::DragFloat("X", &x, 0.1f)) {
		changed = true;
	}
	if (ImGui::DragFloat("Y", &y, 0.1f)) {
		changed = true;
	}
	if (ImGui::DragFloat("W", &w, 0.1f, 0.05f, 1.0e6f)) {
		changed = true;
	}
	if (ImGui::DragFloat("H", &h, 0.1f, 0.05f, 1.0e6f)) {
		changed = true;
	}
	float rot = rotateDeg;
	if (ImGui::DragFloat("Rotate", &rot, 0.25f)) {
		changed = true;
	}

	if (!changed) {
		return false;
	}

	restoreSnap();
	const glm::vec2 delta{x - x0, y - y0};
	if (std::abs(delta.x) > 1e-6f || std::abs(delta.y) > 1e-6f) {
		apply.applyToSelection(
			[&](ecs::CPaths& bag, int pi) { return pathEdit::translatePath(bag, pi, delta); });
	}
	const float sx = (std::abs(w0) > 1e-6f) ? (w / w0) : 1.f;
	const float sy = (std::abs(h0) > 1e-6f) ? (h / h0) : 1.f;
	if (sx != 1.f || sy != 1.f) {
		const glm::vec2 origin{x, y};
		apply.applyToSelection(
			[&](ecs::CPaths& bag, int pi) { return pathEdit::scalePath(bag, pi, origin, sx, sy); });
	}
	const float dRot = rot - rotateDeg;
	if (std::abs(dRot) > 1e-4f) {
		const float rad = dRot * 3.14159265f / 180.f;
		const glm::vec2 center{x + w * 0.5f, y + h * 0.5f};
		apply.applyToSelection([&](ecs::CPaths& bag, int pi) {
			return pathEdit::rotatePath(bag, pi, center, rad);
		});
	}
	rotateDeg = rot;
	bounds.minX = x;
	bounds.minY = y;
	bounds.maxX = x + w;
	bounds.maxY = y + h;
	return true;
}

} // namespace artboards
} // namespace rigkit
