/**
 * @brief Smoke: zoom-under-cursor ordering and selection bounds helpers.
 */
#include "CBedView.h"
#include "CPathEditSelection.h"
#include "CPaths.h"
#include "PathTools.h"
#include "SelectionBounds.h"
#include "SurfaceView.h"
#include "ecs/MEcs.h"

#include <cmath>
#include <cstdio>

static int fail(const char* msg) {
	std::fprintf(stderr, "surface_smoke FAIL: %s\n", msg);
	return 1;
}

int main() {
	rigkit::ecs::CBedView view;
	view.scalePxPerMm = 2.f;
	view.originPx = {10.f, 20.f};
	const glm::vec2 local{100.f, 80.f};
	const glm::vec2 mmBefore = view.pxToMm(local);

	rigkit::artboards::SurfaceView sv;
	sv.handleWheel(view, local, 1.f);
	const glm::vec2 mmAfter = view.pxToMm(local);
	if (std::abs(mmAfter.x - mmBefore.x) > 1e-3f || std::abs(mmAfter.y - mmBefore.y) > 1e-3f) {
		return fail("zoom under cursor drifted");
	}
	if (view.scalePxPerMm <= 2.f) {
		return fail("scale did not increase");
	}

	rigkit::MEcs ecs;
	auto layer = ecs.createEntity("layer");
	rigkit::ecs::CPaths bag;
	using Cmd = rigkit::ecs::CPath::Cmd;
	rigkit::ecs::CPath a;
	a.commands.push_back(rigkit::ecs::CPath::Command{Cmd::MoveTo, {0.f, 0.f}});
	a.commands.push_back(rigkit::ecs::CPath::Command{Cmd::LineTo, {10.f, 0.f}});
	a.commands.push_back(rigkit::ecs::CPath::Command{Cmd::LineTo, {10.f, 10.f}});
	bag.paths.push_back(std::move(a));
	rigkit::ecs::CPath b;
	b.commands.push_back(rigkit::ecs::CPath::Command{Cmd::MoveTo, {20.f, 20.f}});
	b.commands.push_back(rigkit::ecs::CPath::Command{Cmd::LineTo, {30.f, 30.f}});
	bag.paths.push_back(std::move(b));
	ecs.addComponent<rigkit::ecs::CPaths>(layer, bag);

	rigkit::ecs::CPathEditSelection sel;
	sel.selectOnly(0, 0);
	sel.togglePath(0, 1);
	const auto bb =
		rigkit::artboards::selectionBounds(ecs, {layer}, sel);
	if (!bb.valid || bb.minX > 0.1f || bb.maxX < 29.f || bb.maxY < 29.f) {
		return fail("selection bounds");
	}

	const float t = rigkit::artboards::segmentHitT(bag.paths[0], 1, {5.f, 0.f});
	if (t < 0.4f || t > 0.6f) {
		return fail("segmentHitT midpoint");
	}

	std::printf("surface_smoke OK\n");
	return 0;
}
