#include <ge/events/CameraEvents.hpp>

#include <ge/core/Global.hpp>

namespace GE
{
	Camera3DSink::Camera3DSink()
		: Sys::System("CameraSink")
	{
		REGISTER_SYSTEM();
	}

	void Camera3DSink::Attach()
	{
		GE::GlobalDispatcher().sink<Camera3DData>().connect<&Camera3DSink::UpdateCameraData>(this);
	}

	void Camera3DSink::Detach()
	{
		GE::GlobalDispatcher().sink<Camera3DData>().connect<&Camera3DSink::UpdateCameraData>(this);
	}

	void Camera3DSink::UpdateCameraData(const Camera3DData& cameraData)
	{
		_cameraData = cameraData;
		OnCameraUpdate();
	}
}