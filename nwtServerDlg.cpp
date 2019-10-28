
// nwtServerDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "nwtServer.h"
#include "nwtServerDlg.h"
#include "afxdialogex.h"

#include "NwtHeader.h"
#include "Commands.h"
#include "NwtBase.h"

#include "User.h"

#include <fstream>
#include <string>
#include <vector>

using std::ifstream;
using std::string;
using std::vector;

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

    int retCode = ServerStartup();
    if (0 > retCode) {
        CString strText = "[ERROR] ServerStartup调用失败！";
        MessageBox(strText, "错误提示");
        return FALSE;
    }

    retCode = LoadUsers("Users.txt");
    if (0 > retCode) {
        CString strText = "[ERROR] 加载用户文件调用失败！";
        MessageBox(strText, "错误提示");
        return FALSE;
    }

    return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

int CnwtServerDlg::LoadUsers(const char* filename) {
    ifstream users(filename);
    if (!users) {
        CString strText = "";
        strText.Format("[ERROR] 打开用户文件失败： filename = %s", filename);
        MessageBox(strText, "错误提示");
        return -1;
    }

    string line = "";
    vector<string> vec;
    string::size_type begin = 0, end = 0;
    while (getline(users, line)) {
        for (auto iter = line.begin(); iter != line.end();) { //remove spaces
            if (' ' == *iter) {
                iter = line.erase(iter);
                continue;
            }
            iter++;
        }
        begin = 0;
        while (line.size() != begin) {
            end = line.find(',', begin);
            vec.push_back(line.substr(begin, end - begin));
            if (string::npos != end) {
                begin = end + 1;
            }
            else {
                break;
            }
        }

        User user; //TODO: refactor !!!
        user.m_account = stoi(vec[0]);
        user.m_nickname = vec[1];
        user.m_password = vec[2];
        user.m_email = vec[3];
        user.m_mobile = vec[4];
        m_Users[user.m_account] = user;

        vec.clear();
    }

    return 0;
}

void CnwtServerDlg::HandleLoginReq(const LoginReq* loginReq, const unsigned int sock) {
    unsigned int rspCode = LOGIN_SUCCESS;
    string rspMsg("登录成功");
    string nickname("");
    CString strInfo = "";
    auto iterUser = m_Users.find(loginReq->m_account);
    if (iterUser != m_Users.end()) {
        nickname = iterUser->second.m_nickname;
        auto iterContact = m_Contacts.find(loginReq->m_account);
        if (iterContact != m_Contacts.end()) {
            strInfo.Format("[WARNING] 客户已登录： account = %d, clientSock = %d", loginReq->m_account, iterContact->second);
            if (sock != iterContact->second) {
                m_Contacts[loginReq->m_account] = sock; //update client sock 
            }
        }
        else {
            string password = loginReq->m_password;
            if (password != iterUser->second.m_password) {
                rspCode = LOGIN_FAIL;
                rspMsg = "密码错误！";
                strInfo.Format("[WARNING] 用户密码错误： account = %d, clientSock = %d", loginReq->m_account, sock);
            }
            else {
                rspCode = LOGIN_SUCCESS;
                m_Contacts[loginReq->m_account] = sock;
                strInfo.Format("[DEBUG] 客户登录成功： account = %d, clientSock = %d", loginReq->m_account, sock);
            }
        }
    }
    else { //用户未注册
        rspCode = LOGIN_FAIL;
        rspMsg = "用户没有注册";
        strInfo.Format("[WARNING] 用户没有注册： account = %d, clientSock = %d", loginReq->m_account, sock);
    }
    AppendString(strInfo);

    //send LoginRsp
    LoginRsp loginRsp;
    loginRsp.m_head = NwtHeader(CMD_LOGIN_RSP, 0, loginReq->m_account, sizeof(LoginRsp) - sizeof(NwtHeader));
    loginRsp.m_account = loginReq->m_account;
    loginRsp.m_rspCode = rspCode;
    memcpy(&(loginRsp.m_rspMsg), rspMsg.c_str(), sizeof(loginRsp.m_rspMsg));
    memcpy(&(loginRsp.m_nickname), nickname.c_str(), sizeof(loginRsp.m_nickname));
    int retCode = nwtSend(sock, &loginRsp, sizeof(LoginRsp));
    if (0 > retCode) {
        int errNo = WSAGetLastError();
        strInfo.Format("[ERROR] 消息发送失败：targetSock = %d", sock);
        AppendString(strInfo);
    }
    else {
        strInfo.Format("[DEBUG] 消息发送成功：targetSock = %d", sock);
        AppendString(strInfo);
    }
}

void CnwtServerDlg::HandleInstantMsg(const NwtHeader* nwtHead, const unsigned int sock) {
    char* content = new char[nwtHead->m_contentLength + 1];
    memset(content, 0, nwtHead->m_contentLength + 1);
    memcpy(content, (char*)nwtHead + sizeof(NwtHeader), nwtHead->m_contentLength);
    CString strInfo = "";
    strInfo.Format("[RECV-%d] srcAccount=%d, targetAccount=%d, contentLength=%d, content=%s",
        sock, nwtHead->m_srcAccount, nwtHead->m_tarAccount, nwtHead->m_contentLength, content);
    AppendString(strInfo);

    auto iter = m_Contacts.find(nwtHead->m_tarAccount);
    if (iter == m_Contacts.end()) {
        strInfo.Format("[ERROR] targetContact没有登录： targetAccount = %d", nwtHead->m_tarAccount);
        AppendString(strInfo);
    }
    else {
        unsigned int targetSock = iter->second;
        int retCode = nwtSend(targetSock, (void*)nwtHead, sizeof(NwtHeader) + nwtHead->m_contentLength);
        if (0 > retCode) {
            int errNo = WSAGetLastError();
            strInfo.Format("[ERROR] 消息发送失败：targetAccount = %d, errNo = %d, targetSock = %d", nwtHead->m_tarAccount, errNo, targetSock);
            AppendString(strInfo);
        }
        else {
            strInfo.Format("[DEBUG] 消息发送成功：targetAccount = %d", nwtHead->m_tarAccount);
            AppendString(strInfo);
        }
    }

    delete[] content;
}

int CnwtServerDlg::ServerStartup() {
    CString strText = "", strCaptain = "提示信息";
    WSAData wsaData;
    int retCode = WSAStartup(MAKEWORD(1, 1), &wsaData);
    if (NO_ERROR != retCode) {
        strText.Format("WSAStartup调用失败！[retCode = %d]", retCode);
        MessageBox(strText, strCaptain);
        return -1;
    }

    m_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (INVALID_SOCKET == m_sock) {
        strText.Format("socket初始化失败[sock = %d]", m_sock);
        MessageBox(strText, strCaptain);
        return -1;
    }

    int on = 1;
    retCode = setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)& on, sizeof(on));
    if (0 > retCode) {
        strText.Format("setsockopt调用失败！[retCode = %d]", retCode);
        MessageBox(strText, strCaptain);
        return -1;
    }

    SOCKADDR_IN serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(m_port);

    retCode = bind(m_sock, (SOCKADDR*)& serverAddr, sizeof(serverAddr));
    if (0 > retCode) {
        strText.Format("bind调用失败！[retCode = %d]", retCode);
        MessageBox(strText, strCaptain);
        return -1;
    }

    retCode = listen(m_sock, 5);
    if (0 > retCode) {
        strText.Format("listen调用失败！[retCode = %d]", retCode);
        MessageBox(strText, strCaptain);
        return -1;
    }

    m_running = TRUE;
    m_serverThread = AfxBeginThread(ServerProcess, this);

    return 0;
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
    do {
        clientSock = accept(pServerDlg->m_sock, (SOCKADDR*)& clientAddr, &len);
        if (INVALID_SOCKET == clientSock) {
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
    if (INVALID_SOCKET == clientSock || nullptr == pServerDlg) {
        AfxMessageBox("invalid client socket or null dlg pointer!");
        return -1;
    }

    CString strInfo = "";
    //int retCode = 0;
    NwtHeader* nwtHead = NULL;
    void* recv = NULL;
    do {
        recv = nwtRecv(clientSock);
        nwtHead = (NwtHeader*)recv;
        if (NULL == nwtHead) {
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
            strInfo.Format("[ERROR] recv()失败： clientSock = %d, errNo = %d", clientSock, errNo);
            pServerDlg->AppendString(strInfo);
            break;
        }

        if (CMD_LOGIN_REQ == nwtHead->m_cmd) {
            pServerDlg->HandleLoginReq((LoginReq*)nwtHead, clientSock);
        }

        if (CMD_INSTANT_MSG == nwtHead->m_cmd) {
            pServerDlg->HandleInstantMsg(nwtHead, clientSock);
        }

        delete[] (char*)recv;
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
