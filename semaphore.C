#include "semaphore.H"

Semaphore::Semaphore(int _val){
	value = _val;
	m = PTHREAD_MUTEX_INITIALIZER;
	c = PTHREAD_COND_INITIALIZER;
}

Semaphore::Semaphore(){}

int Semaphore::P(){
	pthread_mutex_lock(&this->m);
	value--;
	if (value <0){
		pthread_cond_wait(&c,&m);
	}
	pthread_mutex_unlock(&this->m);
	return value;
}

int Semaphore::V(){
	pthread_mutex_lock(&this->m);
	value++;
	if(value > 0){
		pthread_cond_signal(&c);
	}
    pthread_mutex_unlock(&this->m);
    return value;
}
