#ifndef CHATWND_H
#define CHATWND_H

#include <windows.h>

/* Window procedure for our main window */
LRESULT CALLBACK __export ChatWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

/* Register a class for our main window */
BOOL RegisterChatWindowClass(void);

/* Create an instance of our main window */
HWND CreateChatWindow(void);

#endif
