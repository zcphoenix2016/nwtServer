﻿
// nwtServerDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "nwtServer.h"
#include "nwtServerDlg.h"
#include "afxdialogex.h"

#include "NwtHeader.h"
#include "Commands.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
    CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ABOUTBOX };
#endif

    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CnwtServerDlg 对话框



CnwtServerDlg::CnwtServerDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_NWTSERVER_DIALOG, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CnwtServerDlg::~CnwtServerDlg()
{
    m_running = FALSE;
    shutdown(m_sock, SD_BOTH);
    closesocket(m_sock);
}

void CnwtServerDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_MSGS, m_editMsgs);
}

BEGIN_MESSAGE_MAP(CnwtServerDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()


// CnwtServerDlg 消息处理程序

BOOL CnwtServerDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 将“关于...”菜单项添加到系统菜单中。

    // IDM_ABOUTBOX 必须在系统命令范围内。
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != nullptr)
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

    // 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
    //  执行此操作
    SetIcon(m_hIcon, TRUE);            // 设置大图标
    SetIcon(m_hIcon, FALSE);        // 设置小图标

    // TODO: 在此添加额外的初始化代码
    //[ZCP] begin
    CString strText = "", strCaptain = "提示信息";
    WSAData wsaData;
    int retCode = WSAStartup(MAKEWORD(1, 1), &wsaData);
    if (NO_ERROR != retCode)
    {
        strText.Format("WSAStartup调用失败！[retCode = %d]", retCode);
        MessageBox(strText, strCaptain);
        return FALSE;
    }

    m_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (INVALID_SOCKET == m_sock)
    {
        strText.Format("socket初始化失败[sock = %d]", m_sock);
        MessageBox(strText, strCaptain);
        return FALSE;
    }
    
    int on = 1;
    retCode = setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
    if (0 > retCode)
    {
        strText.Format("setsockopt调用失败！[retCode = %d]", retCode);
        MessageBox(strText, strCaptain);
        return FALSE;
    }

    SOCKADDR_IN serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(m_port);
    
    retCode = bind(m_sock, (SOCKADDR*)& serverAddr, sizeof(serverAddr));
    if (0 > retCode)
    {
        strText.Format("bind调用失败！[retCode = %d]", retCode);
        MessageBox(strText, strCaptain);
        return FALSE;
    }
    
    retCode = listen(m_sock, 5);
    if (0 > retCode)
    {
        strText.Format("listen调用失败！[retCode = %d]", retCode);
        MessageBox(strText, strCaptain);
        return FALSE;
    }

    m_running = TRUE;
    m_serverThread = AfxBeginThread(ServerProcess, this);

    //[ZCP] end

    return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

class RecvProcessParam
{
public:
    RecvProcessParam(SOCKET clientSock, CnwtServerDlg *svrDlg) 
        : m_clientSock(clientSock), m_svrDlg(svrDlg)
    {
    }

    SOCKET m_clientSock = INVALID_SOCKET;
    CnwtServerDlg* m_svrDlg = nullptr;
};

UINT CnwtServerDlg::ServerProcess(LPVOID lParam)
{
    CnwtServerDlg* pServerDlg = (CnwtServerDlg*)lParam;
    SOCKADDR_IN clientAddr;
    int len = sizeof(SOCKADDR);
    SOCKET clientSock = INVALID_SOCKET;
    CString strNew = "", strOld = "", strText = "";
    do
    {
        clientSock = accept(pServerDlg->m_sock, (SOCKADDR*)& clientAddr, &len);
        if (INVALID_SOCKET == clientSock)
        {
            strText.Format("[ERROR] accept调用失败：clientSock = %d", clientSock);
            pServerDlg->AppendString(strText);
            continue;
        }

        strText.Format("[DEBUG] 客户端链接成功：clientSock = %d", clientSock);
        pServerDlg->AppendString(strText);

        RecvProcessParam* rpp = new RecvProcessParam(clientSock, pServerDlg); //will delete in RecvProcess()
        AfxBeginThread(RecvProcess, rpp);

    } while (pServerDlg->m_running);

    return 0;
}

UINT CnwtServerDlg::RecvProcess(LPVOID lParam)
{
    RecvProcessParam* rpp = (RecvProcessParam*)lParam;
    SOCKET clientSock = rpp->m_clientSock;
    CnwtServerDlg* pServerDlg = rpp->m_svrDlg;
    if (INVALID_SOCKET == clientSock || nullptr == pServerDlg)
    {
        AfxMessageBox("invalid client socket or null dlg pointer!");
        return -1;
    }

    CString strNew = "", strOld = "", strRecv = "";
    char buf[1024] = { 0 };
    int rval = 0;
    do
    {
        memset(buf, 0, sizeof(buf));
        rval = recv(clientSock, buf, 1024, 0);//TODO: refactor to single function for loop-recv
        if (0 >= rval) {
            for (auto iter = pServerDlg->m_Contacts.begin(); iter != pServerDlg->m_Contacts.end();) {
                if (clientSock == iter->second) {
                    shutdown(iter->second, SD_BOTH);
                    closesocket(iter->second);
                    iter = pServerDlg->m_Contacts.erase(iter);
                } 
                else {
                    ++iter;
                }
            }
            int errNo = WSAGetLastError();
            if (0 > rval) {
                strRecv.Format("[ERROR] recv()失败： clientSock = %d, errNo = %d", clientSock, errNo);
            } 
            else {
                strRecv.Format("[DEBUG] 客户端关闭链接： clientSock = %d, errNo = %d", clientSock, errNo);
            }
            pServerDlg->AppendString(strRecv);
            break;
        }
        NwtHeader* nwtHead = (NwtHeader*)buf;
        if (CMD_LOGIN == nwtHead->m_cmd)
        {
            SOCKET contactSock = INVALID_SOCKET;
            auto iter = pServerDlg->m_Contacts.find(nwtHead->m_srcAccount);
            if (iter != pServerDlg->m_Contacts.end()) {
                strRecv.Format("[WARNING] 客户已登录： contactNo = %d, clientSock = %d", nwtHead->m_srcAccount, contactSock);
            } else {
                //pServerDlg->m_Contacts.insert({ nwtHead->m_srcNo, clientSock });//insert new contact
                strRecv.Format("[DEBUG] 客户登录成功： contactNo = %d, clientSock = %d", nwtHead->m_srcAccount, clientSock);
            }
            pServerDlg->m_Contacts[nwtHead->m_srcAccount] = clientSock; //update client sock or insert new contact
            pServerDlg->AppendString(strRecv);
            continue;
        }
        if (CMD_INSTANT_MSG == nwtHead->m_cmd)
        {
            char* content = new char[nwtHead->m_contentLength + 1];
            memset(content, 0, nwtHead->m_contentLength + 1);
            memcpy(content, buf + sizeof(NwtHeader), nwtHead->m_contentLength);
            strRecv.Format("[RECV-%d] rval=%d, srcNo=%d, targetNo=%d, contentLength=%d, content=%s",
                clientSock, rval, nwtHead->m_srcAccount, nwtHead->m_tarAccount, nwtHead->m_contentLength, content);
            pServerDlg->AppendString(strRecv);

            auto iter = pServerDlg->m_Contacts.find(nwtHead->m_tarAccount);
            if (iter == pServerDlg->m_Contacts.end()) {
                strRecv.Format("[ERROR] targetContact没有登录： targetNo = %d", nwtHead->m_tarAccount);
                pServerDlg->AppendString(strRecv);
            } else {
                int retCode = 0;
                unsigned int targetSock = iter->second;
                retCode = send(targetSock, buf, sizeof(NwtHeader) + nwtHead->m_contentLength, 0);
                if (0 > retCode)
                {
                    int errNo = WSAGetLastError();
                    strRecv.Format("[ERROR] 消息发送失败：targetNo = %d, errNo = %d, targetSock = %d", nwtHead->m_tarAccount, errNo, targetSock);
                    pServerDlg->AppendString(strRecv);
                }
                else
                {
                    strRecv.Format("[DEBUG] 消息发送成功：targetNo = %d", nwtHead->m_tarAccount);
                    pServerDlg->AppendString(strRecv);
                }
            }

            delete[] content;
        }

    } while (1);

    delete rpp; //allocate by ServerProcess()

    return 0;
}

void CnwtServerDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CnwtServerDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // 用于绘制的设备上下文

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // 使图标在工作区矩形中居中
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // 绘制图标
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialogEx::OnPaint();
    }
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CnwtServerDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

void CnwtServerDlg::AppendString(CString text)
{
    CString strOld = "", strNew = "";
    m_editMsgs.GetWindowText(strOld);
    strNew = strOld + (strOld.IsEmpty() ? "" : "\r\n") + text;
    m_editMsgs.SetWindowText(strNew);
}
