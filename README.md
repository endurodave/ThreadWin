# Win32 Thread Wrapper with Synchronized Start
A Win32 CreateThread() C++ wrapper class for synchronized thread startup and forced message queue creation.

Originally published on CodeProject at: <a href="https://www.codeproject.com/Articles/1095196/Win-Thread-Wrapper-with-Synchronized-Start"><strong>Win32 Thread Wrapper with Synchronized Start</strong></a>

<h2>Introduction</h2>

<p>All operating systems provide services to create threads or tasks. On a Windows Win32 application, the main API is <code>CreateThread()</code>. While it&rsquo;s possible to use the raw Win32 functions, I find it better to encapsulate the behavior into a class that enforces the correct behaviors.&nbsp;</p>

<p>On multithreaded systems, you sometimes need a synchronized startup of all the threads. Depending on the design, if a worker thread starts processing too soon before other threads have had a chance to initialize&nbsp;it can cause problems. Therefore, creating all threads first and simultaneously starting them with a synchronizing event solves this problem.&nbsp;</p>

<p>The Win32 thread API also has idiosyncrasies that, if not managed, can cause intermittent failures at runtime. One problem revolves around the message queue and when it&rsquo;s created. After <code>CreateThread()</code> is called the message queue isn&rsquo;t initialized immediately. The new thread needs a chance to run first. A <code>PostThreadMessage()</code> to a thread without a message queue causes the function to fail.</p>

<p>For these reasons, when designing a Win32 application it&rsquo;s beneficial to use an encapsulating class to enforce correct behavior and prevent runtime errors.&nbsp;</p>

<p>Many wrapper class implementations exist for Win32 threads. However, I found none that solves the aforementioned problems. The <code>ThreadWin </code>class provided here has the following benefits.&nbsp;</p>

<ol>
	<li><strong>Synchronized start</strong> &ndash; start all created threads simultaneously using a synchronization event.&nbsp;</li>
	<li><strong>Force create queue</strong> &ndash; force creating the thread message queue to prevent runtime errors.</li>
	<li><strong>Entry and exit</strong> &ndash; manage the thread resources and provide orderly startup and exit.&nbsp;</li>
	<li><strong>Ease of use</strong> &ndash; implement a single function Process() for a thread loop.&nbsp;</li>
</ol>

<h2>Using the Code</h2>

<p><code>ThreadWin</code> provides the Win32 thread encapsulation. The constructor allows naming the thread and controlling whether synchronized startup is desired.&nbsp;</p>

<pre lang="c++">
class ThreadWin 
{
public:
    ThreadWin (const CHAR* threadName, BOOL syncStart = TRUE);
    // &hellip;
};</pre>

<p>Inherit from the class and implement the pure virtual function <code>Process()</code>.</p>

<pre lang="c++">
virtual unsigned long Process (void* parameter) = 0;</pre>

<p><code>WorkerThread</code> has a simple message loop and shows how to inherit from <code>ThreadWin</code>.&nbsp;</p>

<pre lang="c++">
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
        while ((bRet = GetMessage(&amp;msg, NULL, WM_USER_BEGIN, WM_USER_END)) != 0)
        {
            switch (msg.message)
            {
                case WM_THREAD_MSG:
                {
                    ASSERT_TRUE(msg.wParam != NULL);

                    // Get the ThreadMsg from the wParam value
                    ThreadMsg* threadMsg = reinterpret_cast&lt;ThreadMsg*&gt;(msg.wParam);

                    // Print the incoming message
                    cout &lt;&lt; threadMsg-&gt;message.c_str() &lt;&lt; &quot; &quot; &lt;&lt; GetThreadName() &lt;&lt; endl;

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

};</pre>

<p>Creating thread objects is easy.&nbsp;</p>

<pre lang="c++">
WorkerThread workerThread1(&quot;WorkerThread1&quot;);
WorkerThread workerThread2(&quot;WorkerThread2&quot;);</pre>

<p><code>CreateThread()</code> is used to create the thread and forces the message queue to be created. The thread is now waiting for the startup synchronization event before entering the <code>Process()</code> message loop.&nbsp;</p>

<pre lang="c++">
workerThread1.CreateThread();
workerThread2.CreateThread();</pre>

<p><code>ThreadWin::StartAllThreads()</code> starts all system threads at once. The threads are now allowed to enter the <code>Process()</code> message loop.&nbsp;</p>

<pre lang="c++">
ThreadWin::StartAllThreads();</pre>

<p><code>PostThreadMessage()</code> sends data to the worker thread.</p>

<pre lang="c++">
// Create message to send to worker thread 1
ThreadMsg* threadMsg = new ThreadMsg();
threadMsg-&gt;message = &quot;Hello world!&quot;;

// Post the message to worker thread 1
workerThread1.PostThreadMessage(WM_THREAD_MSG, threadMsg);</pre>

<p>Use<code> ExitThread()</code> for an orderly thread exit and cleanup used resources.&nbsp;</p>

<pre lang="c++">
workerThread1.ExitThread();
workerThread2.ExitThread();</pre>

<h2>Implementation</h2>

<p><code>ThreadWin::CreateThread() </code>creates a thread using the Win32 <code>CreateThread()</code> API. The main thread entry function is <code>ThreadWin::RunProcess()</code>. After the thread is created, the call blocks waiting for the thread to finish creating the message queue using <code>WaitForSingleObject(m_hThreadStarted, MAX_WAIT_TIME)</code>.</p>

<pre lang="c++">
BOOL ThreadWin::CreateThread()
{
&nbsp;&nbsp; &nbsp;// Is the thread already created?
&nbsp;&nbsp; &nbsp;if (!IsCreated ())&nbsp;
&nbsp;&nbsp; &nbsp;{
&nbsp;&nbsp; &nbsp;&nbsp;&nbsp; &nbsp;m_hThreadStarted = CreateEvent(NULL, TRUE, FALSE, TEXT(&quot;ThreadCreatedEvent&quot;));&nbsp;&nbsp; &nbsp;

&nbsp;&nbsp; &nbsp;&nbsp;&nbsp; &nbsp;// Create the worker thread
&nbsp;&nbsp; &nbsp;&nbsp;&nbsp; &nbsp;ThreadParam threadParam;
&nbsp;&nbsp; &nbsp;&nbsp;&nbsp; &nbsp;threadParam.pThread&nbsp;&nbsp; &nbsp;= this;
&nbsp;&nbsp; &nbsp;&nbsp;&nbsp; &nbsp;m_hThread = ::CreateThread (NULL, 0, (unsigned long (__stdcall *)(void *))RunProcess, (void *)(&amp;threadParam), 0, &amp;m_threadId);
&nbsp;&nbsp; &nbsp;&nbsp;&nbsp; &nbsp;ASSERT_TRUE(m_hThread != NULL);

&nbsp;&nbsp; &nbsp;&nbsp;&nbsp; &nbsp;// Block the thread until thread is fully initialized including message queue
&nbsp;&nbsp; &nbsp;&nbsp;&nbsp; &nbsp;DWORD err = WaitForSingleObject(m_hThreadStarted, MAX_WAIT_TIME);&nbsp;&nbsp; &nbsp;
&nbsp;&nbsp; &nbsp;&nbsp;&nbsp; &nbsp;ASSERT_TRUE(err == WAIT_OBJECT_0);

&nbsp;&nbsp; &nbsp;&nbsp;&nbsp; &nbsp;CloseHandle(m_hThreadStarted);
&nbsp;&nbsp; &nbsp;&nbsp;&nbsp; &nbsp;m_hThreadStarted = INVALID_HANDLE_VALUE;

&nbsp;&nbsp; &nbsp;&nbsp;&nbsp; &nbsp;return m_hThread ? TRUE : FALSE;
&nbsp;&nbsp; &nbsp;}
&nbsp;&nbsp; &nbsp;return FALSE;
}</pre>

<p>The Microsoft <code>PostThreadMessasge()</code> function documentation explains how to force the message queue creation.</p>

<blockquote class="quote">
<div class="op">Quote:</div>

<p>In the thread to which the message will be posted, call PeekMessage as shown here to force the system to create the message queue.</p>

<p>PeekMessage(&amp;msg, NULL, WM_USER, WM_USER, PM_NOREMOVE)</p>
</blockquote>

<p>If you don&rsquo;t force the queue creation,<code> PostThreadMessage()</code> may fail randomly depending on how the threads initialize. You can prove this by creating a thread using <code>CreateThread()</code> and immediately post to the new thread using <code>PostThreadMessage()</code>. The return value will indicate failure since the thread wasn&rsquo;t given enough time to initialize the message queue. Placing a <code>Sleep(1000)</code> between<code> CreateThread()</code> and <code>PostThreadMessage()</code> makes it work, but its fragile. <code>ThreadWin</code> reliably solves this problem.&nbsp;</p>

<p><code>ThreadWin::RunProcess()</code> now executes on the new thread of control and forces queue creation using <code>PeekMessage()</code>. After queue creation, the waiting<code> ThreadWin::CreateThread()</code> function is released using <code>SetEvent(thread-&gt;m_hThreadStarted)</code>. If the thread instance wants a synchronized start, it now blocks using <code>WaitForSingleObject(m_hStartAllThreads, MAX_WAIT_TIME)</code>.</p>

<pre lang="c++">
int ThreadWin::RunProcess(void* threadParam)
{
&nbsp;&nbsp; &nbsp;// Extract the ThreadWin pointer from ThreadParam.
&nbsp;&nbsp; &nbsp;ThreadWin* thread;
&nbsp;&nbsp; &nbsp;thread = (ThreadWin*)(static_cast&lt;ThreadParam*&gt;(threadParam))-&gt;pThread;

    // Force the system to create the message queue before setting the event below.
    // This prevents a situation where another thread calls PostThreadMessage to post
    // a message before this thread message queue is created.
    MSG msg;
    PeekMessage(&amp;msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    // Thread now fully initialized. Set the thread started event.
    BOOL err = SetEvent(thread-&gt;m_hThreadStarted);
    ASSERT_TRUE(err != 0);

    // Using a synchronized start?
    if (thread-&gt;SYNC_START == TRUE)
    {
        // Block the thread here until all other threads are ready. A call to 
        // StartAllThreads() releases all the threads at the same time.
        DWORD err = WaitForSingleObject(m_hStartAllThreads, MAX_WAIT_TIME);
        ASSERT_TRUE(err == WAIT_OBJECT_0);
    }

    // Call the derived class Process() function to implement the thread loop.
    int retVal = thread-&gt;Process(NULL);

    // Thread loop exited. Set exit event. 
    err = SetEvent(thread-&gt;m_hThreadExited);
    ASSERT_TRUE(err != 0);    

    return retVal;
}</pre>

<p><code>ThreadWin::StartAllThreads()</code> is called to release all waiting threads.</p>

<pre lang="c++">
void ThreadWin::StartAllThreads()
{
    BOOL err = SetEvent(m_hStartAllThreads);
    ASSERT_TRUE(err != 0);
}</pre>

<p><code>ThreadWin::ExitThread()</code> posts a <code>WM_EXIT_THREAD</code> to the message queue to exit and waits for the thread to actually exit before returning using <code>WaitForSingleObject (m_hThreadExited, MAX_WAIT_TIME)</code>.</p>

<pre lang="c++">
void ThreadWin::ExitThread()
{
    if (m_hThread != INVALID_HANDLE_VALUE)
    {
        m_hThreadExited = CreateEvent(NULL, TRUE, FALSE, TEXT(&quot;ThreadExitedEvent&quot;));    

        PostThreadMessage(WM_EXIT_THREAD);

        // Wait here for the thread to exit
        if (::WaitForSingleObject (m_hThreadExited, MAX_WAIT_TIME) == WAIT_TIMEOUT)
            ::TerminateThread (m_hThread, 1);

        ::CloseHandle (m_hThread);
        m_hThread = INVALID_HANDLE_VALUE;

        ::CloseHandle (m_hThreadExited);
        m_hThreadExited = INVALID_HANDLE_VALUE;
    }
}</pre>

<h2>Conclusion</h2>

<p><code>ThreadWin </code>encapsulates the Win32 thread API in an easy to use class that enforces correct utilization and offers unique thread startup synchronization. Entry and exit features handle all the thread creation and destruction duties. The class standardizes thread usage and reduces common bugs associated with using Win32 worker threads.&nbsp;</p>


