// =============================================================================
//                                  INCLUDES
// =============================================================================
#include <Windows.h>
#include <stdio.h>
#include <GL/glew.h>
#include <gl/GL.h>
#include <GL/wglew.h>
#include <tchar.h>

#include "LithoExporter.h"
#include "LithoSVG.h"
#include "StrokeRenderer.h"



// =============================================================================
//                                  DEFINES/MACROS
// =============================================================================

// =============================================================================
//                               GLOBAL VARIABLES
// =============================================================================
HGLRC   g_GLRenderContext;
HDC     g_HDCDeviceContext;
HWND    g_hwnd;
PFNWGLSWAPINTERVALEXTPROC       wglSwapIntervalEXT;
PFNWGLGETSWAPINTERVALEXTPROC    wglGetSwapIntervalEXT;
int     g_display_w = 800;
int     g_display_h = 600;

// infill
float spacing =10;
float fill_rate = 0.3;


// =============================================================================
//                             FOWARD DECLARATIONS
// =============================================================================
void CreateGlContext();
void SetCurrentContext();
bool SetSwapInterval(int interval); //0 - No Interval, 1 - Sync whit VSYNC, n - n times Sync with VSYNC
bool WGLExtensionSupported(const char* extension_name);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


// =============================================================================
//                            CORE MAIN FUNCTIONS
// =============================================================================
//
// Aplication Entry
//------------------------------------------------------------------------------
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	if (1)
	{
		AllocConsole();
		freopen("conin$", "r+t", stdin);
		freopen("conout$", "w+t", stdout);
		std::cout << "console opened" << std::endl;
	}

	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
	wc.lpszClassName = "NCUI";
	wc.style = CS_OWNDC;
	if (!RegisterClass(&wc))
		return 1;
	g_hwnd = CreateWindow(wc.lpszClassName, "teste", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 640, 480, 0, 0, hInstance, 0);

	// Show the window
	ShowWindow(g_hwnd, SW_SHOWDEFAULT);
	UpdateWindow(g_hwnd);

	// Prepare OpenGlContext
	CreateGlContext();
	SetSwapInterval(1);
	glewInit();

	// Renderer init
	//litho::LithoSVG svg;
	////svg.LoadSVG("../bin/svg/new.svg");
	//svg.LoadSVGFromStl("../bin/stl/micro-lens.stl", glm::vec3(100, 100, 100));
	//float curr_pixel_size = 100 * 0.0001f;
	//StrokeRenderer stroke(3, curr_pixel_size);
	//stroke.UpdatePolygonsData(svg, 5);

	litho::LithoSetting setting;
	setting.pixel_size_external = 100;
	setting.size_external = 1000;
	setting.stl_path = "../bin/stl/micro-lens.stl";
	setting.thickness_external = 10;
	litho::LithoExporter exporter(setting);
	exporter.ConvertToXML();

	
	

	// Main loop
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	
	int pixel_id = 0;
	while (msg.message != WM_QUIT)
	{
		// Poll and handle messages (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			continue;
		}

		wglMakeCurrent(g_HDCDeviceContext, g_GLRenderContext);
		glViewport(0, 0, g_display_w, g_display_h);                 //Display Size got from Resize Command
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		// render
		std::cout << pixel_id << std::endl;
		glm::vec2 curr_anchor = glm::vec2(0, 0);
		
		//stroke.DrawOffscreen(curr_anchor.x, curr_anchor.y, curr_pixel_size, std::to_string(pixel_id++));

	
		wglMakeCurrent(g_HDCDeviceContext, g_GLRenderContext);
		SwapBuffers(g_HDCDeviceContext);
		Sleep(50);
	}


	DestroyWindow(g_hwnd);
	UnregisterClass(_T("NCUI"), wc.hInstance);

	return 0;
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED)
		{
			g_display_w = (UINT)LOWORD(lParam);
			g_display_h = (UINT)HIWORD(lParam);
		}
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void CreateGlContext() {

	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
		PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
		32,                   // Colordepth of the framebuffer.
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		24,                   // Number of bits for the depthbuffer
		8,                    // Number of bits for the stencilbuffer
		0,                    // Number of Aux buffers in the framebuffer.
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	g_HDCDeviceContext = GetDC(g_hwnd);

	int pixelFormal = ChoosePixelFormat(g_HDCDeviceContext, &pfd);
	SetPixelFormat(g_HDCDeviceContext, pixelFormal, &pfd);
	g_GLRenderContext = wglCreateContext(g_HDCDeviceContext);
	wglMakeCurrent(g_HDCDeviceContext, g_GLRenderContext);
}

bool SetSwapInterval(int interval) {
	if (WGLExtensionSupported("WGL_EXT_swap_control"))
	{
		// Extension is supported, init pointers.
		wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

		// this is another function from WGL_EXT_swap_control extension
		wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC)wglGetProcAddress("wglGetSwapIntervalEXT");

		wglSwapIntervalEXT(interval);
		return true;
	}

	return false;

}

//Got from https://stackoverflow.com/questions/589064/how-to-enable-vertical-sync-in-opengl/589232
bool WGLExtensionSupported(const char* extension_name) {
	// this is pointer to function which returns pointer to string with list of all wgl extensions
	PFNWGLGETEXTENSIONSSTRINGEXTPROC _wglGetExtensionsStringEXT = NULL;

	// determine pointer to wglGetExtensionsStringEXT function
	_wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)wglGetProcAddress("wglGetExtensionsStringEXT");

	if (strstr(_wglGetExtensionsStringEXT(), extension_name) == NULL)
	{
		// string was not found
		return false;
	}

	// extension is supported
	return true;
}