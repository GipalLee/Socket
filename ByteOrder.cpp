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



	// �ʱ�ȭ

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return -1;
	}



	hSock = socket(PF_INET, SOCK_STREAM, 0);  // ���� ����


   // htons*() �Լ��� ȣ��Ʈ ����Ʈ ���ķ� > ��Ʈ��ũ ����Ʈ ���ķ� ��ȯ
   // ��Ʈ��ũ ����Ʈ ������ ��ȯ

	WSAHtons(hSock, x, &x2);
	WSAHtonl(hSock, y, &y2);

	printf("ȣ��Ʈ ����Ʈ -> ��Ʈ��ũ ����Ʈ \n");
	printf("0x%x -> 0x%x \n", x, x2);
	printf("0x%x -> 0x%x \n", y, y2);



	// ȣ��Ʈ ����Ʈ ������ ��ȯ

	WSANtohs(hSock, x2, &x);
	WSANtohl(hSock, y2, &y);

	printf("��Ʈ��ũ ����Ʈ -> ȣ��Ʈ ����Ʈ \n");
	printf("0x%x -> 0x%x\n", x2, x);
	printf("0x%x -> 0x%x\n", y2, y);

	printf("�߸� ���� �� \n");
	printf("0x%x -> 0x%x\n", x, htonl(x));


	closesocket(hSock);
	WSACleanup();

	return 0;
}