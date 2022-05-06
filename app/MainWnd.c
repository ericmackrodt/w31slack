#include "AboutDlg.h"
#include "Globals.h"
#include "MainWnd.h"
#include "Resource.h"
#include <stdio.h>
#include <stdlib.h>
#include <windowsx.h>
#include <string.h>
#include <time.h>

#include "restapi/restapi.h"
#include "jsnparse/jsnparse.h"

#define CONFIG_FILE_TO_OPEN "ovim.ini"
#define IDS_FILE "userchan.ids"

#define IP_MAX 20

#define MAX_GLOBAL_MEM_ALLOCATION 32000

#define SINGLE_SHOT_TIMER_ID 4
#define REFRESH_TIMER_ID 6
#define CHAT_ROOMS_BOX_ID 5
#define NICKNAME_TEXT_FIELD_ID 8

#define MAX_NICKNAME_BOX 50
#define MAX_NICKNAME_TO_SEND MAX_NICKNAME_BOX * 3

/* Main window class and title */
static const char MainWndClass[] = "Old'aVista Instant Messenger";

HWND hwnd;
HWND nicknameTextField;
HWND joinButton;
HWND chatRoomsBox;

RECT statusRect;

int refreshRate = 1000;
char ip[IP_MAX];
int port;
char statusText[MAX_NICKNAME_TO_SEND + 20];
int maxMessagesToParse = 5;

BOOL chatRoomsObtained = FALSE;

ChatRoomList chatRoomList = {NULL, 0};

int currentSelectedRoom = 0;

LPSTR lpGlobalMemory;
DWORD allocatedMemorySize;

HBRUSH hbrush;

BOOL openAndProcessConfigFile(char *filename)
{
  FILE *configFile;
  char buff[20];

  configFile = fopen(filename, "r");
  if (configFile == NULL)
  {
    return FALSE;
  }

  fgets(buff, 10, configFile);
  refreshRate = atoi(buff);

  fgets(ip, IP_MAX, configFile);
  // Remove trailing newline
  ip[strcspn(ip, "\r\n")] = '\0';

  fgets(buff, 6, configFile);
  port = atoi(buff);

  fgets(buff, 6, configFile);
  maxMessagesToParse = atoi(buff);

  fclose(configFile);

  return TRUE;
}

void showToStatus(char *action, char *data)
{

  time_t rawtime;
  struct tm *timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);

  sprintf(statusText, "%02d:%02d:%02d: %s %s\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, action, data);
  OutputDebugString(statusText);

  // Remove newline
  statusText[strlen(statusText) - 1] = '\0';
  InvalidateRect(hwnd, &statusRect, FALSE);
}

void updateChatRoomsUI()
{
  int i;
  showToStatus("we got here", "");
  SendMessage(GetDlgItem(hwnd, CHAT_ROOMS_BOX_ID), LB_RESETCONTENT, 0, 0);

  for (i = 0; i < chatRoomList.numRooms; i++)
  {
    showToStatus(chatRoomList.rooms[i].name, "");
    SendMessage(GetDlgItem(hwnd, CHAT_ROOMS_BOX_ID), LB_ADDSTRING, 0, (LPARAM)((LPSTR)chatRoomList.rooms[i].name));
  }
}

void updateChatRoomList()
{

  DWORD bytesReceived;

  showToStatus("Updating chat room list", "");

  bytesReceived = restapi_getChatRooms(ip, port, lpGlobalMemory, allocatedMemorySize);

  if (bytesReceived > 0)
  {
    jsnparse_freeChatRoomList(&chatRoomList);
    jsnparse_parseChatRoomList(lpGlobalMemory, bytesReceived, &chatRoomList);

    updateChatRoomsUI();

    chatRoomsObtained = TRUE;
  }
  else
  {
    showToStatus("Cannot retrieve chat rooms list", "");
  }
}

void sendSingleShotUpdateTimer(HWND hwnd)
{
  SetTimer(hwnd, SINGLE_SHOT_TIMER_ID, 0, NULL);
}

/* Window procedure for our main window */
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

  HDC hdc;
  PAINTSTRUCT ps;

  char settingsText[80];

  switch (msg)
  {
  case WM_CREATE:
    showToStatus("Initializing Old'aVista IM...", "");
    lpGlobalMemory = GlobalAllocPtr(GMEM_MOVEABLE, MAX_GLOBAL_MEM_ALLOCATION);
    allocatedMemorySize = GlobalSize(GlobalPtrHandle(lpGlobalMemory));

    hbrush = CreateSolidBrush(RGB(255, 255, 255));

    return 0;
  case WM_PAINT:
    hdc = BeginPaint(hwnd, &ps);
    sprintf(settingsText, "Proxy %s:%d, Refresh %dms, Get %d msgs", ip, port, refreshRate, maxMessagesToParse);

    SetRect(&statusRect, 20, 357, 430, 374);
    FillRect(hdc, &statusRect, hbrush);
    DrawText(hdc, settingsText, strlen(settingsText), &statusRect, DT_LEFT | DT_NOCLIP);

    SetRect(&statusRect, 20, 380, 430, 397);
    FillRect(hdc, &statusRect, hbrush);
    DrawText(hdc, statusText, strlen(statusText), &statusRect, DT_LEFT | DT_NOCLIP);

    EndPaint(hwnd, &ps);

    return 0;

  case WM_TIMER:
    switch (wParam)
    {
    case SINGLE_SHOT_TIMER_ID:
      KillTimer(hwnd, SINGLE_SHOT_TIMER_ID);
    case REFRESH_TIMER_ID:
      updateChatRoomList();
      return 0;
    }

  case WM_COMMAND:
  {

    if (((HWND)lParam) == chatRoomsBox && (HIWORD(lParam) == LBN_SELCHANGE))
    {
      currentSelectedRoom = (WORD)SendMessage(chatRoomsBox, LB_GETCURSEL, 0, 0L);

      showToStatus("Selected channel:", chatRoomList.rooms[currentSelectedRoom].name);
      return 0;
    }

    if (((HWND)lParam) == joinButton && (HIWORD(wParam) == BN_CLICKED))
    {
      // sendMessage(hwnd);
      sendSingleShotUpdateTimer(hwnd);
      return 0;
    }

    switch (wParam)
    {
    case ID_HELP_ABOUT:
    {
      ShowAboutDialog(hwnd);
      return 0;
    }

    case ID_FILE_EXIT:
    {
      DestroyWindow(hwnd);
      return 0;
    }
    }
    break;
  }

  case WM_GETMINMAXINFO:
  {
    /* Prevent our window from being sized too small */
    MINMAXINFO FAR *minMax = (MINMAXINFO FAR *)lParam;
    minMax->ptMinTrackSize.x = 450;
    minMax->ptMinTrackSize.y = 450;

    return 0;
  }

  /* Item from system menu has been invoked */
  case WM_SYSCOMMAND:
  {
    WORD id = wParam;

    switch (id)
    {
    /* Show "about" dialog on about system menu item */
    case ID_HELP_ABOUT:
    {
      ShowAboutDialog(hwnd);
      return 0;
    }
    }
    break;
  }
  case WM_DESTROY:
  {

    DeleteObject(hbrush);
    GlobalFreePtr(lpGlobalMemory);
    PostQuitMessage(0);
    return 0;
  }
  }

  return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* Register a class for our main window */
BOOL RegisterMainWindowClass()
{
  WNDCLASS wc;

  /* Class for our main window */
  wc.style = 0;
  wc.lpfnWndProc = &MainWndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = g_hInstance;
  wc.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_APPICON));
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszMenuName = MAKEINTRESOURCE(IDR_MAINMENU);
  wc.lpszClassName = MainWndClass;

  return (RegisterClass(&wc)) ? TRUE : FALSE;
}

/* Create an instance of our main window */
HWND CreateMainWindow()
{
  /* Create instance of main window WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX*/
  hwnd = CreateWindowEx(0, MainWndClass, MainWndClass, WS_OVERLAPPED, CW_USEDEFAULT, CW_USEDEFAULT, 360, 360,
                        NULL, NULL, g_hInstance, NULL);

  if (hwnd)
  {
    /* Add "about" to the system menu */
    HMENU hSysMenu = GetSystemMenu(hwnd, FALSE);
    InsertMenu(hSysMenu, 5, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
    InsertMenu(hSysMenu, 6, MF_BYPOSITION, ID_HELP_ABOUT, "About");

    chatRoomsBox = CreateWindow("LISTBOX", "", WS_VSCROLL | WS_TABSTOP | WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL | WS_HSCROLL, 20, 20, 100, 300, hwnd, CHAT_ROOMS_BOX_ID, g_hInstance, NULL);

    nicknameTextField = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER, 20, 325, 330, 25, hwnd, NICKNAME_TEXT_FIELD_ID, g_hInstance, NULL);

    joinButton = CreateWindow(
        "BUTTON",                                              // Predefined class; Unicode assumed
        "Join",                                                // Button text
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, // Styles
        365,                                                   // x position
        325,                                                   // y position
        60,                                                    // Button width
        25,                                                    // Button height
        hwnd,                                                  // Parent window
        NULL,                                                  // No menu.
        g_hInstance,
        NULL); // Pointer not needed.
  }

  if (openAndProcessConfigFile(CONFIG_FILE_TO_OPEN))
  {
    if (refreshRate > 0)
    {
      SetTimer(hwnd, REFRESH_TIMER_ID, refreshRate, NULL);
    }
  }
  else
  {
    MessageBox(hwnd, "Cannot open file ovim.ini containing token, refresh rate (ms), HTTP Proxy IP, port", "Config File Error", MB_OK);
  }
  return hwnd;
}
