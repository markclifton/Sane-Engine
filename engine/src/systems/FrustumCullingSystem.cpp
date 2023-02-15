#include <ge/systems/FrustumCullingSystem.hpp>

#include <ge/components/Common.hpp>
#include <ge/core/Global.hpp>
#include <ge/math/FrustumCull.hpp>

namespace GE
{
	namespace Sys
	{
		void FrustumCullingSystem::Update(int64_t tsMicroseconds)
		{
			Math::Frustum frustum(_cameraData.projection * _cameraData.view);
			auto view = GlobalRegistry().view<const AABB, Visibility>();
			view.each([&](const AABB aabb, Visibility& visibility) {
				visibility = frustum.IsBoxVisible(aabb.min, aabb.max);
			});
		}
	}
}