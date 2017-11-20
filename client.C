/* 
    File: simpleclient.C

    Author: R. Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 2012/07/11

    Simple client main program for MP2 in CSCE 313
*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include <cassert>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdlib.h>
#include <vector>
#include <errno.h>
#include <unistd.h>
#include <string>
#include <array>
#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <thread>
#include <map>

#include "reqchannel.H"
#include "bounded_buffer.H"
#include "semaphore.H"

using namespace std;

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS -- SUPPORT FUNCTIONS */
/*--------------------------------------------------------------------------*/



string int2string(int number) {
   stringstream ss;//create a stringstream
   ss << number;//add number to the stream
   return ss.str();//return a string with the contents of the stream
}

//time functions
void print_time_diff(struct timeval * tp1, struct timeval * tp2) {
  /* Prints to stdout the difference, in seconds and museconds, between
     two timevals. 
  */

  long sec = tp2->tv_sec - tp1->tv_sec;
  long musec = tp2->tv_usec - tp1->tv_usec;
  if (musec < 0) {
    musec += 1000000;
    sec--;
  }
  printf(" [sec = %ld, musec = %ld] ", sec, musec);

}

long get_time_diff(struct timeval * tp1, struct timeval * tp2) {
  /* returns the time diff rather than print it
  */

  long sec = tp2->tv_sec - tp1->tv_sec;
  long musec = tp2->tv_usec - tp1->tv_usec;
  if (musec < 0) {
    musec += 1000000;
    sec--;
  }
  return musec;

}

//histograms

/* struct Request{
	string name; //who the request is for
	Bounded_buffer* response_buff; //where the req goes
	
	Request(string name, Bounded_buffer* response_buff ) : 
	name(name), response_buff(response_buff){}
}; */

 
//Thread parameters
struct ReqParams{
  string name; 
  int numofReqs; //num of data requests 
  Bounded_buffer* reqbuff;

  ReqParams(string name, int numofReqs, Bounded_buffer* reqbuff) : 
	name(name), numofReqs(numofReqs), reqbuff(reqbuff) {}

};

struct StatParams{
  string name;
  int numofReqs;
  vector<int>* hist;
  Bounded_buffer* statbuff;

  StatParams(string name, int numofReqs,Bounded_buffer* statbuff):
    name(name), numofReqs(numofReqs), statbuff(statbuff) {hist = new vector<int>(255,0);}
};

struct WorkParams{
  RequestChannel* chan;
  Bounded_buffer* buff;
  vector<Bounded_buffer*>* replyVec;
  WorkParams(RequestChannel* chan, Bounded_buffer* buff, vector<Bounded_buffer*>* replyVec ):
    chan(chan), buff(buff), replyVec(replyVec) {}
};

struct EventParams{
	vector<RequestChannel*> channels;
	Bounded_buffer* reqBuff;
	int numReq;
	int numChan;
	
	EventParams(vector<RequestChannel*> channels, Bounded_buffer* reqBuff, int numReq, int numChan) : 
		channels(channels), reqBuff(reqBuff), numReq(numReq), numChan(numChan) {}
};

//Thread Functions
void* reqFunc(void* arg){
  //cout << "Request Function" << endl;
  ReqParams* req = (ReqParams*) arg;
  int count = req->numofReqs;
  Bounded_buffer* reqbuff = req->reqbuff;
  string data = req->name;
  for (int i = 0; i < count; i++){
	reqbuff->push(data);
  }
} 

/* void* workFunc(void* arg){
  //cout << "Worker Function" << endl;
  //grabs data requests and forwards reply to data server
  
  WorkParams* worker = (WorkParams*) arg;
  RequestChannel* chan = worker->chan;
  Bounded_buffer* buff = worker->buff;
  vector<Bounded_buffer*>* replyVec = worker->replyVec;
  
  for(;;){
	string data = buff->pop();
	string reply = chan->send_request(data);
	
	if(data == "data Joe Smith"){
		replyVec->at(0)->push(reply);
	}
	else if (data == "data Jane Smith"){
		replyVec->at(1)->push(reply);
	}
	else if(data == "data John Doe"){
		replyVec->at(2)->push(reply);
	}
	else{
		break;
	}
  }
  chan->send_request("quit");
} */



void* statFunc(void* arg){
  //cout << "Stat function" << endl;  
  StatParams* args = (StatParams*) arg;
  
 
  while(arg){
	  string data = args->statbuff->pop();
	  if (data.compare("quit") == 0){
		  break;
	  }else{
		 args->hist->at(stoi(data))++; 
	  }
	  
	 // cout << "DATA: " << data << endl;
	 
  }

}

void* eventFunc(void* arg){
	cout << "Event Function" << endl;
	
	EventParams* event = static_cast<EventParams*>(arg);
	vector<RequestChannel*> channels = event->channels;
	Bounded_buffer* reqBuff = event->reqBuff;
	int numReq = event->numReq;
	int numChan = event->numChan;
	
	map<string,Bounded_buffer*> bufferMap;
	
	//where we are putting the responses from the channels
	vector<Bounded_buffer*> *responses;
	responses->resize(numChan);
	
	//FD stuffs
	fd_set rfds;
	int max = 0;
	int selectResponse;
	
	//counters
	int written = 0;
	int read = 0;
	
	cout << endl << "fill channels with responses" << endl;
	//fill channels with requests
	for (int i = 0; i < numChan;i++){
		string request = reqBuff->pop();
		if (request == "quit") break;
		channels.at(i)->cwrite(request);
		bufferMap.insert(pair<string,Bounded_buffer*>(request,responses->at(i)));
		written++;		
	}
	
	//event loop
	for(;;){
		FD_ZERO(&rfds);
		for(int i =0; i < numChan; i++){
			if (channels.at(i)->read_fd() > max){
				max = channels.at(i)->read_fd();
			}
			FD_SET(channels.at(i)->read_fd(), &rfds);
		}
		selectResponse = select(max+1, &rfds, NULL,NULL,NULL);
		
		if(selectResponse){
			for(int i = 0; i < numChan; i++){
				if (FD_ISSET(channels.at(i)->read_fd(), &rfds)){
					string response = channels.at(i)->cread();
					responses->at(i)->push(response);					
					read++;
				}
			}
		}
		if (read == numReq){
			break;
		}
		
	}
	
	for(int i = 0; i < numChan; i++){
		channels.at(i)->send_request("quit");
	}
}

void makeHistogram(StatParams* stat){
	cout << "Frequency count for " << stat->name << ":" << endl;
	vector<int>* data = stat->hist;
	for(int i = 0; i < data->size(); i++){
		if (data->at(i) != 0){
			cout << i*10 << "-" << (i*10)+9 << ": " << data->at(i) << endl;
		}
	}
	cout << "---------------------------------------------------------" << endl;
	
}






/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/

int main(int argc, char * argv[]) {
  int n = 10; // # of requests per person
  int b = 50; // size of request buffer
  int w = 10; // # of worker threads

  //set each thread to alive(true)
  char optChar;
  while ((optChar = getopt(argc, argv, "n:b:w:")) != -1){
    switch(optChar){
      case 'n':
        n = atoi(optarg);
        break;
      case 'b':
        b = atoi(optarg);
        break;
      case 'w':
        w = atoi(optarg);
        break;
      default:
        cout << "incorrect input, the proper flags are:" << endl;
        cout << "-n <number of data requests per person" << endl;
        cout << "-b <size of bounded buffer in requests>" << endl;
        cout << "-w <number of request channels to handeled by event handler>" << endl;
        cout << "Ex: ./client -n 10 -b 50 -w 10" << endl;
        break;
    }
  }

  int pid = fork();
  if (pid != 0){
	  execv("dataserver", NULL);
  }
	struct timeval start,end;

	cout << "Num of data request per person: " << n << endl;
	cout << "Size of bounded buffer between requests and worker threads: " << b << endl;
	cout << "Num of request channels: " << w << endl; 
	
	cout << "CLIENT STARTED:" << endl;
	cout << "Establishing control channel... " << flush;
	RequestChannel* chan = new RequestChannel("control",RequestChannel::CLIENT_SIDE);
	cout << "done." << endl;
	cout << "-----------------------------------------------------------------------" << endl;
	
	
	gettimeofday(&start,NULL);
	pthread_t joeReqThread;
	pthread_t janeReqThread;
	pthread_t johnReqThread;
	
	Bounded_buffer* reqBuff;
	
	//request thread parameters
	ReqParams* ReqParamsJoe = new ReqParams("data Joe Smith", n, reqBuff);
	ReqParams* ReqParamsJane = new ReqParams("data Jane Smith", n, reqBuff);
	ReqParams* ReqParamsJohn = new ReqParams("data John Doe", n, reqBuff);
		
	//create request threads
	//cout << "creating request threads" << endl;
	pthread_create(&joeReqThread,NULL,reqFunc,static_cast<void*>(ReqParamsJoe));
	pthread_create(&janeReqThread,NULL,reqFunc,static_cast<void*>(ReqParamsJane));
	pthread_create(&johnReqThread,NULL,reqFunc,static_cast<void*>(ReqParamsJohn));
	//usleep(100000);
	
	//create worker threads
	/* cout << "creating worker threads" << endl;
	vector<pthread_t> worker;	
	
	for (int i = 0; i < w; i++){
		string newThread = chan->send_request("newthread");
		RequestChannel *wt_chan = new RequestChannel(newThread, RequestChannel::CLIENT_SIDE);
		pthread_t wt;
		worker.push_back(wt);
		WorkParams* wt_args = new WorkParams(wt_chan, reqBuff, &replyVec);
		pthread_create(&worker.at(i),NULL, workFunc,static_cast<void*>(wt_args));	
		
	} */
	
	//create event thread
	//cout << "creating event thread" << endl;
	vector<RequestChannel*> channels;
	for(int i = 0; i < w; i++){
		string newthread = chan->send_request("newthread");
		RequestChannel* reqChan = new RequestChannel(newthread, RequestChannel::CLIENT_SIDE);
		channels.push_back(reqChan);
	}

	pthread_t event;
	EventParams* et_args = new EventParams(channels,reqBuff,n,w);
	pthread_create(&event,NULL,eventFunc,static_cast<void*>(et_args));
	usleep(1000000);
	
	//statistic thread parameters
	vector<Bounded_buffer*> replyVec;
	for (int i = 0; i < 3; i++){
		Bounded_buffer* bb = new Bounded_buffer(b);
		replyVec.push_back(bb);
	}
	
	StatParams* StatParamsJoe = new StatParams("Joe Smith", n, replyVec[0]);
	StatParams* StatParamsJane = new StatParams("Jane Smith" , n, replyVec[1]);
	StatParams* StatParamsJohn = new StatParams("John Doe", n , replyVec[2]);
	
	pthread_t joeStatThread;
	pthread_t janeStatThread;
	pthread_t johnStatThread;
	
	//creating stat threads
	//cout << "creating stat threads" << endl;
	pthread_create(&joeStatThread,NULL, statFunc,static_cast<void*>(StatParamsJoe));
	pthread_create(&janeStatThread,NULL,statFunc,static_cast<void*>(StatParamsJane));
	pthread_create(&johnStatThread,NULL, statFunc, static_cast<void*>(StatParamsJohn));

	//cout << "-----------------------------------------------------------------------" << endl;
	usleep(1000000);
	
	/* pthread_join(joeReqThread,NULL);
	pthread_join(janeReqThread,NULL);
	pthread_join(johnReqThread,NULL);
	pthread_join(event,NULL);
	pthread_join(joeStatThread,NULL);
	pthread_join(janeStatThread,NULL);
	pthread_join(johnStatThread,NULL);
	 */
	
	cout << endl <<"Joe's Histogram:" << endl;
	makeHistogram(StatParamsJoe);
	cout << endl;
	cout << "Jane's Histogram:"	<< endl;
	makeHistogram(StatParamsJane);
	cout << endl;
	cout << "John's Histogram:"	<< endl;
	makeHistogram(StatParamsJohn);
	cout << endl;
	
	
	chan->send_request("quit");
	gettimeofday(&end,NULL);
	cout << endl << "Time to finish: " << to_string(get_time_diff(&start,&end)) << " usecs" << endl;
	usleep(1000000);
 
}




  

  
