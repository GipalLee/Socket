#define _WINSOCK_DEPRECATED_NO_WARNINGS // 최신 VC++ 컴파일 시 경고 방지
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#define MULTICASTIP "235.7.8.9"
#define LOCALPORT   9000
#define BUFSIZE     512

// 소켓 함수 오류 출력 후 종료
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

// 소켓 함수 오류 출력
void err_display(char *msg)
{
    LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
    printf("[%s] %s", msg, (char *)lpMsgBuf);
    LocalFree(lpMsgBuf);
}

DWORD WINAPI ProcessClient(LPVOID arg)
{
    SOCKET client_sock = (SOCKET)arg;
    SOCKADDR_IN clientaddr;
    char buf[BUFSIZE + 1];
    int addrlen;
    int retval;

    //    클라이언트 정보 얻기
    addrlen = sizeof(clientaddr);
    getpeername(client_sock, (SOCKADDR*)&clientaddr, &addrlen);

    while (1) {

        //retval = recvn(client_sock, buf, BUFSIZE, 0);
        addrlen = sizeof(clientaddr);
        retval = recvfrom(client_sock, buf, BUFSIZE, 0, (SOCKADDR*)&clientaddr, &addrlen);
        if (retval == SOCKET_ERROR) {
            err_display("recvn(zzzzzzzzzzz)");
            continue;
        }

        //    받은 데이터 출력
        buf[retval] = '\0';
        printf("[TCP/%s:%d] %s\n",
            inet_ntoa(clientaddr.sin_addr),
            ntohs(clientaddr.sin_port), buf);


        /*
        //    데이터 보내기
        retval = send(client_sock, buf, strlen(buf), 0);
        if(retval == SOCKET_ERROR) {
            err_display("sendto()");
            break;
        }
        */
    }

    //    closesocket()
    closesocket(client_sock);
    printf("[UDP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",
        inet_ntoa(clientaddr.sin_addr),
        ntohs(clientaddr.sin_port));

    return 0;
}

int main(int argc, char *argv[])
{
    int retval;

    // 윈속 초기화
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    // socket()
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) err_quit("socket()");

    // SO_REUSEADDR 옵션 설정
    BOOL optval = TRUE;
    retval = setsockopt(sock, SOL_SOCKET,
        SO_REUSEADDR, (char *)&optval, sizeof(optval));
    if (retval == SOCKET_ERROR) err_quit("setsockopt()");

    // bind()
    SOCKADDR_IN localaddr;
    ZeroMemory(&localaddr, sizeof(localaddr));
    localaddr.sin_family = AF_INET;
    localaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localaddr.sin_port = htons(LOCALPORT);
    retval = bind(sock, (SOCKADDR *)&localaddr, sizeof(localaddr));
    if (retval == SOCKET_ERROR) err_quit("bind()");

    // 멀티캐스트 그룹 가입
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(MULTICASTIP);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    retval = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
        (char *)&mreq, sizeof(mreq));
    if (retval == SOCKET_ERROR) err_quit("setsockopt()");

    // 데이터 통신에 사용할 변수
    SOCKADDR_IN peeraddr;
    int addrlen;
    char buf[BUFSIZE + 1];
    
    DWORD ThreadId;
    HANDLE hThread = CreateThread(NULL, 0, ProcessClient, (LPVOID)sock, 0, &ThreadId);
    // 멀티캐스트 데이터 받기
    while (1) {
        // 데이터 받기
        addrlen = sizeof(peeraddr);
        retval = recvfrom(sock, buf, BUFSIZE, 0,
            (SOCKADDR *)&peeraddr, &addrlen);
        if (retval == SOCKET_ERROR) {
            err_display("recvfrom()");
            continue;
        }

        // 받은 데이터 출력
        buf[retval] = '\0';
        printf("[UDP/%s:%d] %s\n", inet_ntoa(peeraddr.sin_addr),
            ntohs(peeraddr.sin_port), buf);
    }

    // 멀티캐스트 그룹 탈퇴
    retval = setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP,
        (char *)&mreq, sizeof(mreq));
    if (retval == SOCKET_ERROR) err_quit("setsockopt()");

    // closesocket()
    closesocket(sock);

    // 윈속 종료
    WSACleanup();
    return 0;
}
