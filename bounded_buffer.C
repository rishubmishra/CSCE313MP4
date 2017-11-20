#include "bounded_buffer.H"

Bounded_buffer::Bounded_buffer(int size) {
    bufferSize = size;
    this->full = Semaphore(0);
    this->empty = Semaphore(100);
    this->lock = Semaphore(1);
    
    buffer.resize(size);
}

Bounded_buffer::~Bounded_buffer(){}

void Bounded_buffer::push(string data){
    this->empty.P(); //decrement the empty containers
    this->lock.P();
    this->buffer.push_back(data);
    this->lock.V();
    this->full.V(); //increase the full containers
}

string Bounded_buffer::pop(){
    this->full.P(); //decrement the full containters
    this->lock.P();
    string temp = buffer.front();
    this->buffer.pop_front();
    this->lock.V();
    this->empty.V(); //increase the empty containers

    return temp;

}