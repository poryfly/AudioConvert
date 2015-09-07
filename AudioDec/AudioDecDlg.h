
// AudioDecDlg.h : header file
//

#pragma once
#include "AudioConvert.h"
#include "afxcmn.h"

// CAudioDecDlg dialog
class CAudioDecDlg : public CDialogEx
{
// Construction
public:
	CAudioDecDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_AUDIODEC_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CString statusText;
	CString filePath;
	UINT msgID;
	HWND hwnd;
	MSG msg;
	afx_msg void OnBnClickedFileChooseButton();
	afx_msg void OnBnClickedButton();
	CProgressCtrl AudioConvertProgress;
	afx_msg void OnNMCustomdrawProgress1(NMHDR *pNMHDR, LRESULT *pResult);
	static UINT MyThreadFunction(LPVOID pParam);
};
