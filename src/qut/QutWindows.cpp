/*  NAME:
        QutWindows.c

    DESCRIPTION:
        Quesa Utility Toolkit - Windows.

    COPYRIGHT:
        Copyright (c) 1999-2011, Quesa Developers. All rights reserved.

        For the current release of Quesa, please see:

            <http://www.quesa.org/>
        
        Redistribution and use in source and binary forms, with or without
        modification, are permitted provided that the following conditions
        are met:
        
            o Redistributions of source code must retain the above copyright
              notice, this list of conditions and the following disclaimer.
        
            o Redistributions in binary form must reproduce the above
              copyright notice, this list of conditions and the following
              disclaimer in the documentation and/or other materials provided
              with the distribution.
        
            o Neither the name of Quesa nor the names of its contributors
              may be used to endorse or promote products derived from this
              software without specific prior written permission.
        
        THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
        "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
        LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
        A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
        OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
        SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
        TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
        PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
        LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
        NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
        SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    ___________________________________________________________________________
*/
//=============================================================================
//      Include files
//-----------------------------------------------------------------------------
#include "Qut.h"
#include "QutInternal.h"

#include "QuesaStorage.h"

#include "QutWindowsResource.h"





//=============================================================================
//      Internal constants
//-----------------------------------------------------------------------------
#define kQutWindowClass									"Qut"
#define kQutTimer										2

#define kMenuRenderer									1
#define kMenuStyle										2
#define kMenuSpecial									3

#define kMenuRendererOffset								600
#define kMenuSpecialOffset								700





//=============================================================================
//      Internal globals
//-----------------------------------------------------------------------------
HINSTANCE				gInstance    = NULL;
HACCEL					gAccelTable  = NULL;
HDC						gDC          = NULL;
UINT                    gTimer       = 0;
BOOL					gMouseDown   = FALSE;
BOOL					gSpecialMenu = FALSE;
TQ3Point2D				gLastMouse   = {0.0f, 0.0f};





//=============================================================================
//		Internal functions.
//-----------------------------------------------------------------------------
//		qut_build_renderer_menu : Build the renderer menu.
//-----------------------------------------------------------------------------
static void
qut_build_renderer_menu(void)
{	TCHAR				theName[1024];
	TQ3SubClassData		rendererData;
	TQ3Status			qd3dStatus;
	HMENU				theMenu;
	TQ3Uns32			n, m;



	// Get the renderer menu
	theMenu = GetMenu((HWND)gWindow);
	theMenu = GetSubMenu(theMenu, kMenuRenderer);
	if (theMenu == NULL)
		return;



	// Collect the renderers which are available
	qd3dStatus = Q3ObjectHierarchy_GetSubClassData(kQ3SharedTypeRenderer, &rendererData);
	if (qd3dStatus != kQ3Success)
		return;



	// If we can find any renderers, add them to the menu
	if (rendererData.numClasses != 0)
		{
		// First slot is a dummy
		gRenderers[0] = kQ3ObjectTypeInvalid;
		m = 1;
		
		
		// Fill the remaining slots
		for (n = 0; n < rendererData.numClasses; n++)
			{
			// Skip the generic renderer, since it can't actually render
			if (rendererData.classTypes[n] != kQ3RendererTypeGeneric)
				{
				// Grab the nick name, falling back to the class name if that fails
				qd3dStatus = Q3RendererClass_GetNickNameString(rendererData.classTypes[n], theName);
				if (qd3dStatus == kQ3Failure || theName[0] == 0x00)
					qd3dStatus = Q3ObjectHierarchy_GetStringFromType(rendererData.classTypes[n], theName);


				// Add the menu item and save the type
				if (qd3dStatus == kQ3Success)
					{
					AppendMenu(theMenu, MF_STRING, kMenuRendererOffset + m, theName);
					gRenderers[m++] = rendererData.classTypes[n];
					}
				}
			}
		}



	// Clean up
	Q3ObjectHierarchy_EmptySubClassData(&rendererData);
}





//=============================================================================
//      qut_get_window_size : Get the size of a window.
//-----------------------------------------------------------------------------
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





//=============================================================================
//      qut_about_proc : About box message handler.
//-----------------------------------------------------------------------------
static LRESULT CALLBACK
qut_about_proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{


	// Handle the message
	switch (message) {
		case WM_INITDIALOG:
			return(TRUE);

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
				{
				EndDialog(hDlg, LOWORD(wParam));
				return(TRUE);
				}
			break;
		}

    return(FALSE);
}


static void qut_update_fps_display()
{
	char	theFPS[100];
	sprintf_s( theFPS, "FPS: %.2f", gFPS );
	SetWindowText( (HWND)gWindow, theFPS );
}




//=============================================================================
//      qut_wnd_proc : Window message handler.
//-----------------------------------------------------------------------------
static LRESULT CALLBACK
qut_wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{	TQ3Point2D				mouseDiff, theMouse;
	int						wmId, wmEvent;
	TQ3Area					thePane;
	PAINTSTRUCT				ps;



	// Handle the message
	switch (message) {
		case WM_COMMAND:
			// Handle menu selections
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 

			switch (wmId) {
				case IDM_ABOUT:
				   DialogBox(gInstance, (LPCTSTR) IDD_ABOUTBOX, hWnd, (DLGPROC) qut_about_proc);
				   break;

				case IDM_EXIT:
				   PostQuitMessage(0);
				   break;

				case IDM_STYLE_SHADER_NULL:
					Qut_InvokeStyleCommand(kStyleCmdShaderNull);
					break;
				case IDM_STYLE_SHADER_LAMBERT:
					Qut_InvokeStyleCommand(kStyleCmdShaderLambert);
					break;
				case IDM_STYLE_SHADER_PHONG:
					Qut_InvokeStyleCommand(kStyleCmdShaderPhong);
					break;
				case IDM_STYLE_FILL_FILLED:
					Qut_InvokeStyleCommand(kStyleCmdFillFilled);
					break;
				case IDM_STYLE_FILL_EDGES:
					Qut_InvokeStyleCommand(kStyleCmdFillEdges);
					break;
				case IDM_STYLE_FILL_POINTS:
					Qut_InvokeStyleCommand(kStyleCmdFillPoints);
					break;
				case IDM_STYLE_BACKFACING_BOTH:
					Qut_InvokeStyleCommand(kStyleCmdBackfacingBoth);
					break;
				case IDM_STYLE_BACKFACING_REMOVE:
					Qut_InvokeStyleCommand(kStyleCmdBackfacingRemove);
					break;
				case IDM_STYLE_BACKFACING_FLIP:
					Qut_InvokeStyleCommand(kStyleCmdBackfacingFlip);
					break;
				case IDM_STYLE_INTERPOLATION_NONE:
					Qut_InvokeStyleCommand(kStyleCmdInterpolationNone);
					break;
				case IDM_STYLE_INTERPOLATION_VERTEX:
					Qut_InvokeStyleCommand(kStyleCmdInterpolationVertex);
					break;
				case IDM_STYLE_INTERPOLATION_PIXEL:
					Qut_InvokeStyleCommand(kStyleCmdInterpolationPixel);
					break;
				case IDM_STYLE_ORIENTATION_CLOCKWISE:
					Qut_InvokeStyleCommand(kStyleCmdOrientationClockwise);
					break;
				case IDM_STYLE_ORIENTATION_COUNTER_CLOCKWISE:
					Qut_InvokeStyleCommand(kStyleCmdOrientationCounterClockwise);
					break;
				case IDM_STYLE_ANTIALIAS_NONE:
					Qut_InvokeStyleCommand(kStyleCmdAntiAliasNone);
					break;
				case IDM_STYLE_ANTIALIAS_EDGES:
					Qut_InvokeStyleCommand(kStyleCmdAntiAliasEdges);
					break;
				case IDM_STYLE_ANTIALIAS_FILLED:
					Qut_InvokeStyleCommand(kStyleCmdAntiAliasFilled);
					break;
				case IDM_STYLE_FOG_ON:
					Qut_InvokeStyleCommand(kStyleCmdFogOn);
					break;
				case IDM_STYLE_FOG_OFF:
					Qut_InvokeStyleCommand(kStyleCmdFogOff);
					break;
				case IDM_STYLE_SUBDIVISION_CONSTANT1:
					Qut_InvokeStyleCommand(kStyleCmdSubdivisionConstant1);
					break;
				case IDM_STYLE_SUBDIVISION_CONSTANT2:
					Qut_InvokeStyleCommand(kStyleCmdSubdivisionConstant2);
					break;
				case IDM_STYLE_SUBDIVISION_CONSTANT3:
					Qut_InvokeStyleCommand(kStyleCmdSubdivisionConstant3);
					break;
				case IDM_STYLE_SUBDIVISION_CONSTANT4:
					Qut_InvokeStyleCommand(kStyleCmdSubdivisionConstant4);
					break;
				case IDM_STYLE_SUBDIVISION_WORLDSPACE1:
					Qut_InvokeStyleCommand(kStyleCmdSubdivisionWorldSpace1);
					break;
				case IDM_STYLE_SUBDIVISION_WORLDSPACE2:
					Qut_InvokeStyleCommand(kStyleCmdSubdivisionWorldSpace2);
					break;
				case IDM_STYLE_SUBDIVISION_WORLDSPACE3:
					Qut_InvokeStyleCommand(kStyleCmdSubdivisionWorldSpace3);
					break;
				case IDM_STYLE_SUBDIVISION_SCREENSPACE1:
					Qut_InvokeStyleCommand(kStyleCmdSubdivisionScreenSpace1);
					break;
				case IDM_STYLE_SUBDIVISION_SCREENSPACE2:
					Qut_InvokeStyleCommand(kStyleCmdSubdivisionScreenSpace2);
					break;
				case IDM_STYLE_SUBDIVISION_SCREENSPACE3:
					Qut_InvokeStyleCommand(kStyleCmdSubdivisionScreenSpace3);
					break;

				default:
					if (wmId >= kMenuSpecialOffset)
						{
						// If the app has a callback, call it
						if (gAppMenuSelect != NULL)
							gAppMenuSelect(gView, wmId - kMenuSpecialOffset - 1);
						}

					else if (wmId >= kMenuRendererOffset)
						Q3View_SetRendererByType(gView, gRenderers[wmId - kMenuRendererOffset]);

					else
						return(DefWindowProc(hWnd, message, wParam, lParam));
				}
			break;


		case WM_PAINT:
			BeginPaint(hWnd, &ps);
			Qut_RenderFrame();
			EndPaint(hWnd, &ps);
			break;


		case WM_CLOSE:
			PostQuitMessage(0);
			break;


		case WM_TIMER:
			qut_update_fps_display();
			Qut_RenderFrame();
			break;


		case WM_LBUTTONDOWN:
			// Grab the mouse
			gLastMouse.x = (float) LOWORD(lParam);
			gLastMouse.y = (float) HIWORD(lParam);
			gMouseDown   = TRUE;

			// If we have a mouse down callback, call it
			if (gFuncAppMouseDown != NULL)
				gFuncAppMouseDown(gView, gLastMouse);
			break;


		case WM_LBUTTONUP:
			gMouseDown = FALSE;
			break;


		case WM_MOUSEMOVE:
			if (gFuncAppMouseTrack != NULL && gMouseDown)
				{
				theMouse.x = (float) LOWORD(lParam);
				theMouse.y = (float) HIWORD(lParam);
			
				mouseDiff.x = theMouse.x - gLastMouse.x;
				mouseDiff.y = theMouse.y - gLastMouse.y;

				gFuncAppMouseTrack(gView, mouseDiff);
				Qut_RenderFrame();

				gLastMouse = theMouse;
				}
			break;


		default:
			return(DefWindowProc(hWnd, message, wParam, lParam));
	   }

	return(0);
}





//=============================================================================
//      qut_register_class : Register the window class.
//-----------------------------------------------------------------------------
static ATOM
qut_register_class(void)
{	WNDCLASSEX wcex;



	// Register the window
	wcex.cbSize        = sizeof(WNDCLASSEX); 
	wcex.style         = CS_OWNDC;
	wcex.lpfnWndProc   = (WNDPROC) qut_wnd_proc;
	wcex.cbClsExtra    = 0;
	wcex.cbWndExtra    = 0;
	wcex.hInstance     = gInstance;
	wcex.hIcon         = LoadIcon(NULL, IDI_WINLOGO);
	wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = NULL;
	wcex.lpszMenuName  = (LPCSTR) IDC_QUT;
	wcex.lpszClassName = kQutWindowClass;
	wcex.hIconSm       = NULL;

	return(RegisterClassEx(&wcex));
}





//=============================================================================
//      qut_initialise_platform : Initialise the application.
//-----------------------------------------------------------------------------
static BOOL
qut_initialise_platform(int nCmdShow)
{	TQ3Status		qd3dStatus;



	// Initialise ourselves
	qd3dStatus = Q3Initialize();
	if (qd3dStatus != kQ3Success)
		return(FALSE);

	Qut_Initialise();
	App_Initialise();



	if (gWindow == NULL)
		return(FALSE);



	// Build the renderer menu
	qut_build_renderer_menu();



	// Show the window and start the timer
	ShowWindow((HWND) gWindow, nCmdShow);

	gAccelTable = LoadAccelerators(gInstance, (LPCTSTR)IDC_QUT);
	gTimer      = SetTimer((HWND)gWindow, WM_TIMER, kQutTimer, NULL);



	// Adjust the menu bar
	RemoveMenu(GetSubMenu(GetMenu((HWND)gWindow), kMenuRenderer), 0, MF_BYPOSITION);
	RemoveMenu(GetSubMenu(GetMenu((HWND)gWindow), kMenuSpecial),  0, MF_BYPOSITION);

	if (!gSpecialMenu)
		RemoveMenu(GetMenu((HWND)gWindow), kMenuSpecial, MF_BYPOSITION);

	DrawMenuBar((HWND)gWindow);

	return(TRUE);
}





//=============================================================================
//      qut_terminate_platform : Terminate ourselves.
//-----------------------------------------------------------------------------
static void
qut_terminate_platform(void)
{	TQ3Status		qd3dStatus;



	// Clean up
	if (gDC != NULL)
		ReleaseDC((HWND) gWindow, gDC);

	KillTimer((HWND)gWindow, gTimer);

    DestroyWindow((HWND) gWindow);



	// Terminate Quesa
	qd3dStatus = Q3Exit();
}





//=============================================================================
//		Public functions.
//-----------------------------------------------------------------------------
//      Qut_CreateWindow : Create the window.
//-----------------------------------------------------------------------------
#pragma mark -
void
Qut_CreateWindow(const char		*windowTitle,
					TQ3Uns32	theWidth,
					TQ3Uns32	theHeight,
					TQ3Boolean	canResize)
{


	// Create the window
	gWindow = (void *) CreateWindow(kQutWindowClass, windowTitle,
									WS_OVERLAPPEDWINDOW |
									WS_CLIPCHILDREN     |
									WS_CLIPSIBLINGS,
									30, 30, theWidth, theHeight,
									NULL, NULL, gInstance, NULL);



	// Save the window details
	gWindowCanResize = canResize;
}





//=============================================================================
//		Qut_CreateDrawContext : Create the draw context.
//-----------------------------------------------------------------------------
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





//=============================================================================
//		Qut_SelectMetafileToOpen : Select a metafile for opening.
//-----------------------------------------------------------------------------
TQ3StorageObject
Qut_SelectMetafileToOpen(void)
{	char				typeFilter[MAX_PATH] = "All Files (*.*)\0*.*\0"
											   "\0\0";
    char            	thePath[MAX_PATH]    = "";
	BOOL				selectedFile;
	TQ3StorageObject	theStorage;
    OPENFILENAME    	openFile;



	// Prompt the user for a file
	memset(&openFile, 0x00, sizeof(openFile));
	openFile.lStructSize       = sizeof(openFile);
    openFile.hwndOwner         = NULL;
    openFile.hInstance         = gInstance;
    openFile.lpstrFilter       = typeFilter;
    openFile.nFilterIndex      = 1;
    openFile.lpstrFile         = thePath;
    openFile.nMaxFile          = sizeof(thePath) - 1;
    openFile.lpstrInitialDir   = "..\\Support Files\\Models\\3DMF\\";
    openFile.lpstrTitle        = "Select a Model";
    openFile.Flags             = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST |
                                 OFN_LONGNAMES     | OFN_HIDEREADONLY;

	selectedFile = GetOpenFileName(&openFile);
	if (!selectedFile)
		return(NULL);



	// Create a storage object for the file
	theStorage = Q3PathStorage_New(thePath);
	return(theStorage);
}





//=============================================================================
//		Qut_SelectMetafileToSaveTo : Select a metafile to save to.
//-----------------------------------------------------------------------------
TQ3StorageObject
Qut_SelectMetafileToSaveTo(TQ3FileMode* fileMode)
{	char				typeFilter[MAX_PATH] = "All Files (*.*)\0*.*\0"
											   "\0\0";
    char            	thePath[MAX_PATH]    = "";
	BOOL				selectedFile;
	TQ3StorageObject	theStorage;
    OPENFILENAME    	openFile;



	// Prompt the user for a file
	memset(&openFile, 0x00, sizeof(openFile));
	openFile.lStructSize       = sizeof(openFile);
    openFile.hwndOwner         = NULL;
    openFile.hInstance         = gInstance;
    openFile.lpstrFilter       = typeFilter;
    openFile.nFilterIndex      = 1;
    openFile.lpstrFile         = thePath;
    openFile.nMaxFile          = sizeof(thePath) - 1;
    openFile.lpstrTitle        = "Save a Model";
    openFile.Flags             = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT |
                                 OFN_LONGNAMES     | OFN_HIDEREADONLY;

	selectedFile = GetSaveFileName(&openFile);
	if (!selectedFile)
		return(NULL);



	// Create a storage object for the file
	theStorage = Q3PathStorage_New(thePath);
	return(theStorage);
}





//=============================================================================
//		Qut_SelectPictureFile : Select a picture file.
//-----------------------------------------------------------------------------
TQ3Status
Qut_SelectPictureFile(void *theFile, TQ3Uns32 fileLen)
{

	// To be implemented...
	return(kQ3Failure);
}





//=============================================================================
//      Qut_CreateMenu : Create the Special menu.
//-----------------------------------------------------------------------------
void
Qut_CreateMenu(qutFuncAppMenuSelect appMenuSelect)
{


	// Set the flag and the callback
    gSpecialMenu   = TRUE;
	gAppMenuSelect = appMenuSelect;
}





//=============================================================================
//      Qut_CreateMenuItem : Create a menu item.
//-----------------------------------------------------------------------------
void
Qut_CreateMenuItem(TQ3Uns32 itemNum, const char *itemText)
{	TQ3Uns32	numItems, finalItemNum;
	HMENU		theMenu;



	// Get the special menu
	theMenu = GetMenu((HWND)gWindow);
	theMenu = GetSubMenu(theMenu, kMenuSpecial);
	if (theMenu == NULL)
		return;



	// Work out where the item is going to be
	numItems = GetMenuItemCount(theMenu);
	if (itemNum == 0)
		finalItemNum = 1;
	else if (itemNum > numItems)
		finalItemNum = numItems + 1;
	else
		finalItemNum = itemNum;



	// Insert the item
	if (strcmp(itemText, kMenuItemDivider) == 0)
		AppendMenu(theMenu, MF_SEPARATOR, kMenuSpecialOffset + finalItemNum, NULL);
	else
		AppendMenu(theMenu, MF_STRING,    kMenuSpecialOffset + finalItemNum, itemText);
}





//=============================================================================
//      WinMain : App entry point.
//-----------------------------------------------------------------------------
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{	MSG		theMsg;



	// Initialise ourselves
	gInstance = hInstance;
	qut_register_class();

	if (!qut_initialise_platform(nCmdShow)) 
		return(FALSE);



	// Run the app
	while (GetMessage(&theMsg, NULL, 0, 0))
		{
		if (!TranslateAccelerator(theMsg.hwnd, gAccelTable, &theMsg)) 
			{
			TranslateMessage(&theMsg);
			DispatchMessage(&theMsg);
			}
		}



	// Clean up
	App_Terminate();
	Qut_Terminate();
	qut_terminate_platform();

	return(theMsg.wParam);
}

