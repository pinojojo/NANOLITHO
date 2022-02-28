//// =============================================================================
////                                  INCLUDES
//// =============================================================================
//#include <Windows.h>
//#include <stdio.h>
//
//
//#include <iostream>
//
//#include <imgui/imgui.h>
//#include <imgui/imgui_impl_win32.h>
//#include <imgui/imgui_impl_opengl3.h>
//#include <imgui/imgui_internal.h>
//#include <imgui/imfilebrowser.h>
//
//
//#include <GL/glew.h>
//#include <gl/GL.h>
//#include <GL/wglew.h>
//#include <tchar.h>
//
//#include "LithoModel.h"
//#include "LithoRenderer.h"
//
//
//// =============================================================================
////                                  DEFINES/MACROS
//// =============================================================================
//
//// =============================================================================
////                               GLOBAL VARIABLES
//// =============================================================================
//HGLRC   g_GLRenderContext;
//HDC     g_HDCDeviceContext;
//HWND    g_hwnd;
//PFNWGLSWAPINTERVALEXTPROC       wglSwapIntervalEXT;
//PFNWGLGETSWAPINTERVALEXTPROC    wglGetSwapIntervalEXT;
//int     g_display_w = 512;
//int     g_display_h = 512;
//
//
//LithoModel lithoModel;
//LithoRenderer lithoRenderer;
//bool perserReady = false;
//
//std::string svgFilePath;
//int renderMode = 0;
//
//
//
//
//// =============================================================================
////                             FOWARD DECLARATIONS
//// =============================================================================
//void CreateGlContext();
//void SetCurrentContext();
//bool SetSwapInterval(int interval); //0 - No Interval, 1 - Sync whit VSYNC, n - n times Sync with VSYNC
//bool WGLExtensionSupported(const char* extension_name);
//LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
//
//
//// =============================================================================
////                            CORE MAIN FUNCTIONS
//// =============================================================================
////
////
//// Aplication Entry
////------------------------------------------------------------------------------
//int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
//    UNREFERENCED_PARAMETER(hPrevInstance);
//    UNREFERENCED_PARAMETER(lpCmdLine);
//    UNREFERENCED_PARAMETER(nCmdShow);
//
//	if (1)
//	{
//		AllocConsole();
//		freopen("conin$", "r+t", stdin);
//		freopen("conout$", "w+t", stdout);
//        std::cout << "console opened" << std::endl;
//	}
//
//    lithoModel.DoParse("stl/lamb.stl");
//
//    std::cout << "loga" << lithoModel.m_Layers.size();
//   
//
//    WNDCLASS wc = { 0 };
//    wc.lpfnWndProc = WndProc;
//    wc.hInstance = hInstance;
//    wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
//    wc.lpszClassName = "NCUI";
//    wc.style = CS_OWNDC;
//    if (!RegisterClass(&wc))
//        return 1;
//    g_hwnd = CreateWindow(wc.lpszClassName, "main", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 1280, 1024, 0, 0, hInstance, 0);
//
//    // Show the window
//    ShowWindow(g_hwnd, SW_SHOWDEFAULT);
//    UpdateWindow(g_hwnd);
//
//    //Prepare OpenGlContext
//    CreateGlContext();
//    SetSwapInterval(1);
//    glewInit();
//    glEnable(GL_LINE_SMOOTH);
//    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
//    glEnable(GL_MULTISAMPLE);
//
//    // Setup Dear ImGui binding
//    IMGUI_CHECKVERSION();
//    ImGui::CreateContext();
//    ImGuiIO& io = ImGui::GetIO(); (void)io;
//    io.Fonts->AddFontFromFileTTF("fonts/CascadiaMono-SemiBold.ttf", 17);
//
//    //Init Win32
//    ImGui_ImplWin32_Init(g_hwnd);
//
//    //Init OpenGL Imgui Implementation
//    // GL 3.0 + GLSL 130
//    const char* glsl_version = "#version 330";
//    ImGui_ImplOpenGL3_Init(glsl_version);
//
//    lithoRenderer.Init();
//    
//
//
//    //Set Window bg color
//    ImVec4 clear_color = ImVec4(0.020F, 0.080F, 0.020F, 1.0F);
//
//    // Setup style
//    ImGui::StyleColorsDark();
//
//    // create a file browser instance
//    ImGui::FileBrowser fileDialog;
//    ImGui::FileBrowser stlFileDialog;
//
//    // (optional) set browser properties
//    fileDialog.SetTitle("title");
//    fileDialog.SetTypeFilters({ ".svg", ".txt" });
//
//    stlFileDialog.SetTitle("stl");
//    stlFileDialog.SetTypeFilters({ ".stl" });
//
//    
//
//    // Main loop
//    MSG msg;
//    ZeroMemory(&msg, sizeof(msg));
//    while (msg.message != WM_QUIT)
//    {
//        // Poll and handle messages (inputs, window resize, etc.)
//        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
//        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
//        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
//        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
//        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
//        {
//            TranslateMessage(&msg);
//            DispatchMessage(&msg);
//            continue;
//        }
//
//        
//        
//
//        // Start the Dear ImGui frame
//        ImGui_ImplOpenGL3_NewFrame();
//        ImGui_ImplWin32_NewFrame();
//        ImGui::NewFrame();
//
//       
//
//        // demo window
//        if (true)
//        {
//            ImGui::ShowDemoWindow();
//        }
//        
//        // data window
//        if (1)
//        {
//            if (ImGui::Begin("data window"))
//            {
//                
//
//                {
//                    int layerNum=lithoModel.m_Layers.size();
//                    auto& layers = lithoModel.m_Layers;
//
//                    vector<char*> layerNames;
//                    for (auto& layer:layers)
//                    {
//                        layerNames.push_back(&layer.name[0]);
//                    }
//                    
//                    if (ImGui::TreeNode("sliced data structure"))
//                    {
//                        for (int i = 0; i < layerNum; i++)
//                        {
//                            if (i == 0)
//                                ImGui::SetNextItemOpen(false, ImGuiCond_Once);
//
//                            if (ImGui::TreeNode((void*)(intptr_t)i, layerNames[i], i))
//                            {
//                                int polygonNum=layers[i].polygons.size();
//
//								
//
//                                for (int j = 0; j < polygonNum; j++)
//                                {
//                                  
//                                    
//
//                                    if (ImGui::TreeNode((void*)(intptr_t)j, "polygon %d", j))
//                                    {
//                                        ImGui::SameLine();
//                                        if (ImGui::SmallButton("use"))
//                                        {
//                                            std::cout << " use layer: " << i << "polygon: " << j << std::endl;
//
//                                            lithoRenderer.RemakePolygonVAO(lithoModel, i, j);
//                                        }
//                                        int pointNum = layers[i].polygons[j].points.size();
//
//                                        for (size_t ptid = 0; ptid < pointNum; ptid++)
//                                        {
//                                            std::string pointStr;
//                                            
//                                            pointStr = "x: " + std::to_string(layers[i].polygons[j].points[ptid].x) + " y: " + std::to_string(layers[i].polygons[j].points[ptid].y);
//
//                                            ImGui::Text(pointStr.c_str());
//
//                                        }
//                                       
//
//
//
//                                        ImGui::TreePop();
//                                    }
//
//                                   
//                                }
//
//
//                                
//                                ImGui::TreePop();
//                            }
//                        }
//                        ImGui::TreePop();
//                    }
//
//                }
//                
//                
//            }
//            ImGui::End();
//        }
//
//        // controller window
//        if (true)
//        {
//            if (ImGui::Begin("controller window"))
//            {
//                // open file dialog when user clicks this button
//                if (ImGui::Button("import stl file", ImVec2(200, 30)))
//                    stlFileDialog.Open();
//
//                ImGui::SameLine();
//
//                // open file dialog when user clicks this button
//                if (ImGui::Button("import svg file" ,ImVec2(200, 30)))
//                    fileDialog.Open();
//
//
//                // slicing parameter
//                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
//                ImGui::Separator();
//
//
//                {
//                     
//                    static float physical_pixel_size = 100; //nm
//                    static float physical_expected_model_size = 1000;//um
//                    static bool  expected_direction = 0; // 0 for x dir, for y direction
//                    static float physical_expected_slicing_thickness;
//                    
//                }
//
//
//                
//               
//                
//
//                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
//
//                ImGui::Separator();
//                {
//                    const char* items[] = { "line", "stroke", "mask", "infill", "final" };
//                    static int item_current = 0;
//                    if (ImGui::Combo("render mode", &item_current, items, IM_ARRAYSIZE(items))) 
//                    {
//                        renderMode = item_current;
//                        std::cout << item_current << std::endl;
//                    }
//                    // config each render mode
//                    {
//                        switch (item_current)
//                        {
//                        case 0:
//                            
//                            break;
//                        case 1:
//                        {
//                            static float lineWidth = 0.1f;
//                            if (ImGui::SliderFloat("line width", &lineWidth, 0.0001f, 3.0f, "line width = %.4f"))
//                            {
//                                lithoRenderer.SetLineWidth(lineWidth);
//                            }
//                        }
//                        break;
//                        case 3:
//                        {
//                            static float infill_line_width = 0.02f;
//                            static float infill_line_gap = 0.08f;
//                            static int infill_line_num = 100;
//                            if (ImGui::SliderFloat("infill line width", &infill_line_width, 0.0001f, 10.f, "%.4f"))
//                            {
//                                if (infill_line_gap<infill_line_width)
//                                {
//                                    infill_line_gap = 1.01 * infill_line_width;
//                                }
//                                lithoRenderer.RemakeInfillVAO(infill_line_width, infill_line_gap, infill_line_num);
//                            }
//                           
//                            if (ImGui::SliderFloat("infill line gap", &infill_line_gap, 0.0001f, 10.f, "%.4f"))
//                            {
//                                if (infill_line_gap < infill_line_width)
//                                {
//                                    infill_line_width = 0.99 * infill_line_gap;
//                                }
//                                lithoRenderer.RemakeInfillVAO(infill_line_width, infill_line_gap, infill_line_num);
//
//                            }
//                            if (ImGui::SliderInt("infill line num", &infill_line_num, 20, 200, "%d"))
//                            {
//                                lithoRenderer.RemakeInfillVAO(infill_line_width, infill_line_gap, infill_line_num);
//
//                            }
//
//                            
//                        }
//                            break;
//                        }
//
//                    }
//                }
//
//                
//                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
//
//                ImGui::Separator();
//               
//
//                // mouse interaction
//                {
//					if (ImGui::IsMousePosValid())
//					{
//						//ImGui::Text("mouse pos: (%g,%g)", io.MousePos.x, io.MousePos.y);
//						//ImGui::Text("Mouse down:");
//						
//                        // drag in render window
//                        static int click_pos_x, click_pos_y;
//                        static int curr_pos_x, curr_pos_y;
//                        static int curr_shift_x, curr_shift_y;
//
//                        curr_pos_x = io.MousePos.x;
//                        curr_pos_y = io.MousePos.y;
//
//                        if (ImGui::IsMouseClicked(0))
//                        {
//                            click_pos_x = io.MousePos.x;
//                            click_pos_y = io.MousePos.y;
//                        }
//                       
//                        bool isRenderWindowSelected = (curr_pos_x > 0)&&(curr_pos_x<1000);
//                        if (isRenderWindowSelected)
//                        {
//                            if (ImGui::IsMouseDown(0))
//                            {
//                                curr_shift_x = curr_pos_x - click_pos_x;
//                                curr_shift_y = curr_pos_y - click_pos_y;
//                                //ImGui::Text("shift : (%d,%d)", curr_shift_x, curr_shift_y);
//
//                                lithoRenderer.MoveByPixel(curr_shift_x, curr_shift_y, 0);
//                            }
//                            if (ImGui::IsMouseReleased(0))
//                            {
//                                lithoRenderer.MoveByPixel(curr_shift_x, curr_shift_y, 1);
//                            }
//
//                            /*pixelSize += io.MouseWheel*0.001;
//                            if (pixelSize<0)
//                            {
//                                pixelSize = 0.000001;
//                            }
//                            lithoRenderer.SetPixelSize(pixelSize);*/
//
//                         
//                            auto center = lithoRenderer.GetCenter();
//                            center = lithoRenderer.GetCenter();
//                            std::string centerNow = std::to_string(center.x) + " " + std::to_string(center.y);
//                            ImGui::Text(centerNow.c_str());
//                            
//                            
//                        }
//                        
//
//
//                        
//                        
//                    }
//                }
//
//            }
//            ImGui::End();
//
//            // handling svg data importing
//            fileDialog.Display();
//            if (fileDialog.HasSelected())
//            {
//                svgFilePath = fileDialog.GetSelected().string();
//                fileDialog.ClearSelected();
//                std::cout << "Selected filename" << svgFilePath << std::endl;
//                lithoModel.DoParse(svgFilePath);
//                perserReady = true;
//            }
//            // handling stl data importing
//            stlFileDialog.Display();
//            if (stlFileDialog.HasSelected())
//            {
//                std::string stlPath = stlFileDialog.GetSelected().string();
//                stlFileDialog.ClearSelected();
//                std::cout << "Selected filename" << stlPath << std::endl;
//                lithoModel.DoParse(stlPath);
//                
//                perserReady = true;
//            }
//
//
//        }
//
//       
//
//        // Rendering
//        
//        wglMakeCurrent(g_HDCDeviceContext, g_GLRenderContext);
//
//        //lithoRenderer.DrawPolygonWithInfill();
//        lithoRenderer.RenderSpecificMode(renderMode);
//	
//
//        glViewport(0, 512, g_display_w, g_display_h);                 //Display Size got from Resize Command
//        //glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
//        //glClear(GL_COLOR_BUFFER_BIT);
//        ImGui::Render();
//        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
//
//        
//
//        wglMakeCurrent(g_HDCDeviceContext, g_GLRenderContext);
//        SwapBuffers(g_HDCDeviceContext);
//
//    }
//
//    // Cleanup
//    ImGui_ImplOpenGL3_Shutdown();
//    wglDeleteContext(g_GLRenderContext);
//    ImGui::DestroyContext();
//    ImGui_ImplWin32_Shutdown();
//
//    DestroyWindow(g_hwnd);
//    UnregisterClass(_T("NCUI"), wc.hInstance);
//
//    return 0;
//}
//
//extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
//LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
//    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
//        return true;
//
//    switch (msg)
//    {
//    case WM_SIZE:
//        if (wParam != SIZE_MINIMIZED)
//        {
//            g_display_w = (UINT)LOWORD(lParam);
//            g_display_h = (UINT)HIWORD(lParam);
//        }
//        return 0;
//    case WM_SYSCOMMAND:
//        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
//            return 0;
//        break;
//    case WM_DESTROY:
//        PostQuitMessage(0);
//        return 0;
//    }
//    return DefWindowProc(hWnd, msg, wParam, lParam);
//}
//
//void CreateGlContext() {
//
//    PIXELFORMATDESCRIPTOR pfd =
//    {
//        sizeof(PIXELFORMATDESCRIPTOR),
//        1,
//        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
//        PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
//        32,                   // Colordepth of the framebuffer.
//        0, 0, 0, 0, 0, 0,
//        0,
//        0,
//        0,
//        0, 0, 0, 0,
//        24,                   // Number of bits for the depthbuffer
//        8,                    // Number of bits for the stencilbuffer
//        0,                    // Number of Aux buffers in the framebuffer.
//        PFD_MAIN_PLANE,
//        0,
//        0, 0, 0
//    };
//
//    g_HDCDeviceContext = GetDC(g_hwnd);
//
//    int pixelFormal = ChoosePixelFormat(g_HDCDeviceContext, &pfd);
//    SetPixelFormat(g_HDCDeviceContext, pixelFormal, &pfd);
//    g_GLRenderContext = wglCreateContext(g_HDCDeviceContext);
//    wglMakeCurrent(g_HDCDeviceContext, g_GLRenderContext);
//}
//
//bool SetSwapInterval(int interval) {
//    if (WGLExtensionSupported("WGL_EXT_swap_control"))
//    {
//        // Extension is supported, init pointers.
//        wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
//
//        // this is another function from WGL_EXT_swap_control extension
//        wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC)wglGetProcAddress("wglGetSwapIntervalEXT");
//
//        wglSwapIntervalEXT(interval);
//        return true;
//    }
//
//    return false;
//
//}
//
////Got from https://stackoverflow.com/questions/589064/how-to-enable-vertical-sync-in-opengl/589232
//bool WGLExtensionSupported(const char* extension_name) {
//    // this is pointer to function which returns pointer to string with list of all wgl extensions
//    PFNWGLGETEXTENSIONSSTRINGEXTPROC _wglGetExtensionsStringEXT = NULL;
//
//    // determine pointer to wglGetExtensionsStringEXT function
//    _wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)wglGetProcAddress("wglGetExtensionsStringEXT");
//
//    if (strstr(_wglGetExtensionsStringEXT(), extension_name) == NULL)
//    {
//        // string was not found
//        return false;
//    }
//
//    // extension is supported
//    return true;
//}
