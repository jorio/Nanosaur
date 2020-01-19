#include "Qut.h"
#include "QutInternal.h"

#include <SDL.h>
#include <iostream>

//HINSTANCE				gInstance    = NULL;
//HDC						gDC          = NULL;

TQ3ViewObject                   gView = NULL;
//void*							gWindow = NULL;
TQ3Boolean                      gWindowCanResize = kQ3True;
SDL_Window* gSDLWindow = nullptr;
SDL_GLContext					gGLCtx = nullptr;

TQ3DrawContextData gDCD;
TQ3DrawContextObject gDCO;


#if 0
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
	// don't bring up accelerator menu when pressing alt
	if (wParam == SC_KEYMENU && (lParam >> 16) <= 0) return 0;

	switch (message) {
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
#endif

void
Qut_CreateWindow(const char		*windowTitle,
					TQ3Uns32	theWidth,
					TQ3Uns32	theHeight,
					TQ3Boolean	canResize)
{
#if 1
	TODO2("actually create sdl window here");
#else
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
#endif
}


TQ3DrawContextObject
Qut_CreateDrawContext(void)
{//	TQ3Win32DCDrawContextData	winDrawContextData;
TQ3Boolean					resetDrawContext = kQ3True;
	TQ3DrawContextObject		theDrawContext;
	TQ3Status					qd3dStatus;

	// Get the DC
//	gDC = GetDC((HWND) gWindow);

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
//			Q3DrawContext_GetData(theDrawContext, &winDrawContextData.drawContextData);
			Q3Object_Dispose(theDrawContext);
			}
		}

	TQ3DrawContextData& drawContextData = gDCD;

	// Reset the draw context data if required
	if (resetDrawContext)
		{
		// Fill in the draw context data
		drawContextData.clearImageMethod  = kQ3ClearMethodWithColor;
		drawContextData.clearImageColor.a = 1.0f;
		drawContextData.clearImageColor.r = 1.0f;
		drawContextData.clearImageColor.g = 1.0f;
		drawContextData.clearImageColor.b = 1.0f;
		drawContextData.paneState         = kQ3False;
		drawContextData.maskState		 = kQ3False;	
		drawContextData.doubleBufferState = kQ3True;
		}

	// Reset the fields which are always updated
	drawContextData.pane.min.x = 0;
	drawContextData.pane.min.y = 0;
	drawContextData.pane.max.x = 640;
	drawContextData.pane.max.x = 480;
//	qut_get_window_size((HWND) gWindow, &winDrawContextData.drawContextData.pane);
//	winDrawContextData.hdc = gDC;

	// Create the draw context object
//	theDrawContext = new E3SdlGlDrawContext;
//	theDrawContext = Q3Win32DCDrawContext_New(&winDrawContextData);
	theDrawContext = Q3SDLDrawContext_New();
	return(theDrawContext);
	//return (TQ3DrawContextObject)gGLCtx;
}


#define SDL_ENSURE(X) { \
	if (!(X)) { \
		std::cerr << #X << " --- " << SDL_GetError() << "\n"; \
		exit(1); \
	} \
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{	MSG		theMsg;
	TQ3Status		qd3dStatus;
	


	SDL_ENSURE(0 == SDL_Init(SDL_INIT_VIDEO));

	gSDLWindow = SDL_CreateWindow("SDLNano",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		640,
		480,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN);

	SDL_ENSURE(gSDLWindow);

	SDL_ENSURE(gGLCtx = SDL_GL_CreateContext(gSDLWindow));




	// Initialise ourselves
	qd3dStatus = Q3Initialize();
	if (qd3dStatus != kQ3Success)
		return 1;

	App_Initialise();

#if 0
	if (gWindow == NULL)
		return 1;

	// Show the window and start the timer
	ShowWindow((HWND)gWindow, nCmdShow);

	// Run the app
	while (GetMessage(&theMsg, NULL, 0, 0)) {
		TranslateMessage(&theMsg);
		DispatchMessage(&theMsg);
	}
#endif

	while (true) {
		// ...
	}

	// Clean up
	App_Terminate();
	
	if (gView != NULL)
		Q3Object_Dispose(gView);

//	if (gDC != NULL)
//		ReleaseDC((HWND)gWindow, gDC);

//	DestroyWindow((HWND)gWindow);

	// Terminate Quesa
	qd3dStatus = Q3Exit();

	return 0;//return(theMsg.wParam);
}
