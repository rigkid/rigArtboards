#include "rigArtboards.h"

#include "core/pack/PackRegistry.h"
#include "core/RigKitEngine.h"

#include <memory>
#include <spdlog/spdlog.h>

namespace rigkit {

rigArtboards::rigArtboards() : IPack("rigArtboards") {}

bool rigArtboards::init() {
	spdlog::info("[rigArtboards] init");
	return true;
}

void rigArtboards::setup() {
	spdlog::info("[rigArtboards] setup (helpers; no ECS registration)");
}

} // namespace rigkit

namespace {
struct rigArtboardsRegistrar {
	rigArtboardsRegistrar() {
		rigkit::PackRegistry::instance().addFactory("rigArtboards", []() {
			return std::shared_ptr<rigkit::IPack>(std::make_shared<rigkit::rigArtboards>());
		});
	}
};
static rigArtboardsRegistrar rigArtboards_auto_reg;
} // namespace
