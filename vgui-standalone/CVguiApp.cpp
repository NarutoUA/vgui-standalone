#include <Windows.h>

#include "CVguiApp.h"


#include <vgui/ILocalize.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include <vgui/isystem.h>

#include <vgui_controls/MessageBox.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/Panel.h>

#include "filesystem.h"
#include "appframework/tier3app.h"
#include "tier0/icommandline.h"
#include "inputsystem/iinputsystem.h"

#include <io.h>
#include <stdio.h>


// Since windows redefines MessageBox.
typedef vgui::MessageBox vguiMessageBox;

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------
// Spew func
//-----------------------------------------------------------------------------
SpewRetval_t VguiSpewFunc(SpewType_t spewType, char const *pMsg)
{
#ifdef _WIN32
	OutputDebugString(pMsg);
#endif

	if (spewType == SPEW_ERROR)
	{
		// In Windows vgui mode, make a message box or they won't ever see the error.
#ifdef _WIN32
		MessageBoxA(NULL, pMsg, "Error", MB_OK | MB_TASKMODAL);
		TerminateProcess(GetCurrentProcess(), 1);
#elif _LINUX
		_exit(1);
#else
#error "Implement me"
#endif

		return SPEW_ABORT;
	}
	if (spewType == SPEW_ASSERT)
	{
		if (CommandLine()->FindParm("-noassert") == 0)
			return SPEW_DEBUGGER;
		else
			return SPEW_CONTINUE;
	}
	return SPEW_CONTINUE;
}


//-----------------------------------------------------------------------------
// The application object
//-----------------------------------------------------------------------------
bool CVguiApp::Create()
{
	AppSystemInfo_t appSystems[] =
	{
		//{ "inputsystem.dll",		INPUTSYSTEM_INTERFACE_VERSION },
		{ "inputsystem.dll",		INPUTSYSTEM_INTERFACE_VERSION },
		{ "filesystem_stdio.dll",	FILESYSTEM_INTERFACE_VERSION },
		{ "vgui2.dll",				VGUI_IVGUI_INTERFACE_VERSION },
		{ "vstdlib.dll",			CVAR_INTERFACE_VERSION },
		{ "", "" }	// Required to terminate the list
	};

	return AddSystems(appSystems);
}


//-----------------------------------------------------------------------------
// Purpose: Entry point
//-----------------------------------------------------------------------------
bool CVguiApp::PreInit()
{

	SpewOutputFunc(VguiSpewFunc);

	if (!BaseClass::PreInit())
		return false;

	return true;
}

void CVguiApp::PostShutdown()
{
	BaseClass::PostShutdown();
}

int CVguiApp::Main()
{

	// Init the surface
	vgui::Panel *pPanel = new vgui::Panel(NULL, "TopPanel");
	pPanel->SetVisible(true);

	vgui::surface()->SetEmbeddedPanel(pPanel->GetVPanel());

	VguiStartup();

	// start vgui
	g_pVGui->Start();

	// run app frame loop
	vgui::VPANEL root = g_pVGuiSurface->GetEmbeddedPanel();
	g_pVGuiSurface->Invalidate(root);

	while (g_pVGui->IsRunning() && !ShouldShutdown())
	{
		//AppPumpMessages();
		g_pInputSystem->PollInputState();

		// do begin fame code here

		g_pVGui->RunFrame();
		g_pVGuiSurface->PaintTraverseEx(root, true);

		// place ending frame code here
	}

	VguiShutdown();

	g_pVGui->Shutdown();

	return 1;
}