#pragma once

class BaseWindow 
{
public:
	enum ViewModeType
	{
		Windowed,
		Borderless,
		Fullscreen
	};

	struct ContextSettings
	{
		struct
		{
			size_t Major = 2;
			size_t Minor = 1;
		} Version;

		size_t Samples = 1;
	};

	struct WindowSettings
	{
		size_t Width = 800;
		size_t Height = 600;
		size_t MonitorIndex = 0;
		ViewModeType ViewMode = ViewModeType::Windowed;

		ContextSettings Context;
		const char* Title = "";
	};

protected:
	BaseWindow(const WindowSettings& a_WindowSettings) :
		m_Settings(a_WindowSettings)
	{

	}

	WindowSettings m_Settings;
};