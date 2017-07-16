// base file for standalone vgui application

#include <windows.h>
#include "vstdlib/cvar.h"
#include "vgui/IVGui.h"
#include "vgui_controls/Panel.h"
#include "vgui_controls/Frame.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/consoledialog.h"
#include "VGuiMatSurface\IMatSystemSurface.h"

#include "CVguiApp.h"


// TODO: make these linker includes instead of pragma comment libs
#pragma comment(lib, "../sourcesdk/lib/public/vstdlib.lib")
#pragma comment(lib, "../sourcesdk/lib/public/tier0.lib")
#pragma comment(lib, "../sourcesdk/lib/public/tier1.lib")
#pragma comment(lib, "../sourcesdk/lib/public/tier2.lib")
#pragma comment(lib, "../sourcesdk/lib/public/tier3.lib")
#pragma comment(lib, "../sourcesdk/lib/public/vgui_controls.lib")
#pragma comment(lib, "../sourcesdk/lib/public/mathlib.lib")
#pragma comment(lib, "../sourcesdk/lib/public/appframework.lib")
#pragma comment(lib, "legacy_stdio_definitions.lib")

using namespace vgui;

CON_COMMAND(help, "lists the help of a concommand")
{
	if (ConCommandBase *pCommand = g_pCVar->FindCommandBase(args.Arg(1)))
	{
		g_pCVar->ConsolePrintf("%s\n", pCommand->GetHelpText());
	}
}


CON_COMMAND(list, "lists all convars / concommands")
{
	for (ConCommandBase *pBase = g_pCVar->GetCommands(); pBase != NULL; pBase = pBase->GetNext())
	{
		g_pCVar->ConsolePrintf("%s\n", pBase->GetName());
	}
}

//-----------------------------------------------------------------------------
// Spew func
//-----------------------------------------------------------------------------
SpewRetval_t ConsoleSpewFunc(SpewType_t spewType, const tchar *pMsg)
{
	OutputDebugString(pMsg);
	switch (spewType)
	{
	case SPEW_ASSERT:
		g_pCVar->ConsoleColorPrintf(Color(255, 192, 0, 255), pMsg);
#ifdef _DEBUG
		return SPEW_DEBUGGER;
#else
		return SPEW_CONTINUE;
#endif

	case SPEW_ERROR:
		g_pCVar->ConsoleColorPrintf(Color(255, 0, 0, 255), pMsg);
		return SPEW_ABORT;

	case SPEW_WARNING:
		g_pCVar->ConsoleColorPrintf(Color(192, 192, 0, 255), pMsg);
		break;

	case SPEW_MESSAGE:
	{
		const Color *c = GetSpewOutputColor();
		if (!Q_stricmp(GetSpewOutputGroup(), "developer"))
			g_pCVar->ConsoleDPrintf(pMsg);
		else
			g_pCVar->ConsoleColorPrintf(*c, pMsg);
	}
	break;
	}

	return SPEW_CONTINUE;
}

class CStandaloneVguiApp : public CVguiApp
{
	typedef CVguiApp BaseClass;

public:
	// Methods of IApplication
	virtual bool Create();
	virtual bool PreInit();
	virtual void Destroy();

	virtual void VguiStartup();
	virtual void VguiShutdown();

	virtual bool ShouldShutdown();

private:
	virtual const char *GetAppName() { return "StandaloneVguiApp"; }

	void CreateMainPanel();

	vgui::Frame *pMainPanel;
};

DEFINE_WINDOWED_APPLICATION_OBJECT(CStandaloneVguiApp);


//-----------------------------------------------------------------------------
// Create all singleton systems
//-----------------------------------------------------------------------------
bool CStandaloneVguiApp::Create()
{
	InitDefaultFileSystem();
	Setup(g_pFullFileSystem, NULL);

	if (!BaseClass::Create())
		return false;

	// add the interfaces that you need here

	AppSystemInfo_t appSystems[] =
	{
		{ "", "" }	// Required to terminate the list
	};

	return AddSystems(appSystems);
}

//-----------------------------------------------------------------------------
// Init, shutdown
//-----------------------------------------------------------------------------
bool CStandaloneVguiApp::PreInit()
{
	MathLib_Init(2.2f, 2.2f, 0.0f, 2.0f, false, false, false, false);

	if (!BaseClass::PreInit())
		return false;

	// initialize interfaces
	CreateInterfaceFn appFactory = GetFactory();

	if (!g_pFullFileSystem || !g_pVGui || !g_pVGuiSurface /*|| !g_pMatSystemSurface*/)
	{
		Warning("VGUI app is missing a required interface.\n");
		return false;
	}

	return true;
}

// vgui frame
class CStandaloneVguiFrame : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CStandaloneVguiFrame, vgui::Frame);
public:
	CStandaloneVguiFrame() : BaseClass(NULL, "StandaloneVguiFrame"), closed(false)
	{
		SetTitle("Standalone vgui app", true);
		SetSizeable(true);
		SetCloseButtonVisible(true);
		SetMoveable(true);
		Activate();

		SetSize(200, 200);
		SetMinimumSize(200, 250);

		SetKeyBoardInputEnabled(true);

		m_pConsole = new vgui::CConsoleDialog(this, "ConsoleDialog", false);
		m_pConsole->AddActionSignalTarget(this);

		m_pOpenConsole = new vgui::Button(this, "OpenConsoleButton", "Toggle Console");
		m_pOpenConsole->SetCommand("openconsole");

		LoadControlSettings("StandaloneVguiApp.res");

	}

	virtual ~CStandaloneVguiFrame()
	{
	}

	virtual void Close()
	{
		closed = true;
		return BaseClass::Close();
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();

		//m_pOpenConsole->SetPos(28, 28);
		//m_pOpenConsole->SetSize(100, 25);
	}

	virtual void OnKeyCodePressed(vgui::KeyCode code)
	{
		BaseClass::OnKeyCodePressed(code);
		if (code == BUTTON_CODE_NONE)
		{
			ToggleConsole();
		}
	}

	virtual void OnCommand(const char *command)
	{
		if (!Q_strcmp(command, "openconsole"))
		{
			ToggleConsole();
		}
		else
		{
			BaseClass::OnCommand(command);
		}
	}

	MESSAGE_FUNC_CHARPTR(OnCommandSubmitted, "CommandSubmitted", command)
	{
		CCommand args;
		args.Tokenize(command);

		ConCommandBase *pCommandBase = g_pCVar->FindCommandBase(args[0]);
		if (!pCommandBase)
		{
			Warning("Unknown command %s\n", args[0]);
			//ConWarning("Unknown command or convar '%s'!\n", args[0]);
			return;
		}

		if (pCommandBase->IsCommand())
		{
			ConCommand *pCommand = static_cast<ConCommand*>(pCommandBase);
			pCommand->Dispatch(args);
			return;
		}

		ConVar *pConVar = static_cast< ConVar* >(pCommandBase);
		if (args.ArgC() == 1)
		{
			if (pConVar->IsFlagSet(FCVAR_NEVER_AS_STRING))
			{
				ConMsg("%s = %f\n", args[0], pConVar->GetFloat());
			}
			else
			{
				ConMsg("%s = %s\n", args[0], pConVar->GetString());
			}
			return;
		}

		if (pConVar->IsFlagSet(FCVAR_NEVER_AS_STRING))
		{
			pConVar->SetValue((float)atof(args[1]));
		}
		else
		{
			pConVar->SetValue(args.ArgS());
		}
	}

	void ToggleConsole()
	{
		if (!m_pConsole->IsVisible())
		{
			m_pConsole->Activate();
			m_pConsole->MoveToCenterOfScreen();

		}
		else
		{
			m_pConsole->Hide();
		}
	}


	bool closed;

private:
	vgui::CConsoleDialog *m_pConsole;
	vgui::Button *m_pOpenConsole;
	bool m_bPositioned;
};

void CStandaloneVguiApp::CreateMainPanel()
{
	pMainPanel = new CStandaloneVguiFrame();
	pMainPanel->SetParent(g_pVGuiSurface->GetEmbeddedPanel());

	pMainPanel->MoveToCenterOfScreen();
	pMainPanel->Activate();

	return;
}

void CStandaloneVguiApp::Destroy()
{
	return;
}

void CStandaloneVguiApp::VguiStartup()
{
	vgui::scheme()->LoadSchemeFromFile("sdklauncher_scheme.res", NULL);

	CreateMainPanel();

	SpewOutputFunc(ConsoleSpewFunc);
}

void CStandaloneVguiApp::VguiShutdown()
{
	if (pMainPanel)
		delete pMainPanel;
}

bool CStandaloneVguiApp::ShouldShutdown()
{
	if (CStandaloneVguiFrame *frame = (CStandaloneVguiFrame *) pMainPanel)
	{
		return frame->closed;
	}
	return true;
}
