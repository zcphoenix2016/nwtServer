
// nwtServerDlg.h: 头文件
//

#pragma once

#include <map>

// CnwtServerDlg 对话框
class CnwtServerDlg : public CDialogEx
{
// 构造
public:
    CnwtServerDlg(CWnd* pParent = nullptr);    // 标准构造函数
    ~CnwtServerDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_NWTSERVER_DIALOG };
#endif

    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持


// 实现
protected:
    HICON m_hIcon;

    // 生成的消息映射函数
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    DECLARE_MESSAGE_MAP()

private:
    int ServerStartup();
    static UINT ServerProcess(LPVOID lParam);
    static UINT RecvProcess(LPVOID lParam);
    void AppendString(CString text);

private:
    UINT m_port = 8888;
    SOCKET m_sock = INVALID_SOCKET;
    CWinThread* m_serverThread = NULL;
    BOOL m_running = FALSE;

public:
    CEdit m_editMsgs;
    std::map<unsigned int, unsigned int> m_Contacts;
};
