#pragma comment(lib, "ws2_32")
#include <WinSock2.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	WSADATA wsa;
	u_short x = 0x1234;
	u_long y = 0x12345678;
	u_short x2;
	u_long y2;
	SOCKET hSock;



	// 초기화

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return -1;
	}



	hSock = socket(PF_INET, SOCK_STREAM, 0);  // 소켓 생성


   // htons*() 함수는 호스트 바이트 정렬로 > 네트워크 바이트 정렬로 변환
   // 네트워크 바이트 오더로 변환

	WSAHtons(hSock, x, &x2);
	WSAHtonl(hSock, y, &y2);

	printf("호스트 바이트 -> 네트워크 바이트 \n");
	printf("0x%x -> 0x%x \n", x, x2);
	printf("0x%x -> 0x%x \n", y, y2);



	// 호스트 바이트 오더로 변환

	WSANtohs(hSock, x2, &x);
	WSANtohl(hSock, y2, &y);

	printf("네트워크 바이트 -> 호스트 바이트 \n");
	printf("0x%x -> 0x%x\n", x2, x);
	printf("0x%x -> 0x%x\n", y2, y);

	printf("잘못 사용된 예 \n");
	printf("0x%x -> 0x%x\n", x, htonl(x));


	closesocket(hSock);
	WSACleanup();

	return 0;
}