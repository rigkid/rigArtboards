#pragma once

#include <functional>
#include <string>
#include "IWindow.h"

namespace rigkit {

class PlotDoc;

/**
 * @brief Shared Pathfinder (Shape Modes + Pathfinders) over PlotDoc selection.
 * @details Shells wire PlotDoc + an optional undo mutate hook. Multi-layer
 * selections copy foreign paths onto the primary layer before the boolean.
 */
class PathfinderPanel : public IWindow {
  public:
	explicit PathfinderPanel(const std::string& title = "Pathfinder###artboards.pathfinder");

	void setPlotDoc(PlotDoc* doc) { m_doc = doc; }

	/**
	 * @brief Optional undo wrapper: label + body. When unset, body runs bare.
	 * @details Svg shell: mutateDocGeometry. Plotter: dirty + optional snapshot.
	 */
	void setMutateHook(std::function<void(const char* label, std::function<void()> body)> hook) {
		m_mutate = std::move(hook);
	}

  protected:
	void renderContents() override;

  private:
	PlotDoc* m_doc = nullptr;
	std::function<void(const char*, std::function<void()>)> m_mutate;
	std::string m_status;
};

} // namespace rigkit
