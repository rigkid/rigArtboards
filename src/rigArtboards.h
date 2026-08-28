#pragma once

#include "core/pack/IPack.h"
namespace rigkit {

/**
 * @brief Shared millimetre editing surface - view, gizmo, numeric transform.
 * @details CODE ONLY. Artboard PODs live in **rigPlotComponent**.
 */
class rigArtboards : public IPack {
  public:
	rigArtboards();
	bool init() override;
	void setup() override;
};

} // namespace rigkit
