#include "ThreadWin.h"
#include "UserMsgs.h"
#include "Fault.h"
#include <iostream>

using namespace std;

class ThreadMsg
{
public:
	string message;
};

/// @brief A simple worker thread.
class WorkerThread : public ThreadWin
{
public:
	WorkerThread(const CHAR* threadName) : ThreadWin(threadName) {}

private:
	/// The worker thread entry function
	virtual unsigned long Process (void* parameter)
	{
		MSG msg;
		BOOL bRet;
		while ((bRet = GetMessage(&msg, NULL, WM_USER_BEGIN, WM_USER_END)) != 0)
		{
			switch (msg.message)
			{
				case WM_THREAD_MSG:
				{
					ASSERT_TRUE(msg.wParam != NULL);

					// Get the ThreadMsg from the wParam value
					ThreadMsg* threadMsg = reinterpret_cast<ThreadMsg*>(msg.wParam);

					// Print the incoming message
					cout << threadMsg->message.c_str() << " " << GetThreadName() << endl;

					// Delete dynamic data passed through message queue
					delete threadMsg;
					break;
				}

				case WM_EXIT_THREAD:
					return 0;

				default:
					ASSERT();
			}
		}
		return 0;
	}

};

// Worker thread instances
WorkerThread workerThread1("WorkerThread1");
WorkerThread workerThread2("WorkerThread2");

//------------------------------------------------------------------------------
// main
//------------------------------------------------------------------------------
int main(void)
{	
	// Create worker threads
	workerThread1.CreateThread();
	workerThread2.CreateThread();

	// Start all worker threads
	ThreadWin::StartAllThreads();

	// Create message to send to worker thread 1
	ThreadMsg* threadMsg = new ThreadMsg();
	threadMsg->message = "Hello world!";

	// Post the message to worker thread 1
	workerThread1.PostThreadMessage(WM_THREAD_MSG, threadMsg);

	// Create message to send to worker thread 2
	threadMsg = new ThreadMsg();
	threadMsg->message = "Hello world!";

	// Post the message to worker thread 2
	workerThread2.PostThreadMessage(WM_THREAD_MSG, threadMsg);

	// Give time for messages processing on worker threads
	Sleep(1000);

	workerThread1.ExitThread();
	workerThread2.ExitThread();

	return 0;
}

