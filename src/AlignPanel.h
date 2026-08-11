#pragma once

#include <functional>
#include <string>
#include "IWindow.h"

namespace rigkit {

class PlotDoc;

/**
 * @brief Shared Align & Distribute panel (selection or artboard target).
 */
class AlignPanel : public IWindow {
  public:
	explicit AlignPanel(const std::string& title = "Align###artboards.align");

	void setPlotDoc(PlotDoc* doc) { m_doc = doc; }
	void setMutateHook(std::function<void(const char* label, std::function<void()> body)> hook) {
		m_mutate = std::move(hook);
	}

  protected:
	void renderContents() override;

  private:
	enum class Target : int { Selection = 0, Artboard = 1 };

	PlotDoc* m_doc = nullptr;
	std::function<void(const char*, std::function<void()>)> m_mutate;
	Target m_target = Target::Selection;
	std::string m_status;
};

} // namespace rigkit
