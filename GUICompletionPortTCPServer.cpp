#define _CRT_SECURE_NO_WARNINGS         // �ֽ� VC++ ������ �� ��� ����
#define _WINSOCK_DEPRECATED_NO_WARNINGS // �ֽ� VC++ ������ �� ��� ����
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

#define SERVERPORT 9000
#define BUFSIZE    512

// ���� ���� ������ ���� ����ü
struct SOCKETINFO
{
    OVERLAPPED overlapped;
    SOCKET sock;
    char buf[BUFSIZE + 1];
    int recvbytes;
    int sendbytes;
    WSABUF wsabuf;
};

// ������ ���ν���
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
// ���� ��Ʈ�� ��� �Լ�
void DisplayText(char *fmt, ...);
// ���� ��� �Լ�
void err_quit(char *msg);
void err_display(char *msg);
// ���� ��� ������ �Լ�
DWORD WINAPI ServerMain(LPVOID arg);
DWORD WINAPI ProcessClient(LPVOID arg);
// �۾��� ������ �Լ�
DWORD WINAPI WorkerThread(LPVOID arg);

HINSTANCE hInst; // �ν��Ͻ� �ڵ�
HWND hEdit; // ���� ��Ʈ��
CRITICAL_SECTION cs; // �Ӱ� ����

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    hInst = hInstance;
    InitializeCriticalSection(&cs);

    // ������ Ŭ���� ���
    WNDCLASS wndclass;
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = "MyWndClass";
    if (!RegisterClass(&wndclass)) return 1;

    // ������ ����
    HWND hWnd = CreateWindow("MyWndClass", "TCP ����", WS_OVERLAPPEDWINDOW,
        0, 0, 600, 200, NULL, NULL, hInstance, NULL);
    if (hWnd == NULL) return 1;
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // ���� ��� ������ ����
    CreateThread(NULL, 0, ServerMain, NULL, 0, NULL);

    // �޽��� ����
    MSG msg;
    while (GetMessage(&msg, 0, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    DeleteCriticalSection(&cs);
    return msg.wParam;
}

// ������ ���ν���
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_CREATE:
        hEdit = CreateWindow("edit", NULL,
            WS_CHILD | WS_VISIBLE | WS_HSCROLL |
            WS_VSCROLL | ES_AUTOHSCROLL |
            ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY,
            0, 0, 0, 0, hWnd, (HMENU)100, hInst, NULL);
        return 0;
    case WM_SIZE:
        MoveWindow(hEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        return 0;
    case WM_SETFOCUS:
        SetFocus(hEdit);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// ���� ��Ʈ�� ��� �Լ�
void DisplayText(char *fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);

    char cbuf[BUFSIZE + 256];
    vsprintf(cbuf, fmt, arg);

    EnterCriticalSection(&cs);
    int nLength = GetWindowTextLength(hEdit);
    SendMessage(hEdit, EM_SETSEL, nLength, nLength);
    SendMessage(hEdit, EM_REPLACESEL, FALSE, (LPARAM)cbuf);
    LeaveCriticalSection(&cs);

    va_end(arg);
}

// ���� �Լ� ���� ��� �� ����
void err_quit(char *msg)
{
    LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
    MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
    LocalFree(lpMsgBuf);
    exit(1);
}

// ���� �Լ� ���� ���
void err_display(char *msg)
{
    LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
    DisplayText("[%s] %s", msg, (char *)lpMsgBuf);
    LocalFree(lpMsgBuf);
}

// TCP ���� ���� �κ�
DWORD WINAPI ServerMain(LPVOID arg)
{
    int retval;

    // ���� �ʱ�ȭ
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    // ����� �Ϸ� ��Ʈ ����
    HANDLE hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (hcp == NULL) return 1;

    // CPU ���� Ȯ��
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    // (CPU ���� * 2)���� �۾��� ������ ����
    HANDLE hThread;
    for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++) {
        hThread = CreateThread(NULL, 0, WorkerThread, hcp, 0, NULL);
        if (hThread == NULL) return 1;
        CloseHandle(hThread);
    }

    // socket()
    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET) err_quit("socket()");

    // bind()
    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERVERPORT);
    retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("bind()");

    // listen()
    retval = listen(listen_sock, SOMAXCONN);
    if (retval == SOCKET_ERROR) err_quit("listen()");

    // ������ ��ſ� ����� ����
    SOCKET client_sock;
    SOCKADDR_IN clientaddr;
    int addrlen;
    DWORD recvbytes, flags;

    while (1) {
        // accept()
        addrlen = sizeof(clientaddr);
        client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
        if (client_sock == INVALID_SOCKET) {
            err_display("accept()");
            break;
        }
        // ������ Ŭ���̾�Ʈ ���� ���
        DisplayText("\r\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\r\n",
            inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

        // ���ϰ� ����� �Ϸ� ��Ʈ ����
        CreateIoCompletionPort((HANDLE)client_sock, hcp, client_sock, 0);

        // ���� ���� ����ü �Ҵ�
        SOCKETINFO* ptr = new SOCKETINFO;
        if (ptr == NULL) break;
        ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
        ptr->sock = client_sock;
        ptr->recvbytes = ptr->sendbytes = 0;
        ptr->wsabuf.buf = ptr->buf;
        ptr->wsabuf.len = BUFSIZE;

        // �񵿱� ����� ����
        flags = 0;
        retval = WSARecv(client_sock, &ptr->wsabuf, 1, &recvbytes,
            &flags, &ptr->overlapped, NULL);
        if (retval == SOCKET_ERROR) {
            if (WSAGetLastError() != ERROR_IO_PENDING) {
                err_display("WSARecv()");
            }
            continue;
        }
    }

    // ���� ����
    WSACleanup();
    return 0;
}

// Ŭ���̾�Ʈ�� ������ ���
DWORD WINAPI ProcessClient(LPVOID arg)
{
    SOCKET client_sock = (SOCKET)arg;
    int retval;
    SOCKADDR_IN clientaddr;
    int addrlen;
    char buf[BUFSIZE + 1];

    // Ŭ���̾�Ʈ ���� ���
    addrlen = sizeof(clientaddr);
    getpeername(client_sock, (SOCKADDR *)&clientaddr, &addrlen);

    while (1) {
        // ������ �ޱ�
        retval = recv(client_sock, buf, BUFSIZE, 0);
        if (retval == SOCKET_ERROR) {
            err_display("recv()");
            break;
        }
        else if (retval == 0)
            break;

        // ���� ������ ���
        buf[retval] = '\0';
        DisplayText("[TCP/%s:%d] %s\r\n", inet_ntoa(clientaddr.sin_addr),
            ntohs(clientaddr.sin_port), buf);

        // ������ ������
        retval = send(client_sock, buf, retval, 0);
        if (retval == SOCKET_ERROR) {
            err_display("send()");
            break;
        }
    }

    // closesocket()
    closesocket(client_sock);
    DisplayText("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\r\n",
        inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

    return 0;
}


// �۾��� ������ �Լ�
DWORD WINAPI WorkerThread(LPVOID arg)
{
    int retval;
    HANDLE hcp = (HANDLE)arg;

    while (1) {
        // �񵿱� ����� �Ϸ� ��ٸ���
        DWORD cbTransferred;
        SOCKET client_sock;
        SOCKETINFO* ptr;
        retval = GetQueuedCompletionStatus(hcp, &cbTransferred,
            (LPDWORD)&client_sock, (LPOVERLAPPED*)&ptr, INFINITE);

        // Ŭ���̾�Ʈ ���� ���
        SOCKADDR_IN clientaddr;
        int addrlen = sizeof(clientaddr);
        getpeername(ptr->sock, (SOCKADDR*)&clientaddr, &addrlen);

        // �񵿱� ����� ��� Ȯ��
        if (retval == 0 || cbTransferred == 0) {
            if (retval == 0) {
                DWORD temp1, temp2;
                WSAGetOverlappedResult(ptr->sock, &ptr->overlapped,
                    &temp1, FALSE, &temp2);
                err_display("WSAGetOverlappedResult()");
            }
            closesocket(ptr->sock);
            printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
                inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
            delete ptr;
            continue;
        }

        // ������ ���۷� ����
        if (ptr->recvbytes == 0) {
            ptr->recvbytes = cbTransferred;
            ptr->sendbytes = 0;
            // ���� ������ ���
            ptr->buf[ptr->recvbytes] = '\0';
            DisplayText("[TCP/%s:%d] %s\r\n", inet_ntoa(clientaddr.sin_addr),
                ntohs(clientaddr.sin_port), ptr->buf);
        }
        else {
            ptr->sendbytes += cbTransferred;
        }

        if (ptr->recvbytes > ptr->sendbytes) {
            // ������ ������
            ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
            ptr->wsabuf.buf = ptr->buf + ptr->sendbytes;
            ptr->wsabuf.len = ptr->recvbytes - ptr->sendbytes;

            DWORD sendbytes;
            retval = WSASend(ptr->sock, &ptr->wsabuf, 1,
                &sendbytes, 0, &ptr->overlapped, NULL);
            if (retval == SOCKET_ERROR) {
                if (WSAGetLastError() != WSA_IO_PENDING) {
                    err_display("WSASend()");
                }
                continue;
            }
        }
        else {
            ptr->recvbytes = 0;

            // ������ �ޱ�
            ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
            ptr->wsabuf.buf = ptr->buf;
            ptr->wsabuf.len = BUFSIZE;

            DWORD recvbytes;
            DWORD flags = 0;
            retval = WSARecv(ptr->sock, &ptr->wsabuf, 1,
                &recvbytes, &flags, &ptr->overlapped, NULL);
            if (retval == SOCKET_ERROR) {
                if (WSAGetLastError() != WSA_IO_PENDING) {
                    err_display("WSARecv()");
                }
                continue;
            }
        }
    }

    return 0;
}