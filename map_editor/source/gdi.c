#define _WIN32_WINNT 0x0600
#include <windows.h>
// #include <stdio.h>
#include "common.h"

// Function Declarations
static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void processInput(HWND hwnd);
// static void toggleFullscreen(HWND hwnd);
static void updateCursorPos(HWND hwnd);
static HBITMAP initscreen(HDC hdc, int w, int h, u16 **data);
static void get_argv_from_str(char *arg_str, int *argc, char *argv[]);

#define HID_USAGE_PAGE_GENERIC ((USHORT) 0x01)
#define HID_USAGE_GENERIC_MOUSE ((USHORT) 0x02)

HDC hdc;
HDC memdc;

static int windows_width, windows_height;
static u8 view_lines_mode;
float delta_time_f;

u8 i_cursor_key_pressed, i_key_pressed, i_modifier_key_pressed, mouse_moved, i_mouse_button_pressed, enable_mouse_cursor;
u16 i_direction_pressed;
int i_cursor_dx, i_cursor_dy;

u8 mouse_capture; // fullscreen
static int cursor_xpos, cursor_ypos;

char *file_path;

// WinMain
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int iCmdShow) {
  WNDCLASS wc;
  HWND hwnd;
  MSG msg;
  //HDC hdc;
  //HDC memdc;
  HBITMAP bmp;
  BOOL quit = FALSE;
  
  // register window class
  wc.style = CS_OWNDC;
  wc.lpfnWndProc = WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  wc.lpszMenuName = NULL;
  wc.lpszClassName = "test";
  RegisterClass(&wc);
  
  int wf, hf;
  wf = PC_SCREEN_WIDTH * PC_SCREEN_SCALE_SIZE; //* 5
  hf = PC_SCREEN_HEIGHT * PC_SCREEN_SCALE_SIZE; //* 5
  windows_width = wf + 6; //wf
  windows_height = hf + 35; //hf
  //550,300,262,287
  SetProcessDPIAware();
  
  // create main window
  hwnd = CreateWindow(
    "test", "test",
    WS_CAPTION | WS_POPUPWINDOW | WS_VISIBLE | WS_MINIMIZEBOX,
    0, 0, windows_width, windows_height,
    NULL, NULL, hInstance, NULL);
  hdc = GetDC(hwnd);
  
  RAWINPUTDEVICE Rid[1];
  Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
  Rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
  Rid[0].dwFlags = RIDEV_INPUTSINK;
  Rid[0].hwndTarget = hwnd;
  RegisterRawInputDevices(Rid, 1, sizeof(Rid[0]));
  
  memdc = CreateCompatibleDC(hdc);
  bmp = initscreen(hdc, PC_SCREEN_WIDTH, PC_SCREEN_HEIGHT, &screen);
  SelectObject(memdc, bmp);
  timeBeginPeriod(1);
  
  MoveWindow(hwnd, 500, 100, windows_width, windows_height, TRUE);
  
  int argc;
  char *argv[2];
  LPSTR arg_str = GetCommandLineA();
  get_argv_from_str(arg_str, &argc, argv);
  
  if (argc < 2) {
    printf("no arguments passed");
    return 1;
  }
  
  // char *file_path = (char*)arg_str;
  file_path = argv[1];
  
  init_memory();
  load_files(file_path);
  
  // fullscreen = 0;
  view_lines_mode = 0;
  frame_cycles = 0;
  delta_time_f = 0;
  fps = 60;
  redraw_scene = 1;
  dbg_show_poly_num = 0;
  dbg_show_grid_tile_num = 0;
  mouse_capture = 1;
  enable_mouse_cursor = 0;
  mouse_moved = 0;
  i_cursor_dx = 0;
  i_cursor_dy = 0;
  i_mouse_button_pressed = 0;
  i_direction_pressed = 0;
  i_cursor_key_pressed = 0;
  i_key_pressed = 0;
  i_modifier_key_pressed = 0;
  updateCursorPos(hwnd);
  
  init_3d();
  init_game();
  //BitBlt(hdc, 0, 0, PC_SCREEN_WIDTH, PC_SCREEN_HEIGHT, memdc, 0, 0, SRCCOPY);
  StretchBlt(hdc, 0, 0, wf, hf, memdc, 0, 0, PC_SCREEN_WIDTH, PC_SCREEN_HEIGHT, SRCCOPY);
  
  float update_time = 1000.0 / 60.0; //20;
  float delta_time_t = update_time;
  LARGE_INTEGER t, rt;
  QueryPerformanceFrequency(&t);
  float PCFreq = t.QuadPart / 1000.0;
  
  // program main loop
  while (!quit) {
    key_prev = key_curr;
    
    // check for messages
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      // handle or dispatch messages
      if (msg.message == WM_QUIT) {
        quit = TRUE;
      } else {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
    
    QueryPerformanceCounter(&t);
    if (delta_time_t < update_time) {
      delta_time_t = update_time;
    }
    
    while (delta_time_t > 0) {
      // if (mouse_capture)
      //   processInput(hwnd); // old method without raw input
      
      float dt = min_c(delta_time_t, update_time);
      update_state(fix(dt / 1000));
      delta_time_t -= update_time;
    }
    
    if (mouse_capture && mouse_moved) {
      mouse_moved = 0;
      i_cursor_dx = 0;
      i_cursor_dy = 0;
      SetCursorPos(cursor_xpos, cursor_ypos);
    }
    
    if (redraw_scene) {
      draw();
      
      if (cfg.debug) {
        draw_dbg();
      }
      
      //BitBlt(hdc, 0, 0, PC_SCREEN_WIDTH, PC_SCREEN_HEIGHT, memdc, 0, 0, SRCCOPY);
      StretchBlt(hdc, 0, 0, wf, hf, memdc, 0, 0, PC_SCREEN_WIDTH, PC_SCREEN_HEIGHT, SRCCOPY);
      redraw_scene = 0;
    }

    QueryPerformanceCounter(&rt);
    delta_time_f = (rt.QuadPart - t.QuadPart) / PCFreq;
    delta_time_t = delta_time_f;
    fps = 1000.0 / delta_time_f;
    if (fps > 60) fps = 60;
    
    if (delta_time_f < update_time - 1) {
      Sleep(update_time - delta_time_f - 1);
    }
  }
  
  // free(z_buffer);
  free_memory();
  free(shared_face_dir);
  
  timeEndPeriod(1);
  DeleteObject(bmp);
  DeleteDC(memdc);
  ReleaseDC(hwnd, hdc);
  DestroyWindow(hwnd);
  return msg.wParam;
}

// Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg){
    
    case WM_CLOSE:
      mouse_capture = 0;
      updateCursorPos(hwnd);
      PostQuitMessage(0);
      break;
    
    case WM_KEYDOWN:
      if (!(lParam & 0x40000000)) {
        if (wParam == 'W') {
          i_direction_pressed |= KEY_W;
        } else
        if (wParam == 'S') {
          i_direction_pressed |= KEY_S;
        }
        
        if (wParam == 'A') {
          i_direction_pressed |= KEY_A;
        } else
        if (wParam == 'D') {
          i_direction_pressed |= KEY_D;
        }
        
        if (wParam == 'Q') {
          i_direction_pressed |= KEY_Q;
        } else
        if (wParam == 'E') {
          i_direction_pressed |= KEY_E;
        }
        
        if (wParam == VK_NUMPAD1) {
          i_direction_pressed |= KEY_NUMPAD_1;
        } else
        if (wParam == VK_NUMPAD2) {
          i_direction_pressed |= KEY_NUMPAD_2;
        }
        
        if (wParam == VK_NUMPAD4) {
          i_direction_pressed |= KEY_NUMPAD_4;
        } else
        if (wParam == VK_NUMPAD5) {
          i_direction_pressed |= KEY_NUMPAD_5;
        }
        
        if (wParam == VK_NUMPAD3) {
          i_direction_pressed |= KEY_NUMPAD_3;
        } else
        if (wParam == VK_NUMPAD6) {
          i_direction_pressed |= KEY_NUMPAD_6;
        }
        
        if (wParam == VK_UP) {
          i_cursor_key_pressed |= KEY_UP;
        } else
        if (wParam == VK_DOWN) {
          i_cursor_key_pressed |= KEY_DOWN;
        }
        
        if (wParam == VK_LEFT) {
          i_cursor_key_pressed |= KEY_LEFT;
        } else
        if (wParam == VK_RIGHT) {
          i_cursor_key_pressed |= KEY_RIGHT;
        }
        
        if (wParam == 'Z') {
          i_cursor_key_pressed |= KEY_Z;
        } else
        if (wParam == 'X') {
          i_cursor_key_pressed |= KEY_X;
        }
        
        if (wParam == VK_RETURN) {
          i_key_pressed = KEY_RETURN;
        } else
        if (wParam == VK_SPACE) {
          i_key_pressed = KEY_SPACE;
        } else
        if (wParam == VK_CONTROL) {
          i_key_pressed = KEY_CONTROL;
        } else
        if (wParam == VK_BACK) {
          i_key_pressed = KEY_BACK;
        }
        
        if (wParam == VK_OEM_COMMA) {
          i_key_pressed = KEY_COMMA;
        } else
        if (wParam == VK_OEM_PERIOD) {
          i_key_pressed = KEY_POINT;
        } else
        if (wParam == VK_OEM_MINUS) {
          i_key_pressed = KEY_MINUS;
        }
        
        if (wParam == 'C') {
          i_key_pressed |= KEY_C;
        }
        if (wParam == 'V') {
          i_key_pressed |= KEY_V;
        }
        if (wParam == 'B') {
          i_key_pressed |= KEY_B;
        }
        if (wParam == 'M') {
          i_key_pressed |= KEY_M;
        }
        if (wParam == 'P') {
          i_key_pressed |= KEY_P;
        }
        
        if (wParam == VK_SHIFT) {
          i_modifier_key_pressed |= KEY_SHIFT;
        }
        
        if (wParam == 'F') {
          cfg.debug ^= 1;
          redraw_scene = 1;
        }
        
        if (wParam == 'G') { //lines
          if (!view_lines_mode) {
            view_lines_mode = 1;
            cfg.draw_lines = 1;
          } else
          if (view_lines_mode == 1) {
            view_lines_mode = 2;
            cfg.draw_polys = 0;
          } else {
            view_lines_mode = 0;
            cfg.draw_lines = 0;
            cfg.draw_polys = 1;
          }
          
          redraw_scene = 1;
        }
        
        if (wParam == 'H') {
          map_grid_enabled ^= 1;
          redraw_scene = 1;
        }
        
        if (wParam == 'R') {
          cfg.tx_perspective_mapping_enabled ^= 1;
          redraw_scene = 1;
        }
        
        if (wParam == 'T') {
          cfg.draw_textures ^= 1;
          redraw_scene = 1;
        }
        
        /* if (wParam == 'C') {
          game.collision_enabled ^= 1;
          game.cam_is_on_ground = 0;
        } */
        
        if (wParam == 'O') {
          export_map_file(file_path);
        }
        
        /* if (wParam == 'V') {
          dbg_show_grid_tile_num ^= 1;
          redraw_scene = 1;
        }
        
        if (wParam == 'B') {
          dbg_show_poly_num ^= 1;
          redraw_scene = 1;
        }
        
        if (wParam == 'N') {
          if (dbg_show_poly_num) {
            if (GetKeyState(VK_SHIFT) & 0x8000) {
              dbg_num_poly_dsp -= 10;
              
              if (dbg_num_poly_dsp < 0) {
                dbg_num_poly_dsp = 0;
              }
            } else {
              dbg_num_poly_dsp--;
              
              if (dbg_num_poly_dsp < 0) {
                dbg_num_poly_dsp = g_model->num_faces;
              }
            }
          } else
          if (dbg_show_grid_tile_num) {
            if (GetKeyState(VK_SHIFT) & 0x8000) {
              dbg_num_grid_tile_dsp -= 10;
              
              if (dbg_num_grid_tile_dsp < 0) {
                dbg_num_grid_tile_dsp = 0;
              }
            } else {
              dbg_num_grid_tile_dsp--;
              
              if (dbg_num_grid_tile_dsp < 0) {
                dbg_num_grid_tile_dsp = dl.num_tiles;
              }
            }
          }
          
          redraw_scene = 1;
        }
        
        if (wParam == 'M') {
          if (dbg_show_poly_num) {
            if (GetKeyState(VK_SHIFT) & 0x8000) {
              dbg_num_poly_dsp += 10;
              
              if (dbg_num_poly_dsp > g_model->num_faces) {
                dbg_num_poly_dsp = g_model->num_faces;
              }
            } else {
              dbg_num_poly_dsp++;
              
              if (dbg_num_poly_dsp > g_model->num_faces) {
                dbg_num_poly_dsp = 0;
              }
            }
          } else
          if (dbg_show_grid_tile_num) {
            if (GetKeyState(VK_SHIFT) & 0x8000) {
              dbg_num_grid_tile_dsp += 10;
              
              if (dbg_num_grid_tile_dsp > dl.num_tiles) {
                dbg_num_grid_tile_dsp = dl.num_tiles;
              }
            } else {
              dbg_num_grid_tile_dsp++;
              
              if (dbg_num_grid_tile_dsp > dl.num_tiles) {
                dbg_num_grid_tile_dsp = 0;
              }
            }
          }
          
          redraw_scene = 1;
        } */
        
        // printf("%d",(int)i_direction_pressed);
        if (wParam == VK_ESCAPE) {
          mouse_capture = 0;
          updateCursorPos(hwnd);
          PostQuitMessage(0);
        }
        
        if (wParam == VK_F1) { // free cursor
          mouse_capture ^= 1;
          updateCursorPos(hwnd);
        }
        
        if (wParam == VK_F2) {
          enable_mouse_cursor ^= 1;
          updateCursorPos(hwnd);
        }
        
        // if (wParam==VK_F11){toggleFullscreen(hwnd);}
        
        /* if (wParam==VK_RETURN){
          if (!animation_play){animation_play=1;} else {animation_play=0;}
        } */
      } else { // trigger key repeats
        if (wParam == VK_UP) {
          i_cursor_key_pressed |= KEY_UP;
        } else
        if (wParam == VK_DOWN) {
          i_cursor_key_pressed |= KEY_DOWN;
        }
        
        if (wParam == VK_LEFT) {
          i_cursor_key_pressed |= KEY_LEFT;
        } else
        if (wParam == VK_RIGHT) {
          i_cursor_key_pressed |= KEY_RIGHT;
        }
        
        if (wParam == 'Z') {
          i_cursor_key_pressed |= KEY_Z;
        } else
        if (wParam == 'X') {
          i_cursor_key_pressed |= KEY_X;
        }
        
        if (wParam == VK_OEM_COMMA) {
          i_key_pressed = KEY_COMMA;
        } else
        if (wParam == VK_OEM_PERIOD) {
          i_key_pressed = KEY_POINT;
        }
      }
      break;

    case WM_KEYUP:
      if (wParam == 'W') {
        i_direction_pressed ^= KEY_W;
      }
      
      if (wParam == 'S') {
        i_direction_pressed ^= KEY_S;
      }
      
      if (wParam == 'A') {
        i_direction_pressed ^= KEY_A;
      }
      
      if (wParam == 'D') {
        i_direction_pressed ^= KEY_D;
      }
      
      if (wParam == 'Q') {
        i_direction_pressed ^= KEY_Q;
      }
      
      if (wParam == 'E') {
        i_direction_pressed ^= KEY_E;
      }
        
      if (wParam == VK_NUMPAD1) {
        i_direction_pressed ^= KEY_NUMPAD_1;
      }
      
      if (wParam == VK_NUMPAD2) {
        i_direction_pressed ^= KEY_NUMPAD_2;
      }
      
      if (wParam == VK_NUMPAD3) {
        i_direction_pressed ^= KEY_NUMPAD_3;
      }
      
      if (wParam == VK_NUMPAD4) {
        i_direction_pressed ^= KEY_NUMPAD_4;
      }
      
      if (wParam == VK_NUMPAD5) {
        i_direction_pressed ^= KEY_NUMPAD_5;
      }
      
      if (wParam == VK_NUMPAD6) {
        i_direction_pressed ^= KEY_NUMPAD_6;
      }
      
      if (wParam == VK_SHIFT) {
        i_modifier_key_pressed ^= KEY_SHIFT;
      }
      
      //printf("%d\n",(int)i_direction_pressed);
      break;
    
    // case WM_SYSKEYDOWN:
    // if (wParam == VK_RETURN && HIWORD(lParam) & KF_ALTDOWN){
    //   toggleFullscreen(hwnd);
    //   return 0;
    // }
    // break;
    
    case WM_INPUT:
      if (mouse_capture){
        UINT dwSize = 48;
        static BYTE lpb[48];
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));
        RAWINPUT* raw = (RAWINPUT*)lpb;
        
        if (raw->header.dwType == RIM_TYPEMOUSE){
          if (raw->data.mouse.lLastX || raw->data.mouse.lLastY) {
            mouse_moved = 1;
            i_cursor_dx += raw->data.mouse.lLastX;
            i_cursor_dy += raw->data.mouse.lLastY;
          }
          
          u16 buttons = raw->data.mouse.usButtonFlags;
          
          if (buttons & RI_MOUSE_LEFT_BUTTON_DOWN) {
            i_mouse_button_pressed = LEFT_MOUSE_BUTTON;
          } else if (buttons & RI_MOUSE_RIGHT_BUTTON_DOWN) {
            i_mouse_button_pressed = RIGHT_MOUSE_BUTTON;
          }
        }
      }
      break;
    
    case WM_ACTIVATE:
      if (!wParam){
        if (mouse_capture){
          mouse_capture = 0;
          updateCursorPos(hwnd);
        }
      }
      break;
    
    case WM_PAINT: {
      PAINTSTRUCT ps;
      BeginPaint(hwnd, &ps);
      BitBlt(hdc, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top, memdc, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
      EndPaint(hwnd, &ps);
      break;
    }
    
    default:
      return DefWindowProc(hwnd, msg, wParam, lParam);
  }
  return 0;
}

void processInput(HWND hwnd) {
  POINT p;
  GetCursorPos(&p);
  ScreenToClient(hwnd, &p);
  i_cursor_dx = p.x - SCREEN_HALF_WIDTH * PC_SCREEN_SCALE_SIZE;
  i_cursor_dy = p.y - SCREEN_HALF_HEIGHT * PC_SCREEN_SCALE_SIZE;
  //printf("%d %d\n",i_cursor_dx,i_cursor_dy);
}

void updateCursorPos(HWND hwnd) {
  if (mouse_capture) {
    POINT p;
    RECT r;
    p.x = 0;
    p.y = 0;
    ClientToScreen(hwnd, &p);
    GetClientRect(hwnd, &r);
    r.top += p.y;
    r.bottom += p.y;
    r.left += p.x;
    r.right += p.x;
    cursor_xpos = p.x + SCREEN_HALF_WIDTH * PC_SCREEN_SCALE_SIZE;
    cursor_ypos = p.y + SCREEN_HALF_HEIGHT * PC_SCREEN_SCALE_SIZE;
    ShowCursor(FALSE);
    SetCursorPos(cursor_xpos, cursor_ypos);
    ClipCursor(&r);
  } else {
    ClipCursor(NULL);
    ShowCursor(TRUE);
  }
}

/* void toggleFullscreen(HWND hwnd) {
  if (!fullscreen) {
    fullscreen = 1;
    vp.screen_width = 1920;
    vp.screen_height = 1080;
    SetWindowLongPtr(hwnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);
    MoveWindow(hwnd, 0, 0, vp.screen_width, vp.screen_height, TRUE);
  } else {
    fullscreen = 0;
    vp.screen_width = 896;
    vp.screen_height = 768;
    SetWindowLongPtr(hwnd, GWL_STYLE, WS_CAPTION | WS_POPUPWINDOW | WS_VISIBLE | WS_MINIMIZEBOX);
    MoveWindow(hwnd, 500, 100, windows_width, windows_height, TRUE);
  }
  
  DeleteObject(bmp);
  bmp = initscreen(hdc, vp.screen_width, vp.screen_height,  &screen);
  SelectObject(memdc, bmp);
  initm();
  draw();
  BitBlt(hdc, 0, 0, vp.screen_width, vp.screen_height, memdc, 0, 0, SRCCOPY);
} */

HBITMAP initscreen(HDC hdc, int w, int h, u16 **data) {
  BITMAPINFO *bmi;
  int binfo_size = sizeof(*bmi);
  binfo_size += 3 * sizeof(DWORD);
  bmi = calloc(1, binfo_size);
  
  bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi->bmiHeader.biWidth = w;
  bmi->bmiHeader.biHeight = -h;
  bmi->bmiHeader.biSizeImage = (w * 2) * h;
  bmi->bmiHeader.biPlanes = 1;
  bmi->bmiHeader.biBitCount = 16;
  bmi->bmiHeader.biCompression = BI_BITFIELDS; //BI_RGB;
  bmi->bmiHeader.biClrUsed = 0;
  ((DWORD *)bmi->bmiColors)[0] = 0x001F; // r
  ((DWORD *)bmi->bmiColors)[1] = 0x03E0; // g
  ((DWORD *)bmi->bmiColors)[2] = 0x7C00; // b
  
  HBITMAP bmp;
  bmp = CreateDIBSection(hdc, bmi, DIB_RGB_COLORS, (void**)data, NULL, 0);
  free(bmi);
  return bmp;
}

void get_argv_from_str(char *arg_str, int *argc, char *argv[]) {
  int count = 0;
  char *ptr = arg_str;
  
  while (*ptr) {
    // skip any leading space
    if (*ptr == ' ') ptr++;
    
    // end of string
    if (*ptr == '\0') break;
    
    // if opening quotes were found
    if (*ptr == '"'){
      ptr++; // skip the first quotes
      argv[count] = ptr;
      
      while (*ptr && *ptr != '"') ptr++;
      
      // closing quotes
      if (*ptr == '"') {
        *ptr = '\0';
        ptr++;
      }
    } else {
      argv[count] = ptr;
      
      while (*ptr && *ptr != ' ') ptr++;
      
      // found a space
      if (*ptr) {
        *ptr = '\0';
        ptr++;
      }
    }
    
    count++;
  }
  
  *argc = count;
}