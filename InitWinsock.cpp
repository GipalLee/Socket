#pragma comment(lib, "ws2_32")
#include <winsock2.h>

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

int main(int argc, char *argv[])
{
    // ���� �ʱ�ȭ
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;
    MessageBox(NULL, "���� �ʱ�ȭ ����", "�˸�", MB_OK);

    // socket()
    SOCKET udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock == INVALID_SOCKET) err_quit("socket()");
    MessageBox(NULL, "TCP ���� ���� ����", "�˸�", MB_OK);

    // closesocket()
    closesocket(udp_sock);

    // ���� ����
    WSACleanup();
    return 0;
}

SOCKET WASSocket (int af,int type, int protocol,
LPWSAPROTOCOL_INFO IpProtocolInfo,
GROUP g,
DWORD dwFlags
);

SOCKET tcp_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WAS_FLAG_OVERLAPPED);
SOCKET udp_socket = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0, WAS_FLAG_OVERLAPPED);
