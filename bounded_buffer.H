#include "semaphore.H"
#include <list>
#include <string>
using namespace std;


class Bounded_buffer{

  private:

    list<string> buffer;

    Semaphore full;
    Semaphore empty;
    Semaphore lock;

    int bufferSize;

  public:

    Bounded_buffer(int size);
    ~Bounded_buffer();

    void push(string data);
    string pop();

};
