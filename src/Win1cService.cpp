// Win1cService.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "resource.h"
#include "stdio.h"
#include "psapi.h"

#pragma comment (lib, "Psapi.lib")

void ServiceMain(DWORD argc, LPTSTR *argv); 
void ServiceCtrlHandler(DWORD nControlCode);
BOOL UpdateServiceStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode,
					 DWORD dwServiceSpecificExitCode, DWORD dwCheckPoint,
					 DWORD dwWaitHint);
BOOL StartServiceThread();
DWORD ServiceExecutionThread1(LPDWORD param);
DWORD ServiceExecutionThread2(LPDWORD param);

HANDLE hServiceThreadWindow;
HANDLE hServiceThread;
void KillService();

//name of service
TCHAR Win1cServiceName[] = _T("Win1cService");

SERVICE_STATUS_HANDLE nServiceStatusHandle; 
HANDLE killServiceEvent;
BOOL nServiceRunning;
DWORD nServiceCurrentStatus;

string basePath;
string FuncName;

HWND hwnd = NULL;

NOTIFYICONDATA	ndata;	// notify icon data

#define SWM_TRAYMSG	WM_APP//		the message ID sent to our window

#define SWM_SHOW	WM_APP + 1//	show the window
#define SWM_HIDE	WM_APP + 2//	hide the window
#define SWM_EXIT	WM_APP + 3//	close the window

BOOL CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);
void ShowContextMenu(HWND hWnd);

int _tmain(int argc, _TCHAR* argv[]){
//StartServiceThread();
//while (nServiceRunning) Sleep(1000);

	SERVICE_TABLE_ENTRY servicetable[]=
	{
		{Win1cServiceName,(LPSERVICE_MAIN_FUNCTION)ServiceMain},
		{NULL,NULL}
	};
	if ( (argc > 1) &&
			((*argv[1] == '-') || (*argv[1] == '/')) )
		{
			SC_HANDLE Win1cService, scm;
			if ( _wcsicmp(_T("install"), argv[1]+1 ) == 0 )
			{
				long idProc = GetCurrentProcessId();	  
				HANDLE hProcModule = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, idProc);
				if( !hProcModule ){return -1;}

				DWORD dwModuleSize = 0;
				HMODULE hModules[100] = { 0 };

				EnumProcessModules( hProcModule, hModules, sizeof( hModules ), &dwModuleSize );
				dwModuleSize /= sizeof( HMODULE );
				if( !dwModuleSize ){return -1;}

				TCHAR szName[MAX_PATH];
				GetModuleFileNameEx(hProcModule, hModules[0], szName, MAX_PATH ); 
				
				scm=OpenSCManager(0,0,SC_MANAGER_CREATE_SERVICE);
				if(!scm)
				{
					MessageBox(NULL, _T("Can't open Service Control Manager"), _T("Error"),
						   MB_OK | MB_ICONERROR);
	            
					return -1;
				}
				Win1cService=CreateService(scm, Win1cServiceName,
					_T("Win1c's exchange service"),
					SERVICE_ALL_ACCESS,SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS, 
					SERVICE_DEMAND_START, // SERVICE_AUTO_START
					SERVICE_ERROR_NORMAL,
					szName,
					0,0,0,0,0);
				if(!Win1cService)
				{
					CloseServiceHandle(scm);
					return -1;
				}
				CloseServiceHandle(Win1cService);
				CloseServiceHandle(scm);
			}
			else if ( _wcsicmp(_T("remove"), argv[1]+1 ) == 0 )
			{
				 scm=OpenSCManager(0,0,SC_MANAGER_CREATE_SERVICE);
				 if(!scm) {
					MessageBox(NULL, _T("Can't open Service Control Manager"), _T("Error"),
							   MB_OK | MB_ICONERROR);
					return -1;
				 }
				 Win1cService = OpenService(scm, Win1cServiceName, DELETE);
				 if(!Win1cService) {
					return -1;
				 }
				 SERVICE_STATUS ssStatus;
				 // try to stop the service
					if ( ControlService( Win1cService, SERVICE_CONTROL_STOP, &ssStatus ) )
					{
						printf("Stopping IconService.");
						Sleep( 1000 );

						while( QueryServiceStatus( Win1cService, &ssStatus ) )
						{
							if ( ssStatus.dwCurrentState == SERVICE_STOP_PENDING )
							{
								printf(".");
								Sleep( 1000 );
							}
							else
								break;
						}

						if ( ssStatus.dwCurrentState == SERVICE_STOPPED )
							printf("\nIconService stopped.\n");
						else
						{printf("\nFailed to stop IconService.\n");
							return -1;
						}

					}
				 
				 //удаляем сервис из системы
				 if (!DeleteService(Win1cService) ) 
					MessageBox(NULL, _T("DeleteService failed"), _T("Error"), MB_OK | MB_ICONERROR );
				 else 
					MessageBox(NULL, _T("Service deleted successfully"), _T("Information"), MB_OK | MB_ICONERROR);
		         
				 CloseServiceHandle(Win1cService);
				 CloseServiceHandle(scm); 
			}
			return 0;
		}

	BOOL success;
	success=StartServiceCtrlDispatcher(servicetable);
	if(!success)
	{
		MessageBox(NULL, _T("Can't start service"), _T("Error"), MB_OK | MB_ICONERROR);
	}
	
	return 0;
}

void ServiceMain(DWORD argc, LPTSTR *argv)
{
	/*freopen("c:\\input.txt", "w", stdout);
	printf("Params:\n");
	for (int i = 0; i < argc; ++i){
		
		wcslen(argv[i]);
		
		printf("\n");
	}*/

	
	BOOL success;
	nServiceStatusHandle=RegisterServiceCtrlHandler(Win1cServiceName, (LPHANDLER_FUNCTION)ServiceCtrlHandler);
	if(!nServiceStatusHandle)
	{
		return;
	}
	success=UpdateServiceStatus(SERVICE_START_PENDING,NO_ERROR,0,1,3000);
	if(!success)
	{
		return;
	}
	killServiceEvent=CreateEvent(0,TRUE,FALSE,0);
	if(killServiceEvent==NULL)
	{
		return;
	}
	success=UpdateServiceStatus(SERVICE_START_PENDING,NO_ERROR,0,2,1000);
	if(!success)
	{
		return;
	}
	success=StartServiceThread();
	if(!success)
	{
		return;
	}
	nServiceCurrentStatus=SERVICE_RUNNING;
	success=UpdateServiceStatus(SERVICE_RUNNING,NO_ERROR,0,0,0);
	if(!success)
	{
		return;
	}
	WaitForSingleObject(killServiceEvent,INFINITE);
	CloseHandle(killServiceEvent);
}



BOOL UpdateServiceStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode,
					 DWORD dwServiceSpecificExitCode, DWORD dwCheckPoint,
					 DWORD dwWaitHint)
{
	BOOL success;
	SERVICE_STATUS nServiceStatus;
	nServiceStatus.dwServiceType=SERVICE_WIN32_OWN_PROCESS;
	nServiceStatus.dwCurrentState=dwCurrentState;
	if(dwCurrentState==SERVICE_START_PENDING)
	{
		nServiceStatus.dwControlsAccepted=0;
	}
	else
	{
		nServiceStatus.dwControlsAccepted=SERVICE_ACCEPT_STOP			
			|SERVICE_ACCEPT_SHUTDOWN;
	}
	if(dwServiceSpecificExitCode==0)
	{
		nServiceStatus.dwWin32ExitCode=dwWin32ExitCode;
	}
	else
	{
		nServiceStatus.dwWin32ExitCode=ERROR_SERVICE_SPECIFIC_ERROR;
	}
	nServiceStatus.dwServiceSpecificExitCode=dwServiceSpecificExitCode;
	nServiceStatus.dwCheckPoint=dwCheckPoint;
	nServiceStatus.dwWaitHint=dwWaitHint;

	success=SetServiceStatus(nServiceStatusHandle,&nServiceStatus);

	if(!success)
	{
		KillService();
		return success;
	}
	else
		return success;
}

void CharToWChar(WCHAR* dest, const char* src){
    DWORD len = MultiByteToWideChar(CP_ACP, 0, src, -1, NULL, 0);
    MultiByteToWideChar(CP_ACP,0, src, -1, dest, len);
}
void WCharToChar(WCHAR* src, char* dest){
	dest = new char[wcslen(src)];
	WideCharToMultiByte(CP_ACP, 0, src, -1, dest, 256, NULL, NULL );
}


BOOL StartServiceThread()
{	
	//freopen("logService.txt", "w", stdout); //Отладка

	SC_HANDLE scm;
    SC_HANDLE Win1cService;
    LPQUERY_SERVICE_CONFIG lpsc; 
    DWORD dwBytesNeeded, cbBufSize, dwError; 

	scm=OpenSCManager(0,0,SC_MANAGER_ALL_ACCESS);
	if(!scm) {
		MessageBox(NULL, _T("Can't open Service Control Manager"), _T("Error"),
				   MB_OK | MB_ICONERROR);
		return -1;
	}
	Win1cService = OpenService(scm, Win1cServiceName, SERVICE_QUERY_CONFIG);
	if(!Win1cService) {
		return -1;
	}

	if( !QueryServiceConfig( 
        Win1cService, 
        NULL, 
        0, 
        &dwBytesNeeded))
    {
        dwError = GetLastError();
        if( ERROR_INSUFFICIENT_BUFFER == dwError )
        {
            cbBufSize = dwBytesNeeded;
            lpsc = (LPQUERY_SERVICE_CONFIG) LocalAlloc(LMEM_FIXED, cbBufSize);
        }
        else
        {
            printf("QueryServiceConfig failed (%d)", dwError);
			CloseServiceHandle(Win1cService); 
			CloseServiceHandle(scm);
			return FALSE;
        }
    }
  
    if( !QueryServiceConfig( 
        Win1cService, 
        lpsc, 
        cbBufSize, 
        &dwBytesNeeded) ) 
    {
        printf("QueryServiceConfig failed (%d)", GetLastError());
		CloseServiceHandle(Win1cService); 
		CloseServiceHandle(scm);
		return FALSE;
    }

	wstring _Path = lpsc->lpBinaryPathName;

	string ConfigFile;
	string tmp;
	tmp.resize(_Path.size());
	transform(_Path.begin(),_Path.end(),tmp.begin(),wctob);
	tmp.swap(ConfigFile);
	
	size_t found;
	found=ConfigFile.find_last_of('\\');
	ConfigFile = ConfigFile.substr(0, found + 1) + "config.ini";

	//MessageBoxA(NULL, ConfigFile.c_str(), "", MB_OK);

	basePath = CIniFile::GetValue("Base","Main", ConfigFile);
	basePath = basePath + "Pwd=\"pass\";";
	//File="F:\Bases\Win1c";Usr="Administrator";Pwd="pass";

	FuncName = CIniFile::GetValue("Function","Main", ConfigFile);
	
	LocalFree(lpsc); 

	CloseServiceHandle(Win1cService); 
    CloseServiceHandle(scm);

	DWORD id1, id2;
	hServiceThread = CreateThread(0,0, (LPTHREAD_START_ROUTINE)ServiceExecutionThread1, 0,0,&id1);
	hServiceThreadWindow = CreateThread(0,0, (LPTHREAD_START_ROUTINE)ServiceExecutionThread2, 0,0,&id2);

	if(hServiceThread == 0 || hServiceThreadWindow == 0)
	{
		return false;
	}
	else
	{
		nServiceRunning=true;
		return true;
	}
}

int InitCom() {
	CoInitialize(NULL);
	
	try {
	CLSID clsidV8App;
	
	IDispatch* pDispApp;
	DISPID dispIDConnect;
	DISPID dispIDCommand;

	HRESULT hr=::CLSIDFromProgID(L"v81.COMConnector", &clsidV8App);
	hr=::CoCreateInstance(clsidV8App, NULL, CLSCTX_INPROC_SERVER, IID_IDispatch, (void**) &pDispApp); 
	if(FAILED(hr)){
		MessageBox(0,_T(""),_T(""),MB_OK);
		CoUninitialize();
		return -1;
	}
	
	BSTR bstrConnect = L"Connect";
	hr = pDispApp->GetIDsOfNames(IID_NULL, &bstrConnect, 1, LOCALE_SYSTEM_DEFAULT, &dispIDConnect);
	if (FAILED(hr)){
		if (pDispApp)
			pDispApp->Release();
		CoUninitialize();
		return -2;
	}

	VARIANT vRet;
	DISPPARAMS args = {NULL, NULL, 0, 0};
	VARIANT vars[1];  

	args.cArgs = 1;
	args.rgvarg = vars;

	VariantInit(args.rgvarg);
	args.rgvarg[0].vt = VT_BSTR;
	
	WCHAR *t = new WCHAR[basePath.size()];
	CharToWChar(t, basePath.c_str());
	args.rgvarg[0].bstrVal = SysAllocString(t);

	EXCEPINFO pExceptInfo;
	hr = pDispApp->Invoke(dispIDConnect, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &args, &vRet, &pExceptInfo, NULL);
 	if (FAILED(hr)){
		if (pDispApp)
			pDispApp->Release();
		if (hr = DISP_E_EXCEPTION) {
			MessageBoxA(0, basePath.c_str(), "", MB_OK|MB_ICONERROR);
			MessageBox(0, args.rgvarg[0].bstrVal, _T(""), MB_OK|MB_ICONERROR);
			MessageBox(0, pExceptInfo.bstrDescription, _T(""), MB_OK|MB_ICONERROR);
		}
		else {
			char* mess = new char[100];
			sprintf(mess, "%x", hr);
			MessageBoxA(0, mess, "", MB_OK|MB_ICONERROR);
		}
		CoUninitialize();
		return -3;
	}

	pDispApp = vRet.pdispVal;
	
	BSTR bstrCommand = L"ПараметрыСеанса";
	hr = pDispApp->GetIDsOfNames(IID_NULL, &bstrCommand, 1, LOCALE_SYSTEM_DEFAULT, &dispIDCommand);

	args.rgdispidNamedArgs = NULL;
	args.rgvarg = NULL;
	args.cArgs = 0;
	args.cNamedArgs = 0;

	pDispApp->AddRef();
	hr = pDispApp->Invoke(dispIDCommand, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &args, &vRet, NULL, NULL);

	IDispatch* pParam;
	pParam = vRet.pdispVal;
	
	bstrCommand = L"ТекущийСклад";
	hr = pParam->GetIDsOfNames(IID_NULL, &bstrCommand, 1, LOCALE_SYSTEM_DEFAULT, &dispIDCommand);
	hr = pParam->Invoke(dispIDCommand, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &args, &vRet, NULL, NULL);

	pParam = vRet.pdispVal;
	
	bstrCommand = L"ПериодОпросаНаИзменения";
	hr = pParam->GetIDsOfNames(IID_NULL, &bstrCommand, 1, LOCALE_SYSTEM_DEFAULT, &dispIDCommand);
	hr = pParam->Invoke(dispIDCommand, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &args, &vRet, NULL, NULL);
		
	int stop = 1;
	while (stop > 0) stop = pDispApp->Release();

	CoUninitialize();

	return vRet.intVal;
	}
	catch (...){
		return -4;
	}

	return -5;
	
}

BSTR DoFunction() {
	CoInitialize(NULL);

	try {
	CLSID clsidV8App;
	
	IDispatch* pDispApp;
	DISPID dispIDConnect;
	DISPID dispIDCommand;
	
	HRESULT hr=::CLSIDFromProgID(L"v81.COMConnector", &clsidV8App);
	hr=::CoCreateInstance(clsidV8App, NULL, CLSCTX_INPROC_SERVER, IID_IDispatch, (void**) &pDispApp); 
	if(FAILED(hr)){
		MessageBox(0,_T(""),_T(""),MB_OK);
		CoUninitialize();
		return _T("-1");
	}
	
	BSTR bstrConnect = L"Connect";
	hr = pDispApp->GetIDsOfNames(IID_NULL, &bstrConnect, 1, LOCALE_SYSTEM_DEFAULT, &dispIDConnect);
	if (FAILED(hr)){
		if (pDispApp)
			pDispApp->Release();
		CoUninitialize();
		return _T("-2");
	}

	VARIANT vRet;
	DISPPARAMS args = {NULL, NULL, 0, 0};
	VARIANT vars[1];  

	args.cArgs = 1;
	args.rgvarg = vars;

	VariantInit(args.rgvarg);
	args.rgvarg[0].vt = VT_BSTR;
	
	WCHAR *t = new WCHAR[basePath.size()];
	CharToWChar(t, basePath.c_str());
	args.rgvarg[0].bstrVal = SysAllocString(t);

	EXCEPINFO pExceptInfo;
	hr = pDispApp->Invoke(dispIDConnect, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &args, &vRet, &pExceptInfo, NULL);
 	if (FAILED(hr)){
		if (pDispApp)
			pDispApp->Release();
		if (hr = DISP_E_EXCEPTION) {
			MessageBoxA(0, basePath.c_str(), "", MB_OK|MB_ICONERROR);
			MessageBox(0, args.rgvarg[0].bstrVal, _T(""), MB_OK|MB_ICONERROR);
			MessageBox(0, pExceptInfo.bstrDescription, _T(""), MB_OK|MB_ICONERROR);
		}
		else {
			char* mess = new char[100];
			sprintf(mess, "%x", hr);
			MessageBoxA(0, mess, "", MB_OK|MB_ICONERROR);
		}
		CoUninitialize();
		return _T("-3");
	}

	pDispApp = vRet.pdispVal;
	
	WCHAR *bstrCommand2 = new WCHAR[FuncName.size()];
	CharToWChar(bstrCommand2, FuncName.c_str());
	hr = pDispApp->GetIDsOfNames(IID_NULL, &bstrCommand2, 1, LOCALE_SYSTEM_DEFAULT, &dispIDCommand);
	
	args.rgdispidNamedArgs = NULL;
	args.rgvarg = NULL;
	args.cArgs = 0;
	args.cNamedArgs = 0;

	pDispApp->AddRef();
	hr = pDispApp->Invoke(dispIDCommand, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &args, &vRet, NULL, NULL);

	int stop = 1;
	while (stop > 0) stop = pDispApp->Release();

	CoUninitialize();
	return vRet.bstrVal;
	}
	catch (...){
		return _T("");
	}

	return _T("");
}

DWORD ServiceExecutionThread1(LPDWORD param)
{
	int secWait = InitCom();
	
	if (secWait <= 0) {
		char* temp = new char[100];
		sprintf(temp, "Can't connect to Base :: error code: %d", secWait);
		WCHAR* t = new WCHAR[100];
		CharToWChar(t, temp);
		MessageBox(NULL, t, _T("Error"), MB_OK | MB_ICONERROR);
		return 0;}

	while(nServiceRunning){		
		try {
			BSTR vret = DoFunction();
			
			ndata.uFlags=NIF_TIP;
			lstrcpyn(ndata.szTip, vret, sizeof(ndata.szTip)/sizeof(TCHAR));
			Shell_NotifyIcon(NIM_MODIFY,&ndata);
			
			if (hwnd != NULL) {
				SetDlgItemText(hwnd, IDC_EDIT, vret);
			}
		}
		catch(...){
			char* mess = new char[100];
			sprintf(mess, "Ошибка выполнения метода севиса");
			MessageBoxA(NULL, mess, "Error", MB_OK | MB_ICONERROR);
		}
		
		Sleep(secWait * 1000);
	}

	return 0;
}

DWORD ServiceExecutionThread2(LPDWORD param)
{
	//printf("thread2 start...");

	hwnd=CreateDialog(NULL,MAKEINTRESOURCE(IDD_DIALOG1), NULL, DialogProc);
	ShowWindow(hwnd, SW_HIDE);
	//DialogBox(NULL,MAKEINTRESOURCE(IDD_DIALOG1),hwnd, DialogProc);

	MSG msg;
	BOOL bRet;

	while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0) 
	{ 
		if (bRet == -1)
		{
			// Handle the error and possibly exit
		}
		else if (!IsWindow(hwnd) || !IsDialogMessage(hwnd, &msg)) 
		{ 
			TranslateMessage(&msg); 
			DispatchMessage(&msg); 
		} 
	} 
	return 0;
}

void KillService()
{
	ndata.uFlags = 0;
	Shell_NotifyIcon(NIM_DELETE,&ndata);

	nServiceRunning=false;
	SetEvent(killServiceEvent);
	UpdateServiceStatus(SERVICE_STOPPED,NO_ERROR,0,0,0);
}

void ServiceCtrlHandler(DWORD nControlCode)
{
	BOOL success;
	switch(nControlCode)
	{	
	case SERVICE_CONTROL_SHUTDOWN:
	case SERVICE_CONTROL_STOP:
		nServiceCurrentStatus=SERVICE_STOP_PENDING;
		success=UpdateServiceStatus(SERVICE_STOP_PENDING,NO_ERROR,0,1,3000);
		KillService();		
		return;
	default:
		break;
	}
	UpdateServiceStatus(nServiceCurrentStatus,NO_ERROR,0,0,0);
}


BOOL CALLBACK DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	
	switch (message)
	{
	case WM_INITDIALOG:
		
		ndata.cbSize=sizeof(NOTIFYICONDATA);
		ndata.hWnd=hWnd;
		ndata.uID=2000;
		ndata.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP;
		ndata.uCallbackMessage=SWM_TRAYMSG;
		ndata.hIcon=::LoadIcon(GetModuleHandle(_T("Win1cService.exe")),MAKEINTRESOURCE(IDI_Win1c));
		lstrcpyn(ndata.szTip, _T("Win1c service::"), sizeof(ndata.szTip)/sizeof(TCHAR));
		Shell_NotifyIcon(NIM_ADD,&ndata);
		
		//SetWindowPos(hWnd,NULL,-10,-10,0,0,SWP_NOZORDER|SWP_NOMOVE);
		//hwnd=hwndDlg;
		break;
	
	case SWM_TRAYMSG:
		switch(lParam)
		{
		case WM_LBUTTONDBLCLK:
			ShowWindow(hWnd, SW_RESTORE);
			break;
		case WM_RBUTTONDOWN:
		case WM_CONTEXTMENU:
			ShowContextMenu(hWnd);
		}
		break;
	case WM_SYSCOMMAND:
		if((wParam & 0xFFF0) == SC_MINIMIZE)
		{
			ShowWindow(hWnd, SW_HIDE);
			return 1;
		}
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case SWM_SHOW:
			ShowWindow(hWnd, SW_RESTORE);
			break;
		case SWM_HIDE:
		case IDOK:
			ShowWindow(hWnd, SW_HIDE);
			break;
		case SWM_EXIT:
			DestroyWindow(hWnd);
			break;
		}
		break;
	case WM_CLOSE:
		ndata.uFlags = 0;
		Shell_NotifyIcon(NIM_DELETE,&ndata);
		PostQuitMessage(0);
		break;
	}
	return 0;
}

// Name says it all
void ShowContextMenu(HWND hWnd)
{
	POINT pt;
	GetCursorPos(&pt);
	HMENU hMenu = CreatePopupMenu();
	if(hMenu)
	{
		if( IsWindowVisible(hWnd) )
			InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_HIDE, _T("Hide"));
		else
			InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_SHOW, _T("Show"));
		InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_EXIT, _T("Exit"));

		// note:	must set window to the foreground or the
		//			menu won't disappear when it should
		SetForegroundWindow(hWnd);

		TrackPopupMenu(hMenu, TPM_BOTTOMALIGN,
			pt.x, pt.y, 0, hWnd, NULL );
		DestroyMenu(hMenu);
	}
}