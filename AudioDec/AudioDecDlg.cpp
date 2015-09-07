
// AudioDecDlg.cpp : implementation file
//

#include "stdafx.h"
#include "AudioDec.h"
#include "AudioDecDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CAudioDecDlg dialog




CAudioDecDlg::CAudioDecDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CAudioDecDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CAudioDecDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS1, AudioConvertProgress);
}

BEGIN_MESSAGE_MAP(CAudioDecDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(FileChoose_BUTTON, &CAudioDecDlg::OnBnClickedFileChooseButton)
	ON_BN_CLICKED(DecStart_BUTTON, &CAudioDecDlg::OnBnClickedButton)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_PROGRESS1, &CAudioDecDlg::OnNMCustomdrawProgress1)
END_MESSAGE_MAP()


// CAudioDecDlg message handlers

BOOL CAudioDecDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	AudioConvertProgress.SetRange(0,100);
	AudioConvertProgress.SetPos(0); 
	hwnd = GetSafeHwnd();
	msgID = 0;
	return TRUE;  // return TRUE  unless you set the focus to a control
}


void CAudioDecDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CAudioDecDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CAudioDecDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CAudioDecDlg::OnBnClickedFileChooseButton()
{
	// TODO: 在此添加控件通知处理程序代码
	statusText = "程序未启动";
	GetDlgItem(ProgramState)->SetWindowTextW(statusText);
	AudioConvertProgress.SetPos(0);
	CFileDialog dlg(TRUE,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,NULL,NULL);
	if (dlg.DoModal() == IDOK)
	{
		filePath = dlg.GetPathName();
		CString filename = filePath.Mid(filePath.ReverseFind('\\')+1);
		GetDlgItem(inputfile_EDIT)->SetWindowTextW(filename);
	}

	else
	{
		return;
	}
}


void CAudioDecDlg::OnBnClickedButton()
{
	// TODO: 在此添加控件通知处理程序代码
	
	statusText = "程序已启动";	
	GetDlgItem(ProgramState)->SetWindowTextW(statusText);
	GetDlgItem(DecStart_BUTTON)->EnableWindow(FALSE);
	AfxBeginThread(MyThreadFunction,this);
	int pos = 0;
	
	while(1)
	{
		PeekMessage(&msg,hwnd,0,1,PM_REMOVE);
		//GetMessage(&msg,hwnd,0,0);
		if(msg.wParam == DECING_TAG)
		{
			pos = msg.lParam;
			AudioConvertProgress.SetPos(pos);
			//Sleep(10);
		}
		else if(msg.wParam == FINISH_TAG)
		{
			AudioConvertProgress.SetPos(100);
			statusText = "解码已完成";	
			GetDlgItem(ProgramState)->SetWindowTextW(statusText);
			GetDlgItem(DecStart_BUTTON)->EnableWindow(TRUE);
			break;
		}
		else if(msg.wParam == FAILED_TAG)
		{
			statusText = "解码失败";	
			GetDlgItem(ProgramState)->SetWindowTextW(statusText);
			break;
		}
		else
		{
			//Sleep(500);
			continue;
		}
		//Sleep(100);
	}
}

UINT CAudioDecDlg::MyThreadFunction(LPVOID pParam)
{
	CAudioDecDlg *p = NULL;
	p = (CAudioDecDlg *)pParam;
	HWND hwnd = p->hwnd;
	UINT msgID = p->msgID;
	char *inputfile = NULL;
	USES_CONVERSION;
	inputfile = T2A(p->filePath);
	char *outputfile = "temp.mp3";
	int ret = AudioConvert(inputfile,outputfile,0,0,0,hwnd,msgID);
	return 0;
}


void CAudioDecDlg::OnNMCustomdrawProgress1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
}
