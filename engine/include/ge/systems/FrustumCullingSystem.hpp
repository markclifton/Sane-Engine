#pragma once

#include <ge/events/CameraEvents.hpp>
#include <ge/systems/Systems.hpp>

namespace GE
{
	namespace Sys
	{
		class FrustumCullingSystem : virtual public GE::Sys::System, public GE::Camera3DSink
		{
		public:
			FrustumCullingSystem() : GE::Sys::System("FrustumCullingSystem") {}
			virtual void Update(int64_t tsMicroseconds) override;
		};
	}
}