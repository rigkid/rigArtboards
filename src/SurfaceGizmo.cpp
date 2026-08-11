#include "SurfaceGizmo.h"
#include "PathEditOps.h"
#include <cmath>

namespace rigkit {
namespace artboards {
namespace {

constexpr float kPi = 3.14159265f;

void handlePositions(float r, float t, float w, float h, float hx[8], float hy[8]) {
	hx[0] = r;
	hy[0] = t;
	hx[1] = r + w;
	hy[1] = t;
	hx[2] = r + w;
	hy[2] = t + h;
	hx[3] = r;
	hy[3] = t + h;
	hx[4] = r + w * 0.5f;
	hy[4] = t;
	hx[5] = r + w;
	hy[5] = t + h * 0.5f;
	hx[6] = r + w * 0.5f;
	hy[6] = t + h;
	hx[7] = r;
	hy[7] = t + h * 0.5f;
}

glm::vec2 scaleOrigin(float r0, float t0, float w0, float h0, int h, bool aboutCentre) {
	if (aboutCentre) {
		return {r0 + w0 * 0.5f, t0 + h0 * 0.5f};
	}
	if (h == 0) {
		return {r0 + w0, t0 + h0};
	}
	if (h == 1) {
		return {r0, t0 + h0};
	}
	if (h == 2) {
		return {r0, t0};
	}
	if (h == 3) {
		return {r0 + w0, t0};
	}
	if (h == 4) {
		return {r0 + w0 * 0.5f, t0 + h0};
	}
	if (h == 5) {
		return {r0, t0 + h0 * 0.5f};
	}
	if (h == 6) {
		return {r0 + w0 * 0.5f, t0};
	}
	if (h == 7) {
		return {r0 + w0, t0 + h0 * 0.5f};
	}
	return {r0, t0};
}

} // namespace

void setScaleCursor(int handle) {
	static const ImGuiMouseCursor kCursor[8] = {
		ImGuiMouseCursor_ResizeNWSE, ImGuiMouseCursor_ResizeNESW, ImGuiMouseCursor_ResizeNWSE,
		ImGuiMouseCursor_ResizeNESW, ImGuiMouseCursor_ResizeNS,	  ImGuiMouseCursor_ResizeEW,
		ImGuiMouseCursor_ResizeNS,	 ImGuiMouseCursor_ResizeEW};
	if (handle >= 0 && handle < 8) {
		ImGui::SetMouseCursor(kCursor[handle]);
	}
}

void SurfaceGizmo::liveBox(float& r, float& t, float& w, float& h) const {
	if (xform.kind == SurfaceDrag::Scale) {
		r = xform.liveX;
		t = xform.liveY;
		w = xform.liveW;
		h = xform.liveH;
		return;
	}
	r = xform.x0;
	t = xform.y0;
	w = xform.w0;
	h = xform.h0;
}

int SurfaceGizmo::hitScaleHandle(const SurfaceMap& map, const SelBounds& bounds,
								 const ImVec2& mousePx) const {
	if (!bounds.valid || !map.view) {
		return -1;
	}
	float hx[8]{}, hy[8]{};
	handlePositions(bounds.minX, bounds.minY, bounds.width(), bounds.height(), hx, hy);
	const float hitR = handleRadiusPx + scaleHitPadPx;
	for (int i = 0; i < 8; ++i) {
		const ImVec2 s = map.toScreen({hx[i], hy[i]});
		const float dx = mousePx.x - s.x;
		const float dy = mousePx.y - s.y;
		if (dx * dx + dy * dy <= hitR * hitR) {
			return i;
		}
	}
	return -1;
}

bool SurfaceGizmo::hitRotate(const SurfaceMap& map, const SelBounds& bounds,
							 const ImVec2& mousePx) const {
	if (!bounds.valid || !map.view) {
		return false;
	}
	const float stemMm =
		std::max(2.f, rotateStemMm / std::max(0.05f, map.view->scalePxPerMm));
	const glm::vec2 topCenter{bounds.minX + bounds.width() * 0.5f, bounds.minY};
	const glm::vec2 rotMm{topCenter.x, topCenter.y - stemMm};
	const ImVec2 rotS = map.toScreen(rotMm);
	const float rotStemHitR = handleRadiusPx + 6.f;
	const float rdx = mousePx.x - rotS.x;
	const float rdy = mousePx.y - rotS.y;
	if (rdx * rdx + rdy * rdy <= rotStemHitR * rotStemHitR) {
		return true;
	}
	float hx[8]{}, hy[8]{};
	handlePositions(bounds.minX, bounds.minY, bounds.width(), bounds.height(), hx, hy);
	const ImVec2 center = map.toScreen(
		{bounds.minX + bounds.width() * 0.5f, bounds.minY + bounds.height() * 0.5f});
	const float rotCornerInner = handleRadiusPx + scaleHitPadPx + 1.f;
	for (int i = 0; i < 4; ++i) {
		const ImVec2 sh = map.toScreen({hx[i], hy[i]});
		const float dx = mousePx.x - sh.x;
		const float dy = mousePx.y - sh.y;
		const float d2 = dx * dx + dy * dy;
		if (d2 < rotCornerInner * rotCornerInner ||
			d2 > rotateCornerOuterPx * rotateCornerOuterPx) {
			continue;
		}
		const float vx = sh.x - center.x;
		const float vy = sh.y - center.y;
		const float mx = mousePx.x - center.x;
		const float my = mousePx.y - center.y;
		if (mx * vx + my * vy > vx * vx + vy * vy) {
			return true;
		}
	}
	return false;
}

void SurfaceGizmo::draw(ImDrawList* dl, const SurfaceMap& map, const SelBounds& bounds,
						bool active) const {
	if (!dl || !bounds.valid || !map.view) {
		return;
	}
	float boxR = bounds.minX;
	float boxT = bounds.minY;
	float boxW = bounds.width();
	float boxH = bounds.height();
	if (xform.kind == SurfaceDrag::Scale) {
		liveBox(boxR, boxT, boxW, boxH);
	}

	const ImVec2 b0 = map.toScreen({boxR, boxT});
	const ImVec2 b1 = map.toScreen({boxR + boxW, boxT + boxH});
	dl->AddRect(b0, b1, IM_COL32(255, 200, 80, 220), 0.f, 0, 1.5f);

	float hx[8]{}, hy[8]{};
	handlePositions(boxR, boxT, boxW, boxH, hx, hy);
	for (int i = 0; i < 8; ++i) {
		const ImVec2 s = map.toScreen({hx[i], hy[i]});
		dl->AddRectFilled(ImVec2(s.x - handleRadiusPx, s.y - handleRadiusPx),
						  ImVec2(s.x + handleRadiusPx, s.y + handleRadiusPx),
						  IM_COL32(255, 200, 80, 255));
	}

	const float stemMm =
		std::max(2.f, rotateStemMm / std::max(0.05f, map.view->scalePxPerMm));
	const glm::vec2 topCenter{boxR + boxW * 0.5f, boxT};
	const glm::vec2 rotMm{topCenter.x, topCenter.y - stemMm};
	const ImVec2 topS = map.toScreen(topCenter);
	const ImVec2 rotS = map.toScreen(rotMm);
	dl->AddLine(topS, rotS, IM_COL32(255, 200, 80, 200), 1.5f);
	dl->AddCircleFilled(rotS, handleRadiusPx + 1.5f, IM_COL32(255, 200, 80, 255));
	dl->AddCircle(rotS, handleRadiusPx + 1.5f, IM_COL32(40, 40, 40, 200), 0, 1.5f);

	if (active && xform.kind == SurfaceDrag::None) {
		const ImVec2 mp = ImGui::GetIO().MousePos;
		SelBounds hitB = bounds;
		const int h = hitScaleHandle(map, hitB, mp);
		if (h >= 0) {
			setScaleCursor(h);
		} else if (hitRotate(map, hitB, mp)) {
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		}
	}
}

bool SurfaceGizmo::beginClick(const SurfaceMap& map, const SelBounds& bounds, const ImVec2& mousePx,
							  const glm::vec2& mouseMm, bool shift, bool alt, bool ctrlRotate) {
	if (!bounds.valid) {
		return false;
	}
	xform = {};
	xform.startMm = mouseMm;
	xform.x0 = bounds.minX;
	xform.y0 = bounds.minY;
	xform.w0 = bounds.width();
	xform.h0 = bounds.height();
	xform.liveX = xform.x0;
	xform.liveY = xform.y0;
	xform.liveW = xform.w0;
	xform.liveH = xform.h0;
	xform.proportional = shift;
	xform.aboutCentre = alt;

	const bool wantRotate =
		hitRotate(map, bounds, mousePx) ||
		(ctrlRotate && ImGui::GetIO().KeyCtrl && hitScaleHandle(map, bounds, mousePx) >= 0);
	if (wantRotate) {
		xform.kind = SurfaceDrag::Rotate;
		xform.handle = hitScaleHandle(map, bounds, mousePx);
		const float cx = xform.x0 + xform.w0 * 0.5f;
		const float cy = xform.y0 + xform.h0 * 0.5f;
		xform.rot0 = std::atan2(mouseMm.y - cy, mouseMm.x - cx);
		xform.liveAngle = 0.f;
		return true;
	}
	const int h = hitScaleHandle(map, bounds, mousePx);
	if (h >= 0) {
		xform.kind = SurfaceDrag::Scale;
		xform.handle = h;
		return true;
	}
	return false;
}

void SurfaceGizmo::beginMove(const SelBounds& bounds, const glm::vec2& mouseMm) {
	xform = {};
	xform.kind = SurfaceDrag::Move;
	xform.startMm = mouseMm;
	xform.x0 = bounds.minX;
	xform.y0 = bounds.minY;
	xform.w0 = bounds.width();
	xform.h0 = bounds.height();
}

bool SurfaceGizmo::drag(const SurfaceMap& map, const glm::vec2& mouseMm, bool shift, bool alt,
						const SurfaceApply& apply, const std::function<void()>& restoreSnap) {
	(void)map;
	if (xform.kind == SurfaceDrag::None || !apply.applyToSelection || !restoreSnap) {
		return false;
	}
	xform.proportional = shift;
	xform.aboutCentre = alt;
	restoreSnap();

	if (xform.kind == SurfaceDrag::Move) {
		const glm::vec2 delta = mouseMm - xform.startMm;
		apply.applyToSelection(
			[&](ecs::CPaths& bag, int pi) { return pathEdit::translatePath(bag, pi, delta); });
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
		return true;
	}

	if (xform.kind == SurfaceDrag::Scale) {
		const glm::vec2 d = mouseMm - xform.startMm;
		const float r0 = xform.x0;
		const float t0 = xform.y0;
		const float w0 = xform.w0;
		const float h0 = xform.h0;
		float nx = r0, ny = t0, nw = w0, nh = h0;
		const int h = xform.handle;
		const float asp = (std::abs(h0) > 1e-6f) ? (w0 / h0) : 1.f;

		if (xform.aboutCentre) {
			float dw = 0.f, dh = 0.f;
			if (h == 0 || h == 3 || h == 7) {
				dw = -d.x;
			} else if (h == 1 || h == 2 || h == 5) {
				dw = d.x;
			}
			if (h == 0 || h == 1 || h == 4) {
				dh = -d.y;
			} else if (h == 2 || h == 3 || h == 6) {
				dh = d.y;
			}
			if (h == 4 || h == 6) {
				dw = 0.f;
			}
			if (h == 5 || h == 7) {
				dh = 0.f;
			}
			if (shift && h >= 0 && h < 4) {
				if (std::abs(dw) >= std::abs(dh)) {
					dh = ((dh < 0.f) ? -1.f : 1.f) * std::abs(dw) / asp;
				} else {
					dw = ((dw < 0.f) ? -1.f : 1.f) * std::abs(dh) * asp;
				}
			}
			nw = w0 + 2.f * dw;
			nh = h0 + 2.f * dh;
			nx = r0 + w0 * 0.5f - nw * 0.5f;
			ny = t0 + h0 * 0.5f - nh * 0.5f;
		} else if (h == 0) {
			nx += d.x;
			ny += d.y;
			nw -= d.x;
			nh -= d.y;
		} else if (h == 1) {
			ny += d.y;
			nw += d.x;
			nh -= d.y;
		} else if (h == 2) {
			nw += d.x;
			nh += d.y;
		} else if (h == 3) {
			nx += d.x;
			nw -= d.x;
			nh += d.y;
		} else if (h == 4) {
			ny += d.y;
			nh -= d.y;
		} else if (h == 5) {
			nw += d.x;
		} else if (h == 6) {
			nh += d.y;
		} else if (h == 7) {
			nx += d.x;
			nw -= d.x;
		}
		if (!xform.aboutCentre && shift && h >= 0 && h < 4) {
			if (std::abs(nw - w0) >= std::abs(nh - h0)) {
				nh = nw / asp;
			} else {
				nw = nh * asp;
			}
		}
		constexpr float kMinAbs = 0.05f;
		if (std::abs(nw) < kMinAbs) {
			nw = (nw < 0.f) ? -kMinAbs : kMinAbs;
		}
		if (std::abs(nh) < kMinAbs) {
			nh = (nh < 0.f) ? -kMinAbs : kMinAbs;
		}
		xform.liveX = nx;
		xform.liveY = ny;
		xform.liveW = nw;
		xform.liveH = nh;

		const float sx = (std::abs(w0) > 1e-6f) ? (nw / w0) : 1.f;
		const float sy = (std::abs(h0) > 1e-6f) ? (nh / h0) : 1.f;
		const glm::vec2 origin = scaleOrigin(r0, t0, w0, h0, h, xform.aboutCentre);
		apply.applyToSelection([&](ecs::CPaths& bag, int pi) {
			return pathEdit::scalePath(bag, pi, origin, sx, sy);
		});
		setScaleCursor(h);
		return true;
	}

	if (xform.kind == SurfaceDrag::Rotate) {
		const float cx = xform.x0 + xform.w0 * 0.5f;
		const float cy = xform.y0 + xform.h0 * 0.5f;
		float angle = std::atan2(mouseMm.y - cy, mouseMm.x - cx) - xform.rot0;
		if (shift) {
			constexpr float kStep = 15.f * kPi / 180.f;
			angle = std::round(angle / kStep) * kStep;
		}
		xform.liveAngle = angle;
		apply.applyToSelection([&](ecs::CPaths& bag, int pi) {
			return pathEdit::rotatePath(bag, pi, {cx, cy}, angle);
		});
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		return true;
	}
	return false;
}

void SurfaceGizmo::end() {
	xform = {};
}

} // namespace artboards
} // namespace rigkit
