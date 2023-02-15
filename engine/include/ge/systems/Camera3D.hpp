#pragma once

#include <glm/glm.hpp>

#include <ge/events/CameraEvents.hpp>
#include <ge/systems/InputSystem.hpp>
#include <ge/systems/Systems.hpp>

namespace GE
{
	namespace Sys
	{
		class Camera3D : public GE::Sys::System
		{
		public:
			Camera3D();

			void Attach();
			void Detach();

			void ControllerUpdate(GE::Sys::ControllerState state);
			virtual void Update(int64_t tsMicroseconds) override;

		private:
			glm::vec3 rotation{ 0 };
			glm::vec3 translate{ 0 };
			ControllerState currentState{};

			Camera3DData _currentCameraData;

			float velocity{ .5f };
			float rotationVelocity{ 1.25f };
		};
	}
}