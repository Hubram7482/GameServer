#include "pch.h"

#include <atomic>
#include <mutex>
#include <future>
#include <thread>
#include <functional>
#include <queue>
#include "ThreadManager.h"

#include "SocketUtils.h"

int main()
{
	SOCKET socket = SocketUtils::CreateSocket();
	
	SocketUtils::BindAnyAddress(socket, 7777);
	SocketUtils::Listen(socket);

	accept(socket, nullptr, nullptr);

	cout << "Client Connect!" << '\n';


	return 0;
}
