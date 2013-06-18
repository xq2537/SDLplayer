#include "SDL.h"
#include "SDL_syswm.h"
#include "SDL_main.h"

#include "sensor_listener.h"
#include "misc.h"

#ifdef __linux__
#include <X11/Xlib.h>
#endif

#include "socket.h"

#ifdef WIN32
  #include <windows.h>
  #include <tchar.h>
#else

#endif

#define TCP_PORT "22469"

#define USAGE " width height [device]\n"

int AndroVM_initLibrary();
int AndroVM_FrameBuffer_initialize(int w, int h);
#ifdef __APPLE
int AndroVM_FrameBuffer_setupSubWindow(void *id, int w, int h, float zrot);
#elif defined(WIN32)
int AndroVM_FrameBuffer_setupSubWindow(HWND id, int w, int h, float zrot);
HWND AndroVM_FrameBuffer_getSubWindow();
#elif defined(__linux__)
int AndroVM_FrameBuffer_setupSubWindow(Window id, int w, int h, float zrot);
#endif
int AndroVM_FrameBuffer_removeSubWindow();
int AndroVM_RenderServer_create(int p);
int AndroVM_RenderServer_Main();
int AndroVM_RenderServer_start();
int AndroVM_setStreamMode(int);
int AndroVM_setVMIP(char *);
void AndroVM_setOpenGLDisplayRotation(float);
int AndroVM_initOpenGLRenderer(int, int, int, void *, void*);
void AndroVM_setCallbackRotation(void (* fn)(float));
void AndroVM_repaintOpenGLDisplay();
void AndroVM_setDPI(int);

#define STREAM_MODE_DEFAULT   0
#define STREAM_MODE_TCP       1
#define STREAM_MODE_UNIX      2
#define STREAM_MODE_PIPE      3
#define STREAM_MODE_TCPCLI    4

static int g_width, g_height;
static SDL_Surface *g_window_surface = NULL;
static void *g_window_id = NULL;
static float g_rot = 0.0;

static float g_ratio = 1.0;

static char *g_vmip = NULL;

#define USER_EVENT_ROTATION 1
#define USER_EVENT_NEWCLIENT 2

static SDL_Surface *open_window(int width, int height, int density)
{
  SDL_Surface *screen;

  if (SDL_Init(SDL_INIT_VIDEO))
    { 
      fprintf(stderr, "Can't init SDL system\n");
      exit(1);
    }
  char *title = malloc(512 * sizeof(*title));
  if (!title)
    {
      fprintf(stderr, "Out of memory\n");
      exit(1);
  }
  memset(title, 0, 512 * sizeof(*title));

  snprintf(title, 512 * sizeof(*title), "AndroVM Player (%i * %i, %i DPI)", width, height, density);
  SDL_WM_SetCaption(title, 0);

  screen = SDL_SetVideoMode((int)(width*g_ratio), (int)(height*g_ratio), 16, SDL_HWSURFACE);
  
  if (!screen)
    {
      fprintf(stderr, "Can't create window\n");
      exit(1);
    }
  atexit(SDL_Quit);
  return (screen);
}

static void *get_window_id(void)
{
  SDL_SysWMinfo wminfo;
  void *winhandle;

  memset(&wminfo, 0, sizeof(wminfo));
  SDL_GetWMInfo(&wminfo);
#ifdef WIN32
  winhandle = (void *)wminfo.window;
#elif defined(__APPLE__)
  winhandle = (void *)wminfo.nsWindowPtr;
#else
  winhandle = (void *)wminfo.info.x11.window;
#endif
  return (winhandle);
}

static void callbackRotation(float zRot) {
  SDL_Event rot_event;

  rot_event.type = SDL_USEREVENT;
  rot_event.user.code = USER_EVENT_ROTATION;
  rot_event.user.data1 = (int)zRot;
  SDL_PushEvent(&rot_event);
}

static void do_rotation(float zRot) {
  printf("Rotation to : %f\n", zRot);

  switch ((int)zRot) {
   case 0:
   case 180:
      AndroVM_FrameBuffer_removeSubWindow();
      SDL_FreeSurface(g_window_surface);
      g_window_surface = SDL_SetVideoMode((int)(g_width*g_ratio), (int)(g_height*g_ratio), 16, SDL_HWSURFACE);
      if (!g_window_surface) {
          fprintf(stderr, "Unable to recreate Window surface");
          exit(1);
      }
      g_window_id = get_window_id();
      AndroVM_FrameBuffer_setupSubWindow(g_window_id, (int)(g_width*g_ratio), (int)(g_height*g_ratio), zRot);
      break;
   case 90:
   case 270:
      AndroVM_FrameBuffer_removeSubWindow();
      SDL_FreeSurface(g_window_surface);
      g_window_surface = SDL_SetVideoMode((int)(g_height*g_ratio), (int)(g_width*g_ratio), 16, SDL_HWSURFACE);
      if (!g_window_surface) {
          fprintf(stderr, "Unable to recreate Window surface");
          exit(1);
      }
      g_window_id = get_window_id();
      AndroVM_FrameBuffer_setupSubWindow(g_window_id, (int)(g_height*g_ratio), (int)(g_width*g_ratio), zRot);
      break;
   default:
      fprintf(stderr, "Unknown rotation value : %f\n", zRot);  
      break;
  }
  AndroVM_setOpenGLDisplayRotation(zRot);
  //AndroVM_repaintOpenGLDisplay();
  g_rot = zRot;
  printf("Rotation OK\n");
}

int sdlkey2xt(int scancode, int keysym) {
  (void)scancode;
  int keycode = 0;

#ifdef __APPLE__
  /* This is derived partially from SDL_QuartzKeys.h and partially from testing. */
  static const int s_aMacToSet1[] =
  {
     /*  set-1            SDL_QuartzKeys.h    */
        0x1e,        /* QZ_a            0x00 */
        0x1f,        /* QZ_s            0x01 */
        0x20,        /* QZ_d            0x02 */
        0x21,        /* QZ_f            0x03 */
        0x23,        /* QZ_h            0x04 */
        0x22,        /* QZ_g            0x05 */
        0x2c,        /* QZ_z            0x06 */
        0x2d,        /* QZ_x            0x07 */
        0x2e,        /* QZ_c            0x08 */
        0x2f,        /* QZ_v            0x09 */
        0x56,        /* between lshift and z. 'INT 1'? */
        0x30,        /* QZ_b            0x0B */
        0x10,        /* QZ_q            0x0C */
        0x11,        /* QZ_w            0x0D */
        0x12,        /* QZ_e            0x0E */
        0x13,        /* QZ_r            0x0F */
        0x15,        /* QZ_y            0x10 */
        0x14,        /* QZ_t            0x11 */
        0x02,        /* QZ_1            0x12 */
        0x03,        /* QZ_2            0x13 */
        0x04,        /* QZ_3            0x14 */
        0x05,        /* QZ_4            0x15 */
        0x07,        /* QZ_6            0x16 */
        0x06,        /* QZ_5            0x17 */
        0x0d,        /* QZ_EQUALS       0x18 */
        0x0a,        /* QZ_9            0x19 */
        0x08,        /* QZ_7            0x1A */
        0x0c,        /* QZ_MINUS        0x1B */
        0x09,        /* QZ_8            0x1C */
        0x0b,        /* QZ_0            0x1D */
        0x1b,        /* QZ_RIGHTBRACKET 0x1E */
        0x18,        /* QZ_o            0x1F */
        0x16,        /* QZ_u            0x20 */
        0x1a,        /* QZ_LEFTBRACKET  0x21 */
        0x17,        /* QZ_i            0x22 */
        0x19,        /* QZ_p            0x23 */
        0x1c,        /* QZ_RETURN       0x24 */
        0x26,        /* QZ_l            0x25 */
        0x24,        /* QZ_j            0x26 */
        0x28,        /* QZ_QUOTE        0x27 */
        0x25,        /* QZ_k            0x28 */
        0x27,        /* QZ_SEMICOLON    0x29 */
        0x2b,        /* QZ_BACKSLASH    0x2A */
        0x33,        /* QZ_COMMA        0x2B */
        0x35,        /* QZ_SLASH        0x2C */
        0x31,        /* QZ_n            0x2D */
        0x32,        /* QZ_m            0x2E */
        0x34,        /* QZ_PERIOD       0x2F */
        0x0f,        /* QZ_TAB          0x30 */
        0x39,        /* QZ_SPACE        0x31 */
        0x29,        /* QZ_BACKQUOTE    0x32 */
        0x0e,        /* QZ_BACKSPACE    0x33 */
        0x9c,        /* QZ_IBOOK_ENTER  0x34 */
        0x01,        /* QZ_ESCAPE       0x35 */
        0x5c|0x100,  /* QZ_RMETA        0x36 */
        0x5b|0x100,  /* QZ_LMETA        0x37 */
        0x2a,        /* QZ_LSHIFT       0x38 */
        0x3a,        /* QZ_CAPSLOCK     0x39 */
        0x38,        /* QZ_LALT         0x3A */
        0x1d,        /* QZ_LCTRL        0x3B */
        0x36,        /* QZ_RSHIFT       0x3C */
        0x38|0x100,  /* QZ_RALT         0x3D */
        0x1d|0x100,  /* QZ_RCTRL        0x3E */
           0,        /*                      */
           0,        /*                      */
        0x53,        /* QZ_KP_PERIOD    0x41 */
           0,        /*                      */
        0x37,        /* QZ_KP_MULTIPLY  0x43 */
           0,        /*                      */
        0x4e,        /* QZ_KP_PLUS      0x45 */
           0,        /*                      */
        0x45,        /* QZ_NUMLOCK      0x47 */
           0,        /*                      */
           0,        /*                      */
           0,        /*                      */
        0x35|0x100,  /* QZ_KP_DIVIDE    0x4B */
        0x1c|0x100,  /* QZ_KP_ENTER     0x4C */
           0,        /*                      */
        0x4a,        /* QZ_KP_MINUS     0x4E */
           0,        /*                      */
           0,        /*                      */
        0x0d/*?*/,   /* QZ_KP_EQUALS    0x51 */
        0x52,        /* QZ_KP0          0x52 */
        0x4f,        /* QZ_KP1          0x53 */
        0x50,        /* QZ_KP2          0x54 */
        0x51,        /* QZ_KP3          0x55 */
        0x4b,        /* QZ_KP4          0x56 */
        0x4c,        /* QZ_KP5          0x57 */
        0x4d,        /* QZ_KP6          0x58 */
        0x47,        /* QZ_KP7          0x59 */
           0,        /*                      */
        0x48,        /* QZ_KP8          0x5B */
        0x49,        /* QZ_KP9          0x5C */
           0,        /*                      */
           0,        /*                      */
           0,        /*                      */
        0x3f,        /* QZ_F5           0x60 */
        0x40,        /* QZ_F6           0x61 */
        0x41,        /* QZ_F7           0x62 */
        0x3d,        /* QZ_F3           0x63 */
        0x42,        /* QZ_F8           0x64 */
        0x43,        /* QZ_F9           0x65 */
           0,        /*                      */
        0x57,        /* QZ_F11          0x67 */
           0,        /*                      */
        0x37|0x100,  /* QZ_PRINT / F13  0x69 */
        0x63,        /* QZ_F16          0x6A */
        0x46,        /* QZ_SCROLLOCK    0x6B */
           0,        /*                      */
        0x44,        /* QZ_F10          0x6D */
        0x5d|0x100,  /*                      */
        0x58,        /* QZ_F12          0x6F */
           0,        /*                      */
           0/* 0xe1,0x1d,0x45*/, /* QZ_PAUSE        0x71 */
        0x52|0x100,  /* QZ_INSERT / HELP 0x72 */
        0x47|0x100,  /* QZ_HOME         0x73 */
        0x49|0x100,   /* QZ_PAGEUP       0x74 */
        0x53|0x100,  /* QZ_DELETE       0x75 */
        0x3e,        /* QZ_F4           0x76 */
        0x4f|0x100,  /* QZ_END          0x77 */
        0x3c,        /* QZ_F2           0x78 */
        0x51|0x100,  /* QZ_PAGEDOWN     0x79 */
        0x3b,        /* QZ_F1           0x7A */
        0x4b|0x100,  /* QZ_LEFT         0x7B */
        0x4d|0x100,  /* QZ_RIGHT        0x7C */
        0x6c,  /* QZ_DOWN         0x7D */
        0x67,  /* QZ_UP           0x7E */
        0x5e|0x100,  /* QZ_POWER        0x7F */ /* have different break key! */
  };

  if (scancode == 0) {
    /* This could be a modifier or it could be 'a'. */
    switch (keysym) {
     case SDLK_LSHIFT:           keycode = 0x2a; break;
     case SDLK_RSHIFT:           keycode = 0x36; break;
     case SDLK_LCTRL:            keycode = 0x1d; break;
     case SDLK_RCTRL:            keycode = 0x1d | 0x100; break;
     case SDLK_LALT:             keycode = 0x38; break;
     case SDLK_MODE: /* alt gr */
     case SDLK_RALT:             keycode = 0x38 | 0x100; break;
     case SDLK_RMETA:
     case SDLK_RSUPER:           keycode = 0x5c | 0x100; break;
     case SDLK_LMETA:
     case SDLK_LSUPER:           keycode = 0x5b | 0x100; break;
     case SDLK_CAPSLOCK:         keycode = 0x3a; break;
     /* Assumes normal key. */
      default:                   if (keycode < ((sizeof(s_aMacToSet1)/sizeof(int))))
                                     keycode = s_aMacToSet1[keycode];
                                 break;
    }
  }
  else {
    if (scancode < ((sizeof(s_aMacToSet1)/sizeof(int))))
      keycode = s_aMacToSet1[scancode];
    else
      keycode = 0;
  }
#endif

  if (keycode != 0) {
    return keycode;
  }
  else {
    // Fallback
    switch (keysym) {
        case SDLK_ESCAPE:           return 0x01;
        case SDLK_EXCLAIM:
        case SDLK_1:                return 0x02;
        case SDLK_AT:
        case SDLK_2:                return 0x03;
        case SDLK_HASH:
        case SDLK_3:                return 0x04;
        case SDLK_DOLLAR:
        case SDLK_4:                return 0x05;
        /* % */
        case SDLK_5:                return 0x06;
        case SDLK_CARET:
        case SDLK_6:                return 0x07;
        case SDLK_AMPERSAND:
        case SDLK_7:                return 0x08;
        case SDLK_ASTERISK:
        case SDLK_8:                return 0x09;
        case SDLK_LEFTPAREN:
        case SDLK_9:                return 0x0a;
        case SDLK_RIGHTPAREN:
        case SDLK_0:                return 0x0b;
        case SDLK_UNDERSCORE:
        case SDLK_MINUS:            return 0x0c;
        case SDLK_EQUALS:
        case SDLK_PLUS:             return 0x0d;
        case SDLK_BACKSPACE:        return 0x0e;
        case SDLK_TAB:              return 0x0f;
        case SDLK_q:                return 0x10;
        case SDLK_w:                return 0x11;
        case SDLK_e:                return 0x12;
        case SDLK_r:                return 0x13;
        case SDLK_t:                return 0x14;
        case SDLK_y:                return 0x15;
        case SDLK_u:                return 0x16;
        case SDLK_i:                return 0x17;
        case SDLK_o:                return 0x18;
        case SDLK_p:                return 0x19;
        case SDLK_LEFTBRACKET:      return 0x1a;
        case SDLK_RIGHTBRACKET:     return 0x1b;
        case SDLK_RETURN:           return 0x1c;
        case SDLK_KP_ENTER:         return 0x1c | 0x100;
        case SDLK_LCTRL:            return 0x1d;
        case SDLK_RCTRL:            return 0x1d | 0x100;
        case SDLK_a:                return 0x1e;
        case SDLK_s:                return 0x1f;
        case SDLK_d:                return 0x20;
        case SDLK_f:                return 0x21;
        case SDLK_g:                return 0x22;
        case SDLK_h:                return 0x23;
        case SDLK_j:                return 0x24;
        case SDLK_k:                return 0x25;
        case SDLK_l:                return 0x26;
        case SDLK_COLON:
        case SDLK_SEMICOLON:        return 0x27;
        case SDLK_QUOTEDBL:
        case SDLK_QUOTE:            return 0x28;
        case SDLK_BACKQUOTE:        return 0x29;
        case SDLK_LSHIFT:           return 0x2a;
        case SDLK_BACKSLASH:        return 0x2b;
        case SDLK_z:                return 0x2c;
        case SDLK_x:                return 0x2d;
        case SDLK_c:                return 0x2e;
        case SDLK_v:                return 0x2f;
        case SDLK_b:                return 0x30;
        case SDLK_n:                return 0x31;
        case SDLK_m:                return 0x32;
        case SDLK_LESS:
        case SDLK_COMMA:            return 0x33;
        case SDLK_GREATER:
        case SDLK_PERIOD:           return 0x34;
        case SDLK_KP_DIVIDE:        /*??*/
        case SDLK_QUESTION:
        case SDLK_SLASH:            return 0x35;
        case SDLK_RSHIFT:           return 0x36;
        case SDLK_KP_MULTIPLY:
        case SDLK_PRINT:            return 0x37; /* fixme */
        case SDLK_LALT:             return 0x38;
        case SDLK_MODE: /* alt gr*/
        case SDLK_RALT:             return 0x38 | 0x100;
        case SDLK_SPACE:            return 0x39;
        case SDLK_CAPSLOCK:         return 0x3a;
        case SDLK_F1:               return 0x3b;
        case SDLK_F2:               return 0x3c;
        case SDLK_F3:               return 0x3d;
        case SDLK_F4:               return 0x3e;
        case SDLK_F5:               return 0x3f;
        case SDLK_F6:               return 0x40;
        case SDLK_F7:               return 0x41;
        case SDLK_F8:               return 0x42;
        case SDLK_F9:               return 0x43;
        case SDLK_F10:              return 0x44;
        case SDLK_PAUSE:            return 0x45; /* not right */
        case SDLK_NUMLOCK:          return 0x45;
        case SDLK_SCROLLOCK:        return 0x46;
        case SDLK_KP7:              return 0x47;
        case SDLK_HOME:             return 0x66;
        case SDLK_KP8:              return 0x48;
        case SDLK_UP:               return 0x67;
        case SDLK_KP9:              return 0x49;
        case SDLK_PAGEUP:           return 0x49 | 0x100;
        case SDLK_KP_MINUS:         return 0x4a;
        case SDLK_KP4:              return 0x4b;
        case SDLK_LEFT:             return 0x4b | 0x100;
        case SDLK_KP5:              return 0x4c;
        case SDLK_KP6:              return 0x4d;
        case SDLK_RIGHT:            return 0x4d | 0x100;
        case SDLK_KP_PLUS:          return 0x4e;
        case SDLK_KP1:              return 0x4f;
        case SDLK_END:              return 0x4f | 0x100;
        case SDLK_KP2:              return 0x50;
        case SDLK_DOWN:             return 0x6c;
        case SDLK_KP3:              return 0x51;
        case SDLK_PAGEDOWN:         return 0x51 | 0x100;
        case SDLK_KP0:              return 0x52;
        case SDLK_INSERT:           return 0x52 | 0x100;
        case SDLK_KP_PERIOD:        return 0x53;
        case SDLK_DELETE:           return 0x53 | 0x100;
        case SDLK_SYSREQ:           return 0x54;
        case SDLK_F11:              return 0x57;
        case SDLK_F12:              return 0x58;
        case SDLK_F13:              return 0x5b;
        case SDLK_LMETA:
        case SDLK_LSUPER:           return 0x5b | 0x100;
        case SDLK_F14:              return 0x5c;
        case SDLK_RMETA:
        case SDLK_RSUPER:           return 0x5c | 0x100;
        case SDLK_F15:              return 0x5d;
        case SDLK_MENU:             return 0x5d | 0x100;
    }
  }
  return (0);
}

static void convert_mouse_pos(int *p_x, int *p_y) {
  int tmp;

  *p_x = *p_x / g_ratio;
  *p_y = *p_y / g_ratio;
  switch ((int)g_rot) {
   case 90:
     tmp = *p_x;
     *p_x = g_width - *p_y;
     *p_y = tmp;
     break;
   case 180:
     *p_x = g_width - *p_x;
     break;
   case 270:
     tmp = *p_x;
     *p_x = *p_y;
     *p_y = g_height - tmp;
     break;
  }
}

static void event_loop(int w, int h) {
  SDL_Event event;
  char mbuff[256];
  int nx, ny;
  SOCKET client_sock = 0;

end_conn:
  if (client_sock > 0) {
    closesocket(client_sock);
    client_sock = 0;
  }

  while (SDL_WaitEvent(&event)) {
    int ret;
    int xtkey;

    //printf("Got Event : %d\n", event.type);
    switch (event.type) {	 
     case SDL_USEREVENT:
      //printf("Got SDL_USEREVENT with code=%d\n", event.user.code);
      switch (event.user.code) {
       case USER_EVENT_ROTATION:
         printf("Got ROTATE user event with rotation value=%d\n", (int)event.user.data1);
         do_rotation((int)event.user.data1);
	 break;
       case USER_EVENT_NEWCLIENT:
         printf("Got NEWCLIENT uservent with socket=%d\n", (int)event.user.data1);
	 if (client_sock) {
	   printf("Closing old client socket connection...\n");
	   closesocket(client_sock);
	 }
	 client_sock = (SOCKET)event.user.data1;
         sprintf(mbuff, "CONFIG:%d:%d\n", w, h);
         send(client_sock, mbuff, strlen(mbuff), 0);
         break;
      }
      break;
     case SDL_MOUSEMOTION:
      //printf("Got SDL_MOUSEMOTION with x=%d,y=%d,state=%d\n", event.motion.x, event.motion.y,event.motion.state);
      nx = event.motion.x;
      ny = event.motion.y;
      convert_mouse_pos(&nx, &ny);
      if (client_sock) {
        sprintf(mbuff, "MOUSE:%d:%d\n", nx, ny);
        ret = send(client_sock, mbuff, strlen(mbuff), 0);
        if (ret == SOCKET_ERROR) {
	  fprintf(stderr, "Error sending MOUSE command\n");
	  goto end_conn;
        }
      }
      break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
      //printf("Got SDL_MOUSEBUTTON with state=%d button=%d x=%d y=%d\n", event.button.state, event.button.button, event.button.x, event.button.y);
      nx = event.motion.x;
      ny = event.motion.y;
      convert_mouse_pos(&nx, &ny);
      if (client_sock) {
        switch (event.button.button) {
         case SDL_BUTTON_LEFT:
            if (event.button.state == SDL_PRESSED) {
	      sprintf(mbuff, "MSBPR:%d:%d\n", nx, ny);
            }
            else {
	      sprintf(mbuff, "MSBRL:%d:%d\n", nx, ny);
            }
            break;
         case SDL_BUTTON_WHEELUP:
            sprintf(mbuff, "WHEEL:%d:%d:1:0\n", nx, ny);
            break;
         case SDL_BUTTON_WHEELDOWN:
            sprintf(mbuff, "WHEEL:%d:%d:-1:0\n", nx, ny);
            break;
         default:
            *mbuff = 0x00;
            break;
        }
        if (*mbuff) {
            ret = send(client_sock, mbuff, strlen(mbuff), 0);
            if (ret == SOCKET_ERROR)
              goto end_conn;
        }
      }
      break;
    case SDL_ACTIVEEVENT: 
      //printf("Got SDL_ACTIVEEVENT with state=%d\n", event.active.state);
      if (event.active.state & SDL_APPMOUSEFOCUS) {
	//printf("[SDL_ACTIVEEVENT] Got SDL_APPMOUSEFOCUS with gain=%d\n", event.active.gain);
	//SDL_ShowCursor((event.active.gain==1)?SDL_DISABLE:SDL_ENABLE);
      }
      if (event.active.state & SDL_APPINPUTFOCUS) {
	//printf("[SDL_ACTIVEEVENT] Got SDL_APPINPUTFOCUS with gain=%d\n", event.active.gain);
      }
      break;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      xtkey = sdlkey2xt(event.key.keysym.scancode, event.key.keysym.sym);
      //printf("Got SDL_KEY with state=%d code=%d keysym=%d xtkey=%d\n", event.key.state, event.key.keysym.scancode, event.key.keysym.sym, xtkey);
      if (client_sock) {	  
        sprintf(mbuff, "%s:%d:%d\n", (event.key.state==SDL_PRESSED)?"KBDPR":"KBDRL", xtkey, event.key.state);
        ret = send(client_sock, mbuff, strlen(mbuff), 0);
        if (ret == SOCKET_ERROR)
          goto end_conn;
      }
      break;
    case SDL_QUIT:
      printf("Got SDL_QUIT\n");
      exit(0);
      break;
    }
  }
}

#ifdef WIN32
static void input_management()
#else
static void *input_management(void *arg)
#endif
{
    if (!g_vmip) {
      fprintf(stderr, "No VM IP, aborting...\n");
      exit(0);
    }

    printf("[Input Server init OK]\n");
    printf("You shall now start the AndroVM Virtual Machine configured for OpenGL Hardware support - Connecting to the VM\n");

      SOCKET vinput_client_sock;
      do {
        vinput_client_sock = open_socket(g_vmip, 22469);

        if (vinput_client_sock == INVALID_SOCKET) {
	  perror("connect()");
          sleep(1);
        }
      } while (vinput_client_sock == INVALID_SOCKET);
      printf("vinput client connected with socket %d\n", vinput_client_sock);

      SDL_Event conn_event;

      conn_event.type = SDL_USEREVENT;
      conn_event.user.code = USER_EVENT_NEWCLIENT;
      conn_event.user.data1 = (int)vinput_client_sock;
      SDL_PushEvent(&conn_event);	  
}

#ifdef WIN32
static void winexit(void) 
{
  WSACleanup();
}
#endif

#if 0
static void catch_sigint(int sig)
{
  (void)sig;
  exit(1);
}
#endif

#ifdef __APPLE__
int SDL_main(int argc, char **argv)
#elif defined(WIN32)
  int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
#else
  int main(int argc, char **argv)
#endif
{
  int width = 1024;
  int height = 600;
  int dpi = 160;
  void *window_id = 0;

#ifdef WIN32
  WSADATA wsaData;
  int argc = __argc;
  char **argv = __argv;

  // Init windows sockets
  if (WSAStartup(MAKEWORD(2,0), &wsaData)) {
    fprintf(stderr, "Unable to initialize Winsock");
    return(1);
  }

  atexit(&winexit);
#endif

  if (argc < 2)
    exit(1);

  g_vmip = argv[1];

  if (argc >= 3)
    {
      width = atoi(argv[2]);
    }

  if (argc >= 4)
    {
      height = atoi(argv[3]);
    }

  if (argc >= 5)
    {
      dpi = atoi(argv[4]);
    }

  g_width = width;
  g_height = height;
  AndroVM_setDPI(dpi);

#ifdef __linux__
  XInitThreads();
#endif

  g_window_surface = open_window(width, height, dpi);
     
  g_window_id = window_id = get_window_id();
  printf("Window ID: %p\n",window_id);

  if (!AndroVM_initLibrary()) {
    fprintf(stderr, "Unable to initialize Library\n");
    exit(1);
  }

  AndroVM_setStreamMode(STREAM_MODE_TCPCLI);
  AndroVM_setVMIP(g_vmip);
  AndroVM_initOpenGLRenderer( width, height, 25000, NULL, NULL);

  if (!AndroVM_FrameBuffer_setupSubWindow(window_id, (int)(width*g_ratio), (int)(height*g_ratio), 0.0)) {
    fprintf(stderr, "Unable to setup SubWindow\n");
    exit(1);
  }
  AndroVM_setCallbackRotation(callbackRotation);

  printf("[OpenGL init OK]\n");


  if (argc == 6)
   {
     printf("Starting sensor listener\n");
     start_sensor_listener(argv[4]);
   }

#ifdef WIN32
  HANDLE hThread;
  
  hThread = CreateThread(NULL, 0, input_management, 0, 0, NULL);
  if (hThread == NULL) {
    fprintf(stderr, "Unable to start input thread, exiting...\n");
	exit(1);
  }
#else
  pthread_t tid;

  if (pthread_create(&tid, NULL, input_management, NULL) != 0) {
    fprintf(stderr, "Unable to start input thread, exiting...\n");
    exit(1);
  }
#endif
  event_loop(width, height);

  return (0);
}
