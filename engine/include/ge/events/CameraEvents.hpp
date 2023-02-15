#pragma once

#include <glm/glm.hpp>

#include <ge/systems/Systems.hpp>

namespace GE
{
	struct Camera3DData
	{
		glm::mat4 projection;
		glm::mat4 view;
		glm::vec3 position;
		glm::vec3 front{ 0.f, 0.f, 0.f };
	};

	class Camera3DSink : virtual public Sys::System
	{
	public:
		Camera3DSink();

		void Attach();
		void Detach();

		virtual void OnCameraUpdate() {}

	protected:
		Camera3DData _cameraData;

	private:
		void UpdateCameraData(const Camera3DData& cameraData);
	};
}