#include "Tests.cpp"
#include "PlayGame.cpp"
#include <atomic>
atomic<bool> lock(false);
#include "View.cpp"

#ifdef _PTHREAD_H
#include <pthread.h>
#endif
 
//Enter critical section
void Lock()
{
	while (atomic_exchange_explicit(&lock, true, memory_order_acquire));
}

//Exit critical section
void Unlock()
{
	atomic_store_explicit(&lock, false, memory_order_release);
}


//creates the window
int GLmain(int argc, char** argv)
{

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE);
	glutInitWindowSize(800, 800);
	glutInitWindowPosition(100, 100);
	
	glutEnterGameMode(); //prevents the user from resizing the window
	
	glutDisplayFunc(displayView);
	glutMouseFunc(onClick);
	glutIdleFunc(signal_idle);
	

	glutMainLoop(); 


	return 0;
}

void *relay(void *data)
{
	Player *(*players) [2] = (Player * ((*) [2]))(data);
	play_game((*players)[0], (*players)[1]);
	return NULL;
}

int main(int argc, char **argv)
{


	cout << "Please enter your name" << endl;
	cin.getline(human_name, 100);

	Human_Player human = Human_Player();
	CPU_Player computer = CPU_Player();

	//Three threads: the computation thread, the view, and the human player. I used a separate thread for computing the computer's moves so that
	//calculating distributions would not slow down the game. I needed the view on a separate thread from the human player since the state needed
	//to be updated continuously, not only during OpenGL callback events.
	thread t = thread(&play_game, &human, &computer);
	thread T = thread(&GLmain, argc, argv);
	
#ifdef _PTHREAD_H
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 2050000);
	pthread_t Thread;
	Player *players [2] = {&computer, &human};
	pthread_create(&Thread, &attr, relay, &players);
	t.join();
	T.join();
	pthread_join(Thread, NULL);
	
#else
	play_game(&computer, &human);	
	t.join();
	T.join();

#endif

	return 0;
}

