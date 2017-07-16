#pragma once

#include "appframework/tier3app.h"


//-----------------------------------------------------------------------------
// The application object
//-----------------------------------------------------------------------------
class CVguiApp : public CVguiSteamApp
{
	typedef CVguiSteamApp BaseClass;

public:
	CVguiApp() {};

	// Methods of IApplication
	virtual bool Create();
	virtual bool PreInit();
	virtual void PostShutdown();
	virtual int Main();

	virtual void VguiStartup() = 0;
	virtual void VguiShutdown() = 0;
	virtual bool ShouldShutdown() = 0;

private:

	// Dummy window
	WNDCLASS staticWndclass = { NULL };
	ATOM staticWndclassAtom = 0;
	HWND staticHwnd = 0;
};