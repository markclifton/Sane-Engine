#include <ge/systems/Camera3D.hpp>

#include <glm/gtx/transform.hpp>

#include <ge/core/Global.hpp>

namespace GE
{
	namespace Sys
	{
		Camera3D::Camera3D() 
			: GE::Sys::System("Camera3D") 
		{
			_currentCameraData.projection = glm::perspective(45.f, 16.f / 9.f, .1f, 10000.f);
			_currentCameraData.position = glm::vec3(100.f, -140.f, 0.f);
			_currentCameraData.view = glm::mat4(1.f);

			REGISTER_SYSTEM();
		}

		void Camera3D::Attach()
		{
			GE::GlobalDispatcher().sink<GE::Sys::ControllerState>().connect<&Camera3D::ControllerUpdate>(this);
		}

		void Camera3D::Detach()
		{
			GE::GlobalDispatcher().sink<GE::Sys::ControllerState>().disconnect<&Camera3D::ControllerUpdate>(this);
		}

		void Camera3D::ControllerUpdate(GE::Sys::ControllerState state) 
		{ 
			currentState = state; 
		}

		void Camera3D::Update(int64_t tsMicroseconds)
		{
			float x = 0, y = 0, z = 0;
			float tsSeconds = (tsMicroseconds / 1000.f);
			float rate = velocity * tsSeconds;
			float rotateRate = rotationVelocity * tsSeconds;

			if (GE::Sys::IsButtonPressed(currentState, GE::Controller::Up))
				z += rate;
			if (GE::Sys::IsButtonPressed(currentState, GE::Controller::Down))
				z -= rate;
			if (GE::Sys::IsButtonPressed(currentState, GE::Controller::Right))
				x += rate;
			if (GE::Sys::IsButtonPressed(currentState, GE::Controller::Left))
				x -= rate;
			if (GE::Sys::IsButtonPressed(currentState, GE::Controller::A))
				y -= rate;
			if (GE::Sys::IsButtonPressed(currentState, GE::Controller::B))
				y += rate;

			if (GE::Sys::IsButtonPressed(currentState, GE::Controller::ZR))
				rotation.y -= 20 * rotateRate * glm::pi<float>() / 180.f;
			if (GE::Sys::IsButtonPressed(currentState, GE::Controller::ZL))
				rotation.y += 20 * rotateRate * glm::pi<float>() / 180.f;

			_currentCameraData.front.x = cos(glm::radians(rotation.y)) * cos(glm::radians(rotation.z));
			_currentCameraData.front.y = sin(glm::radians(rotation.z));
			_currentCameraData.front.z = sin(glm::radians(rotation.y)) * cos(glm::radians(rotation.z));
			_currentCameraData.front = glm::normalize(_currentCameraData.front);

			glm::vec3 up = glm::normalize(glm::cross(glm::normalize(glm::cross(_currentCameraData.front, glm::vec3(0.0f, -1.0f, 0.0f))), _currentCameraData.front));

			glm::vec3 ffront = _currentCameraData.front;
			ffront.y = 0.f;
			ffront = glm::normalize(ffront);

			_currentCameraData.position += z * ffront;
			_currentCameraData.position += x * glm::normalize(glm::cross(ffront, up));
			_currentCameraData.position.y += y;

			_currentCameraData.view = glm::lookAt(_currentCameraData.position, _currentCameraData.position + _currentCameraData.front, up) * glm::rotate(glm::mat4(1.f), glm::pi<float>(), { 0,0,1 });

			GE::GlobalDispatcher().trigger(_currentCameraData);
		}
	}
}