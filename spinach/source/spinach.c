// spinach.c

#include "spinach.h"

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <windowsx.h>
#include <gl/gl.h>

#include <math.h>
#include <stdio.h>
#include <stdarg.h>

#pragma warning(push)
#pragma warning(disable : 4456) // declaration of hides previous local declaration
#define STB_IMAGE_STATIC
#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#pragma warning(pop)

// globals
static bool global_logging_active = false;
static const char* global_window_title = 0;
static HWND global_window_handle = 0;
static HDC global_window_device = 0;
static int global_window_width = 0;
static int global_window_height = 0;
static spinach_point_t global_mouse_position = { 0 };
static bool global_buttons_curr[MOUSE_BUTTON_COUNT] = { 0 };
static bool global_buttons_prev[MOUSE_BUTTON_COUNT] = { 0 };
static bool global_keys_curr[KEY_COUNT] = { 0 };
static bool global_keys_prev[KEY_COUNT] = { 0 };
static FILE *global_stream = NULL;
static GLuint global_default_texture = 0;
static GLuint global_debug_font_texture = 0;

// opengl
typedef BOOL WINAPI wglSwapIntervalEXT_t(int interval);
static wglSwapIntervalEXT_t *wglSwapIntervalEXT;

static bool 
opengl__init(HDC device_context)
{
   PIXELFORMATDESCRIPTOR pfd = { 0 };
   pfd.nSize      = sizeof(PIXELFORMATDESCRIPTOR);
   pfd.nVersion   = 1;
   pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
   pfd.iPixelType = PFD_TYPE_RGBA;
   pfd.cColorBits = 32;
   pfd.iLayerType = PFD_MAIN_PLANE;
   int pixel_format_index = ChoosePixelFormat(device_context, &pfd);
   SetPixelFormat(device_context, pixel_format_index, &pfd);
   HGLRC render_context = wglCreateContext(device_context);
   if (!wglMakeCurrent(device_context, render_context))
      return false;

   wglSwapIntervalEXT = (wglSwapIntervalEXT_t *)wglGetProcAddress("wglSwapIntervalEXT");
   if (wglSwapIntervalEXT)
      wglSwapIntervalEXT(1);

   return true;
}

static int
opengl__create_texture(int width, int height, const void* data)
{
   GLuint id = 0;
   glGenTextures(1, &id);
   glBindTexture(GL_TEXTURE_2D, id);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
   glBindTexture(GL_TEXTURE_2D, 0);
   GLenum err = glGetError();
   if (err != GL_NO_ERROR) { return 0; }
   return (int)id;
}

static void
opengl__projection(const int width, const int height)
{
   global_window_width = width;
   global_window_height = height;

   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, width, height, 0, -1, 1);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}

static void
set_pixel(int width, int height, uint32_t *dst, int x, int y, uint32_t src)
{
   if (x < 0 || x >= width) return;
   if (y < 0 || y >= height) return;
   dst[y * width + x] = src;
}

static void
blit_glyphs(int width, int height, uint32_t *bitmap)
{
   // Source:
   // - https://github.com/dhepper/font8x8
   //
   // license: 
   // - Public Domain
   //
   const unsigned char font8x8_basic[][8] =
   {
      { 0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00},   // U+0021 (!)
      { 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0022 (")
      { 0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00},   // U+0023 (#)
      { 0x0C, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x0C, 0x00},   // U+0024 ($)
      { 0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00},   // U+0025 (%)
      { 0x1C, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x6E, 0x00},   // U+0026 (&)
      { 0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0027 (')
      { 0x18, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x18, 0x00},   // U+0028 (()
      { 0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06, 0x00},   // U+0029 ())
      { 0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00},   // U+002A (*)
      { 0x00, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x00, 0x00},   // U+002B (+)
      { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x06},   // U+002C (,)
      { 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00},   // U+002D (-)
      { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00},   // U+002E (.)
      { 0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00},   // U+002F (/)
      { 0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00},   // U+0030 (0)
      { 0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00},   // U+0031 (1)
      { 0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00},   // U+0032 (2)
      { 0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00},   // U+0033 (3)
      { 0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00},   // U+0034 (4)
      { 0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00},   // U+0035 (5)
      { 0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00},   // U+0036 (6)
      { 0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00},   // U+0037 (7)
      { 0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00},   // U+0038 (8)
      { 0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00},   // U+0039 (9)
      { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00},   // U+003A (:)
      { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x06},   // U+003B (//)
      { 0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00},   // U+003C (<)
      { 0x00, 0x00, 0x3F, 0x00, 0x00, 0x3F, 0x00, 0x00},   // U+003D (=)
      { 0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00},   // U+003E (>)
      { 0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00},   // U+003F (?)
      { 0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00},   // U+0040 (@)
      { 0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00},   // U+0041 (A)
      { 0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00},   // U+0042 (B)
      { 0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00},   // U+0043 (C)
      { 0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00},   // U+0044 (D)
      { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00},   // U+0045 (E)
      { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00},   // U+0046 (F)
      { 0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00},   // U+0047 (G)
      { 0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00},   // U+0048 (H)
      { 0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+0049 (I)
      { 0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00},   // U+004A (J)
      { 0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00},   // U+004B (K)
      { 0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00},   // U+004C (L)
      { 0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00},   // U+004D (M)
      { 0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00},   // U+004E (N)
      { 0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00},   // U+004F (O)
      { 0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00},   // U+0050 (P)
      { 0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00},   // U+0051 (Q)
      { 0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00},   // U+0052 (R)
      { 0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00},   // U+0053 (S)
      { 0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+0054 (T)
      { 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00},   // U+0055 (U)
      { 0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00},   // U+0056 (V)
      { 0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00},   // U+0057 (W)
      { 0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00},   // U+0058 (X)
      { 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00},   // U+0059 (Y)
      { 0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00},   // U+005A (Z)
      { 0x1E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x1E, 0x00},   // U+005B ([)
      { 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40, 0x00},   // U+005C (\)
      { 0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x00},   // U+005D (])
      { 0x08, 0x1C, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00},   // U+005E (^)
      { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF},   // U+005F (_)
      { 0x0C, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0060 (`)
      { 0x00, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x6E, 0x00},   // U+0061 (a)
      { 0x07, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3B, 0x00},   // U+0062 (b)
      { 0x00, 0x00, 0x1E, 0x33, 0x03, 0x33, 0x1E, 0x00},   // U+0063 (c)
      { 0x38, 0x30, 0x30, 0x3e, 0x33, 0x33, 0x6E, 0x00},   // U+0064 (d)
      { 0x00, 0x00, 0x1E, 0x33, 0x3f, 0x03, 0x1E, 0x00},   // U+0065 (e)
      { 0x1C, 0x36, 0x06, 0x0f, 0x06, 0x06, 0x0F, 0x00},   // U+0066 (f)
      { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x1F},   // U+0067 (g)
      { 0x07, 0x06, 0x36, 0x6E, 0x66, 0x66, 0x67, 0x00},   // U+0068 (h)
      { 0x0C, 0x00, 0x0E, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+0069 (i)
      { 0x30, 0x00, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E},   // U+006A (j)
      { 0x07, 0x06, 0x66, 0x36, 0x1E, 0x36, 0x67, 0x00},   // U+006B (k)
      { 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+006C (l)
      { 0x00, 0x00, 0x33, 0x7F, 0x7F, 0x6B, 0x63, 0x00},   // U+006D (m)
      { 0x00, 0x00, 0x1F, 0x33, 0x33, 0x33, 0x33, 0x00},   // U+006E (n)
      { 0x00, 0x00, 0x1E, 0x33, 0x33, 0x33, 0x1E, 0x00},   // U+006F (o)
      { 0x00, 0x00, 0x3B, 0x66, 0x66, 0x3E, 0x06, 0x0F},   // U+0070 (p)
      { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x78},   // U+0071 (q)
      { 0x00, 0x00, 0x3B, 0x6E, 0x66, 0x06, 0x0F, 0x00},   // U+0072 (r)
      { 0x00, 0x00, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x00},   // U+0073 (s)
      { 0x08, 0x0C, 0x3E, 0x0C, 0x0C, 0x2C, 0x18, 0x00},   // U+0074 (t)
      { 0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00},   // U+0075 (u)
      { 0x00, 0x00, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00},   // U+0076 (v)
      { 0x00, 0x00, 0x63, 0x6B, 0x7F, 0x7F, 0x36, 0x00},   // U+0077 (w)
      { 0x00, 0x00, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x00},   // U+0078 (x)
      { 0x00, 0x00, 0x33, 0x33, 0x33, 0x3E, 0x30, 0x1F},   // U+0079 (y)
      { 0x00, 0x00, 0x3F, 0x19, 0x0C, 0x26, 0x3F, 0x00},   // U+007A (z)
      { 0x38, 0x0C, 0x0C, 0x07, 0x0C, 0x0C, 0x38, 0x00},   // U+007B ({)
      { 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00},   // U+007C (|)
      { 0x07, 0x0C, 0x0C, 0x38, 0x0C, 0x0C, 0x07, 0x00},   // U+007D (})
      { 0x6E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+007E (~)
      { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+007F
   };

   const int glyph_atlas_columns = 16;
   const int glyph_atlas_rows = 8;
   const int glyph_count = (sizeof(font8x8_basic) / sizeof(font8x8_basic[0]));
   for (int glyph = 0; glyph < glyph_count; glyph++)
   {
      const int glyph_x = glyph % glyph_atlas_columns * 8;
      const int glyph_y = glyph / glyph_atlas_columns * 8;

      for (int y = 0; y < 8; y++)
      {
         for (int x = 0; x < 8; x++)
         {
            if (font8x8_basic[glyph][y] & 1 << x)
               set_pixel(width, height, bitmap, glyph_x + x, glyph_y + y, 0xffffffff);
            else
               set_pixel(width, height, bitmap, glyph_x + x, glyph_y + y, 0x00000000);
         }
      }
   }
}

void win32_create_debug_font()
{
   uint32_t bitmap[16384] = { 0 };
   blit_glyphs(128, 128, bitmap);
   global_debug_font_texture = opengl__create_texture(128, 128, bitmap);
}

static void
spinach__init_logging(int active)
{
   global_logging_active = active;
   if (global_logging_active)
   {
      freopen_s(&global_stream, "debug.txt", "w", stdout);
   }
}

void spinach_log(const char* format, ...)
{
   if (!global_logging_active) { return; }
   va_list vl;
   va_start(vl, format);
   vfprintf(stdout, format, vl);
   va_end(vl);
}

uint32_t spinach_get_ticks()
{
   static __int64 f = 0, s = 0;
   if (!s)
   {
      QueryPerformanceFrequency((LARGE_INTEGER*)&f);
      QueryPerformanceCounter((LARGE_INTEGER*)&s);
      f /= 1000;
   }
   __int64 c = 0;
   QueryPerformanceCounter((LARGE_INTEGER*)&c);
   __int64 d = c - s;
   return (uint32_t)(d / f);
}

static LRESULT CALLBACK win_window_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   switch (uMsg)
   {
      case WM_MOUSEMOVE:
      {
         global_mouse_position.x_ = GET_X_LPARAM(lParam);
         global_mouse_position.y_ = GET_Y_LPARAM(lParam);
      } break;
      case WM_MOUSEWHEEL:
      {
         //global_mouse_wheel.y = (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
      } break;
      case WM_LBUTTONDOWN:
      case WM_LBUTTONUP:
      {
         global_buttons_curr[MOUSE_BUTTON_LEFT] = uMsg == WM_LBUTTONDOWN ? 1 : 0;
      } break;
      case WM_RBUTTONDOWN:
      case WM_RBUTTONUP:
      {
         global_buttons_curr[MOUSE_BUTTON_RIGHT] = uMsg == WM_RBUTTONDOWN ? 1 : 0;
      } break;

      case WM_KEYDOWN:
      case WM_KEYUP:
      {
         int index = (int)wParam;
         if (index <= 0xff)
            global_keys_curr[index] = uMsg == WM_KEYDOWN ? 1 : 0;
      } break;
      case WM_SIZE:
      {
         opengl__projection(LOWORD(lParam), HIWORD(lParam));
      } break;
      case WM_GETMINMAXINFO:
      {
         RECT cr = { 0 }; cr.right = 1024; cr.bottom = 640;
         AdjustWindowRect(&cr, WS_OVERLAPPEDWINDOW, FALSE);

         MINMAXINFO* mmi = (MINMAXINFO*)lParam;
         mmi->ptMinTrackSize.x = cr.right - cr.left;
         mmi->ptMinTrackSize.y = cr.bottom - cr.top;
      } break;
      case WM_CLOSE:
      {
         PostQuitMessage(0);
      } break;
      default:
      {
         return DefWindowProcA(hWnd, uMsg, wParam, lParam);
      } break;
   }

   return 0;
}

bool spinach_window_init(const char* title, int width, int height)
{
   {
      uint32_t bitmap[] = { 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff };
      global_default_texture = opengl__create_texture(2, 2, bitmap);
   }

   spinach__init_logging(true);
   spinach_get_ticks();

   spinach_log("[init] spinach: because it's healthy for you!\n");

   WNDCLASSA wc = { 0 };
   wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
      wc.lpfnWndProc = win_window_proc;
   wc.hInstance = GetModuleHandle(NULL);
   wc.lpszClassName = "spinachClassName";
   wc.hCursor = LoadCursor(NULL, IDC_ARROW);
   wc.hbrBackground = CreateSolidBrush(0x00000000);
   wc.hIcon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(101));
   if (!RegisterClassA(&wc))
   {
      return 0;
   }

   global_window_width = width;
   global_window_height = height;
   DWORD ws = WS_OVERLAPPEDWINDOW;//(WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU);
   RECT wr = { 0 }; wr.right = width; wr.bottom = height;
   if (!AdjustWindowRect(&wr, ws, 0))
   {
      spinach_log("[init] error: adjust rect\n");
      return false;
   }

   HWND window = global_window_handle = CreateWindowA(wc.lpszClassName,
                                                      title, ws, 
                                                      CW_USEDEFAULT, CW_USEDEFAULT,
                                                      wr.right - wr.left, 
                                                      wr.bottom - wr.top, 
                                                      0, 0, 
                                                      wc.hInstance, 0);
   if (!window)
   {
      spinach_log("[init] error: window\n");
      return false;
   }

   HDC device = GetDC(window);
   if (!opengl__init(device))
   {
      spinach_log("[init] error: opengl create\n");
      return false;
   }

   spinach_log("[init] screen - %dx%d\n", width, height);
   spinach_log("[init] opengl - %s\n", glGetString(GL_VENDOR));
   spinach_log("[init]        - %s\n", glGetString(GL_VERSION));
   spinach_log("[init]        - %s\n", glGetString(GL_RENDERER));
   spinach_log("[init]        - vsync: %s\n", wglSwapIntervalEXT ? "on" : "off");

   /*glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, width, height, 0, -1, 1);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();*/
   
   glEnable(GL_TEXTURE_2D);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   glEnableClientState(GL_COLOR_ARRAY);

   glClearColor(0.1f, 0.2f, 0.2f, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   global_window_title = title;
   global_window_handle = window;
   global_window_device = device;

   ShowWindow(window, SW_SHOWNORMAL);

   spinach_log("[init] ... ok!\n\n");

   return true;
}

bool spinach_window_process()
{
   // note: present back buffer
   SwapBuffers(global_window_device);
   
   // note: clear screen
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   // note: slow things down for the forrest
   //Sleep(5); // todo: remove this sleep

   // note: button-state
   for (int index = 0; index < MOUSE_BUTTON_COUNT; index++)
   {
      global_buttons_prev[index] = global_buttons_curr[index];
   }

   // note: key-state
   for (int index = 0; index < KEY_COUNT; index++)
   {
      global_keys_prev[index] = global_keys_curr[index];
   }

   // note: frame-timing
   static unsigned prev = 0;
   unsigned now = spinach_get_ticks();
   unsigned diff = now - prev;
   prev = now;

   // note: update title-bar
   char title[128];
   sprintf_s(title, 128, "%s [%ums]", global_window_title, diff);
   SetWindowTextA(global_window_handle, title);

   // note: pump os messages
   MSG msg = { 0 };
   while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
   {
      if (msg.message == WM_QUIT)
         return false;

      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }

   return true;
}

spinach_size_t spinach_window_size()
{
   spinach_size_t result = { 0 };
   RECT cr = { 0 };
   GetClientRect(global_window_handle, &cr);
   result.width_ = (cr.right - cr.left);
   result.height_ = (cr.bottom - cr.top);
   return result;
}

int spinach_file_size(const char* filename)
{
   WIN32_FILE_ATTRIBUTE_DATA data = { 0 };
   if (!GetFileAttributesExA(filename, GetFileExInfoStandard, &data)) 
      return 0; 

   return (int)data.nFileSizeLow;
}

bool spinach_file_load(const char* filename, int size, void* dst)
{
   HANDLE hF = CreateFileA(filename, 
                           GENERIC_READ, 
                           FILE_SHARE_READ,
                           NULL, 
                           OPEN_EXISTING, 
                           FILE_ATTRIBUTE_NORMAL, 
                           NULL);
   if (hF == INVALID_HANDLE_VALUE) 
      return false;

   int n = (int)GetFileSize(hF, NULL);
   if (n <= size) 
      ReadFile(hF, dst, n, NULL, NULL);

   CloseHandle(hF);

   return true;
}

spinach_point_t spinach_mouse_position()
{
   return global_mouse_position;
}

bool spinach_button_down(spinach_mouse_button_t button)
{
   return global_buttons_curr[button];
}

bool spinach_button_pressed(spinach_mouse_button_t button)
{
   return global_buttons_curr[button] && !global_buttons_prev[button];
}

bool spinach_button_released(spinach_mouse_button_t button)
{
   return !global_buttons_curr[button] && global_buttons_prev[button];
}

bool spinach_key_down(spinach_keycode_t code)
{
   return global_keys_curr[code];
}

bool spinach_key_pressed(spinach_keycode_t code)
{
   return global_keys_curr[code] && !global_keys_prev[code];
}

bool spinach_key_released(spinach_keycode_t code)
{
   return !global_keys_curr[code] && global_keys_prev[code];
}

spinach_texture_t spinach_texture_load(const char* filename)
{
   int width, height, c;
   stbi_uc* bitmap = stbi_load(filename, &width, &height, &c, 4);
   if (!bitmap)
   {
      spinach_log("[error] could not load texture (%s)\n", filename);
      return (spinach_texture_t){ 0 };
   }

   GLuint id = opengl__create_texture(width, height, bitmap);
   stbi_image_free(bitmap);

   spinach_texture_t result = { 0 };
   result.handle_ = id;
   result.width_ = width;
   result.height_ = height;
   return result;
}

void spinach_texture_destroy(spinach_texture_t *texture)
{
   if (!texture)
      return;

   GLuint id = texture->handle_;
   glDeleteTextures(1, &id);

   texture->handle_ = 0;
   texture->width_ = 0;
   texture->height_ = 0;
}

void spinach_texture_bind(const spinach_texture_t *texture)
{
   static GLuint bound = 0;
   if (!texture)
   {
      bound = 0;
   }
   else if (bound == texture->handle_) 
      return;

   glBindTexture(GL_TEXTURE_2D, bound = texture->handle_);
}

void spinach_render(const int count, const spinach_vertex_t *vertices)
{
   if (count <= 0 || !vertices)
      return;

   const uint8_t *base = (const uint8_t *)vertices;

   glVertexPointer(2, GL_FLOAT, sizeof(spinach_vertex_t), base + offsetof(spinach_vertex_t, position_));
   glTexCoordPointer(2, GL_FLOAT, sizeof(spinach_vertex_t), base + offsetof(spinach_vertex_t, texcoord_));
   glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(spinach_vertex_t), base + offsetof(spinach_vertex_t, color_));
   
   glDrawArrays(GL_QUADS, 0, count);
}
