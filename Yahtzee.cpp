//#include "Demos.cpp" //TODO remove
#include "Demos.cpp"
#include <thread>
#include <atomic>
atomic<bool> lock(false);

//TODO is this an okay location for this
void Lock()
{
	while (atomic_exchange_explicit(&lock, true, memory_order_acquire));
}

void Unlock()
{
	atomic_store_explicit(&lock, false, memory_order_release);
}

int main(int argc, char *argv[])
{

	thread t = thread(GLmain, argc, argv);
	t.detach();
	Sleep(5000000);
	
}
