#pragma once

#include "core/U_core.h"
#include "packs/rigArtboards/src/SurfaceGizmo.h"
#include "packs/rigArtboards/src/SurfaceNumeric.h"
#include "packs/rigArtboards/src/SurfaceView.h"
#include "packs/rigImGui/src/IWindow.h"
#include "packs/rigPlotComponent/src/CArtboard.h"
#include "packs/rigPlotComponent/src/CBedView.h"
#include "packs/rigPlotComponent/src/CPathEditSelection.h"
#include "packs/rigPlotComponent/src/CPaths.h"

class SurfaceHeroApp : public rigkit::IApp {
  public:
	class SurfacePanel : public rigkit::IWindow {
	  public:
		SurfacePanel(rigkit::ecs::CBedView* view, rigkit::ecs::CArtboard* board,
					 rigkit::ecs::CPaths* bag, rigkit::ecs::CPathEditSelection* sel);
		void renderContents() override;

	  private:
		rigkit::ecs::CBedView* m_view = nullptr;
		rigkit::ecs::CArtboard* m_board = nullptr;
		rigkit::ecs::CPaths* m_bag = nullptr;
		rigkit::ecs::CPathEditSelection* m_sel = nullptr;
		rigkit::artboards::SurfaceView m_surface;
		rigkit::artboards::SurfaceGizmo m_gizmo;
		rigkit::artboards::SurfaceNumeric m_numeric;
		rigkit::ecs::CPaths m_snap;
		bool m_haveSnap = false;
		bool m_marquee = false;
		glm::vec2 m_marqueeA{0.f};
		glm::vec2 m_marqueeB{0.f};
		float m_fitW = 0.f;
		float m_fitH = 0.f;
	};

	SurfaceHeroApp();
	void setup() override;
	void update(float) override {}
	void draw() override {}

  private:
	rigkit::ecs::CBedView m_view;
	rigkit::ecs::CArtboard m_board;
	rigkit::ecs::CPaths m_bag;
	rigkit::ecs::CPathEditSelection m_sel;
};
