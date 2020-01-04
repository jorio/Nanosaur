#include "Qut.h"
#include "QutInternal.h"

constexpr auto kQutWindowClass = L"Qut";

HINSTANCE				gInstance    = NULL;
HDC						gDC          = NULL;

TQ3ViewObject                   gView = NULL;
void*							gWindow = NULL;
TQ3Boolean                      gWindowCanResize = kQ3True;

static void
qut_get_window_size(HWND theWnd, TQ3Area *theArea)
{	RECT		theRect;

	// Get the size of the window
	GetClientRect(theWnd, &theRect);

	theArea->min.x = (float) theRect.left;
	theArea->min.y = (float) theRect.top;
	theArea->max.x = (float) theRect.right;
	theArea->max.y = (float) theRect.bottom;
}

static LRESULT CALLBACK
qut_wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void
Qut_CreateWindow(const char		*windowTitle,
					TQ3Uns32	theWidth,
					TQ3Uns32	theHeight,
					TQ3Boolean	canResize)
{
	wchar_t wString[4096];
	MultiByteToWideChar(CP_ACP, 0, windowTitle, -1, wString, 4096);
	// Create the window
	gWindow = (void *) CreateWindow(kQutWindowClass, wString,
									WS_OVERLAPPEDWINDOW |
									WS_CLIPCHILDREN     |
									WS_CLIPSIBLINGS,
									30, 30, theWidth, theHeight,
									NULL, NULL, gInstance, NULL);

	// Save the window details
	gWindowCanResize = canResize;
}

TQ3DrawContextObject
Qut_CreateDrawContext(void)
{	TQ3Win32DCDrawContextData	winDrawContextData;
	TQ3Boolean					resetDrawContext;
	TQ3DrawContextObject		theDrawContext;
	TQ3Status					qd3dStatus;

	// Get the DC
	gDC = GetDC((HWND) gWindow);

	// See if we've got an existing draw context we can reuse. If we
	// do, we grab as much of its state data as we can - this means we
	// wil preserve any changes made by the app's view-configure method.
	resetDrawContext = kQ3True;
	if (gView != NULL)
		{
		qd3dStatus = Q3View_GetDrawContext(gView, &theDrawContext);
		if (qd3dStatus == kQ3Success)
			{
			resetDrawContext = kQ3False;
			Q3DrawContext_GetData(theDrawContext, &winDrawContextData.drawContextData);
			Q3Object_Dispose(theDrawContext);
			}
		}

	// Reset the draw context data if required
	if (resetDrawContext)
		{
		// Fill in the draw context data
		winDrawContextData.drawContextData.clearImageMethod  = kQ3ClearMethodWithColor;
		winDrawContextData.drawContextData.clearImageColor.a = 1.0f;
		winDrawContextData.drawContextData.clearImageColor.r = 1.0f;
		winDrawContextData.drawContextData.clearImageColor.g = 1.0f;
		winDrawContextData.drawContextData.clearImageColor.b = 1.0f;
		winDrawContextData.drawContextData.paneState         = kQ3False;
		winDrawContextData.drawContextData.maskState		 = kQ3False;	
		winDrawContextData.drawContextData.doubleBufferState = kQ3True;
		}

	// Reset the fields which are always updated
	qut_get_window_size((HWND) gWindow, &winDrawContextData.drawContextData.pane);
	winDrawContextData.hdc = gDC;

	// Create the draw context object
	theDrawContext = Q3Win32DCDrawContext_New(&winDrawContextData);
	return(theDrawContext);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{	MSG		theMsg;
	TQ3Status		qd3dStatus;
	WNDCLASSEX wcex;

	// Initialise ourselves
	gInstance = hInstance;

	// Register the window
	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style			= CS_OWNDC;
	wcex.lpfnWndProc	= (WNDPROC)qut_wnd_proc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= gInstance;
	wcex.hIcon			= LoadIcon(NULL, IDI_WINLOGO);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= NULL;
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= kQutWindowClass;
	wcex.hIconSm		= NULL;
	RegisterClassEx(&wcex);

	// Initialise ourselves
	qd3dStatus = Q3Initialize();
	if (qd3dStatus != kQ3Success)
		return 1;

	App_Initialise();

	if (gWindow == NULL)
		return 1;

	// Show the window and start the timer
	ShowWindow((HWND)gWindow, nCmdShow);

	// Run the app
	while (GetMessage(&theMsg, NULL, 0, 0)) {
		TranslateMessage(&theMsg);
		DispatchMessage(&theMsg);
	}

	// Clean up
	App_Terminate();
	
	if (gView != NULL)
		Q3Object_Dispose(gView);

	if (gDC != NULL)
		ReleaseDC((HWND)gWindow, gDC);

	DestroyWindow((HWND)gWindow);

	// Terminate Quesa
	qd3dStatus = Q3Exit();

	return(theMsg.wParam);
}
