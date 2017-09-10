#include "windows.h"
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <future>
#include <memory>
#include <time.h>

using namespace std;
const int g_buff_size = 10;
const int g_batch_size = 2;
const int g_data_size = 50;
bool g_if_end = false;

struct 
{
	int buff[g_buff_size][g_batch_size];
	size_t write_position;
	size_t read_position;
	mutex buff_mux;
	condition_variable buff_not_full;
	condition_variable buff_not_empty;
	mutex is_end_mux;
	bool is_end;
}g_pool;

int poolInit()
{
	g_pool.write_position = 0;
	g_pool.read_position = 0;
	g_pool.is_end = false;
	return 0;
}

void produceData(int& index,int time)
{	
	unique_lock<mutex> lock(g_pool.buff_mux);
	g_pool.buff_not_full.wait(lock, 
		[]()->bool{ return (g_pool.write_position+1)%g_buff_size!=g_pool.read_position; });
	for (int i = 0; i < g_batch_size; i++)
	{
		g_pool.buff[g_pool.write_position][i] = index + i;
	}
	cout <<"pro"<<index << "is write" << endl;
	g_pool.write_position++;
	if (g_pool.write_position >= g_buff_size)
	{
		g_pool.write_position = 0;
	}
	g_pool.buff_not_empty.notify_all();
	lock.unlock();
	Sleep((DWORD)time);
	index++;
}

void producerTask()
{
	srand(10);
	int index = 0;
	while (index<g_data_size)
	{
		produceData(index, (int)(rand() / double(RAND_MAX) * 1000));
	}
	unique_lock<mutex> lock(g_pool.is_end_mux);
	g_pool.is_end = true;
	lock.unlock();
}

bool consumeDate(int time)
{
	int data[g_batch_size]{0};
	bool is_empty{false};
	unique_lock<mutex> lock(g_pool.buff_mux);
	g_pool.buff_not_empty.wait(lock, []()->bool{return g_pool.read_position != g_pool.write_position; });
	cout <<"Con"<< g_pool.buff[g_pool.read_position][0] << '\t' << g_pool.buff[g_pool.read_position][1] << endl;
	g_pool.read_position++;
	if (g_pool.read_position >= g_buff_size)
	{
		g_pool.read_position = 0;
	}
	if (g_pool.read_position == g_pool.write_position)
		is_empty = true;
	g_pool.buff_not_full.notify_all();
	lock.unlock();
	Sleep((DWORD)time);
	return is_empty;
}

void consumerTask()
{
	srand(1);
	while (1)
	{
		bool isEnd;
		unique_lock<mutex> lock(g_pool.is_end_mux);
		isEnd = g_pool.is_end;
		lock.unlock();
		
		if (consumeDate((int)(rand() / double(RAND_MAX) * 1000))&&isEnd)
			break;
	}
}

int main(int argc, char* argv[])
{
	
	poolInit();
	thread produceThread(producerTask);
	thread consumeThread(consumerTask);
	produceThread.join();
	consumeThread.join();
	system("PAUSE");
}