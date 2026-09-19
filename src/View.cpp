/// @file View.cpp
/// @author rev

#include "stdafx.h"
#include "View.h"
#include "resource.h"
#define TIMER_ID 1
#define TIMER_TIMEOUT_MS 250

HHOOK	g_CapsKeyboardHook;
HWND g_hWnd;
UINT g_uTaskbarCreatedMsg;

#include <sstream>

void NotifyIcon(DWORD message, unsigned char state, bool from_hook) {
	int icon_id = IDI_C0N0S0;
	bool scrolllock_on = state & 0x1;
	bool numlock_on = state & 0x2;
	bool capslock_on = state & 0x4;
	//	this is legacy, beause I don't need to support Scroll Lock and Num Lock indicators
	//	but I want to leave this code here to be able uncomment it if/when needed
	// switch ( state ) {
	// 	case 0: icon_id = IDI_C0N0S0; break;
	// 	case 1: icon_id = IDI_C0N0S1; break;
	// 	case 2: icon_id = IDI_C0N1S0; break;
	// 	case 3: icon_id = IDI_C0N1S1; break;
	// 	case 4: icon_id = IDI_C1N0S0; break;
	// 	case 5: icon_id = IDI_C1N0S1; break;
	// 	case 6: icon_id = IDI_C1N1S0; break;
	// 	case 7: icon_id = IDI_C1N1S1; break;
	// 	default: break;
	// }
	if (capslock_on) {
		icon_id = from_hook? IDI_FLAGUS : IDI_FLAGRU;
	}
	else {
		icon_id = from_hook ? IDI_FLAGRU : IDI_FLAGUS;
	}
	NOTIFYICONDATA nid = { 0 };
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = g_hWnd;
	nid.uID = IDW_MAIN;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = MSG_TRAYICON;
	nid.hIcon = (HICON)(::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(icon_id), IMAGE_ICON,
		::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0));

	lstrcpy(nid.szTip, _T("CapsTray"));
	Shell_NotifyIcon(message, &nid);
}

LRESULT CALLBACK KbdHook(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode < 0) {
		return CallNextHookEx(g_CapsKeyboardHook, nCode, wParam, lParam);
	}
	if (nCode == HC_ACTION) {
		KBDLLHOOKSTRUCT *ks = (KBDLLHOOKSTRUCT*)lParam;
		if (ks->vkCode == VK_CAPITAL && (GetKeyState(VK_SHIFT) >= 0)) {
			if (wParam == WM_KEYDOWN) {
				unsigned char state = CView::GetLockStates();
				NotifyIcon(NIM_MODIFY, state, true);
				return CallNextHookEx(g_CapsKeyboardHook, nCode, wParam, lParam);
				//return true; // without calling CallNextHookEx event processing stops here
			}
		}
	}
skip:
	return CallNextHookEx(g_CapsKeyboardHook, nCode, wParam, lParam);
}

void CView::OnCreate( void ) {
	// OnCreate is called automatically during window creation when a
	// WM_CREATE message received.
	// Tasks such as setting the icon, creating child windows, or anything
	// associated with creating windows are normally performed here.
	g_uTaskbarCreatedMsg = RegisterWindowMessageW(L"TaskbarCreated");
	::SetTimer( this->GetHwnd(), TIMER_ID, TIMER_TIMEOUT_MS, NULL );
	g_hWnd = m_hWnd;
	g_CapsKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KbdHook, GetModuleHandle(0), 0);
	NotifyIcon( NIM_ADD, m_capsState, false );
	ShowWindow( SW_HIDE );
}

void CView::OnAbout( void ) {
	std::wstringstream oss;
	oss << _T( "CapsTray, by Ron Wilson (c) 2010"
			"\nEnhanced by Konstantin Lvov (konstlvov@github :) (c) 2023"
			"\nRelease 2023-07-26"
			"\n"
			"\nWhat's new"
			"\nRelease 2023-07-26:"
			"\n  * WM_TIMER handling restored"
			) << std::endl;
	MessageBox( oss.str().c_str(), _T( "About CapsTray" ), MB_OK | MB_ICONINFORMATION );
}

void CView::OnDestroy( void ) {	
	UnhookWindowsHookEx(g_CapsKeyboardHook);
	::PostQuitMessage( 0 );
}

void CView::OnTrayIcon( WPARAM wParam, LPARAM lParam ) {
	// For a NOTIFYICONDATA with uVersion= 0, wParam and lParam have the following values:
	// The wParam parameter contains the identifier of the taskbar icon in which the event occurred.
	// The lParam parameter holds the mouse or keyboard message associated with the event.
	if ( wParam != IDW_MAIN ) {
		return;
	}
	if ( lParam == WM_RBUTTONUP ) {
		HMENU hTopMenu = LoadMenu( GetApp()->GetResourceHandle(), MAKEINTRESOURCE( IDM_MINIMIZED ) );
		HMENU hSubMenu = GetSubMenu( hTopMenu, 0 );
		SetForegroundWindow();
		CPoint pt = GetCursorPos();
		UINT uSelected = TrackPopupMenu( hSubMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, m_hWnd, NULL );
		switch ( uSelected ) {
			case IDM_MIN_ABOUT:
				OnAbout();
				break;
			case IDM_MIN_EXIT:
				Destroy();
				break;
		}
	}
}

void CView::PreCreate( CREATESTRUCT& cs ) {
	// This function will be called automatically by Create. It provides an
	// opportunity to set various window parameters prior to window creation.
	// You are not required to set these parameters, any paramters which
	// aren't specified are set to reasonable defaults.

	// Set some optional parameters for the window
	cs.dwExStyle = WS_EX_TOOLWINDOW;       // tool windows do not show up in the task bar
	cs.style = WS_ICONIC;                  // start the window minimized
	cs.lpszName = LoadString( IDW_MAIN );  // Window title
	cs.hMenu =  LoadMenu( GetApp()->GetResourceHandle(), MAKEINTRESOURCE( IDM_MINIMIZED ) );
}

void CView::RemoveIcon( void ) {
	NOTIFYICONDATA nid = { 0 };
	nid.cbSize = sizeof( NOTIFYICONDATA );
	nid.hWnd = m_hWnd;
	nid.uID = IDW_MAIN;
	Shell_NotifyIcon( NIM_DELETE, &nid );
}

LRESULT CView::WndProc( UINT uMsg, WPARAM wParam, LPARAM lParam ) {
	// This function is our message procedure. We process the messages for
	// the view window here.  Unprocessed messages are passed on for
	// default processing.
	if (uMsg == g_uTaskbarCreatedMsg) {
		NotifyIcon(NIM_ADD, m_capsState, false);
		return 0;
	}
	switch ( uMsg ) {
		case WM_TIMER:
			OnTimer();
			break;
		case WM_DESTROY:
			RemoveIcon();
			OnDestroy();
			return 0;
		case MSG_TRAYICON:
			OnTrayIcon( wParam, lParam );
			break;
		case WM_SYSCOMMAND:
			// wierd scenario - if you right-click the icon, choose 'About', hit enter
			// to close the About MessageBox, then hit enter again, the system restores
			// the main form, which is bad behavior for this app because the main form
			// is not designed to be seen.  trap the message here and drop it on the floor.
			if ( wParam == SC_RESTORE ) {
				return 0;
			}
			break;
	}
	// pass unhandled messages on for default processing
	return WndProcDefault( uMsg, wParam, lParam );
}

unsigned char CView::GetLockStates( void ) {
	bool scrolllock_on = 0 != ( ::GetKeyState( VK_SCROLL )  & 0x0001 );
	bool numlock_on    = 0 != ( ::GetKeyState( VK_NUMLOCK ) & 0x0001 );
	bool capslock_on   = 0 != ( ::GetKeyState( VK_CAPITAL ) & 0x0001 );
	unsigned char state = scrolllock_on ? 0x1 : 0x0;
	state |= numlock_on ? 0x2 : 0x0;
	state |= capslock_on ? 0x4 : 0x0;
	return state;
}

void CView::OnTimer( void )
{
	unsigned char state = GetLockStates();
	if ( m_capsState != state )
	{
		m_capsState = state;
		NotifyIcon( NIM_MODIFY, m_capsState, false);
	}
}

