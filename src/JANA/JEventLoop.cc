// $Id: JEventLoop.cc 1709 2006-04-26 20:34:03Z davidl $
//
//    File: JEventLoop.cc
// Created: Wed Jun  8 12:30:51 EDT 2005
// Creator: davidl (on Darwin wire129.jlab.org 7.8.0 powerpc)
//

#include <cstdio>
#include <iostream>
#include <iomanip>
#include <map>
using namespace std;

#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include <sys/time.h>


#include "JApplication.h"
#include "JEventLoop.h"
#include "JEvent.h"
#include "JFactory.h"
#include "JStreamLog.h"
using namespace jana;

jmp_buf SETJMP_ENV;

// Thread commits suicide when it receives HUP signal
void thread_HUP_sighandler(int sig)
{
	jerr<<"Caught HUP signal for thread 0x"<<hex<<pthread_self()<<dec<<" thread exiting..."<<endl;
	pthread_exit(NULL);
}

//---------------------------------
// JEventLoop    (Constructor)
//---------------------------------
JEventLoop::JEventLoop(JApplication *app)
{
	// Last Resort exit strategy: If this thread stops responding, the
	// main thread will send it a HUP signal to tell it to exit immediately.
	signal(SIGHUP, thread_HUP_sighandler);

	this->app = app;
	app->AddJEventLoop(this, heartbeat);
	event.SetJEventLoop(this);
	initialized = false;
	print_parameters_called = false;
	pause = 0;
	quit = 0;
	auto_free = 1;
	pthread_id = pthread_self();
	
	Nevents = 0;
	Nevents_rate = 0;
	delta_time_single = 0.0;
	delta_time_rate = 0.0;
	delta_time = 0.0;
	rate_instantaneous = 0.0;
	rate_integrated = 0.0;
		
	// Initialize the caller strings for when we record the call stack
	// these will get over written twice with each call to Get(). Once
	// to copy in who is being called and then again later to copy
	// back in who did the calling.
	caller_name = "DEventProcessor";
	caller_tag = "";
}

//---------------------------------
// ~JEventLoop    (Destructor)
//---------------------------------
JEventLoop::~JEventLoop()
{
	// Remove us from the JEventLoop's list. The application exits
	// when there are no more JEventLoops registered with it.
	app->RemoveJEventLoop(this);

	// Call all factories' fini methods
	for(unsigned int i=0; i<factories.size(); i++){
		try{
			if(factories[i]->init_was_called())factories[i]->fini();
		}catch(jerror_t err){
			jerr<<endl;
			_DBG_<<" Error thrown ("<<err<<") from JFactory<";
			jerr<<factories[i]->GetDataClassName()<<">::fini()"<<endl;
		}
	}

	// Delete all of the factories
	for(unsigned int i=0; i<factories.size(); i++){
		try{
			delete factories[i];
		}catch(jerror_t err){
			jerr<<endl;
			_DBG_<<" Error thrown ("<<err<<") while deleting JFactory<";
			jerr<<factories[i]->GetDataClassName()<<">"<<endl;
		}
	}

	factories.clear();
}

//-------------
// RefreshProcessorListFromJApplication
//-------------
void JEventLoop::RefreshProcessorListFromJApplication(void)
{
	processors = app->GetProcessors();
}

//-------------
// AddFactory
//-------------
jerror_t JEventLoop::AddFactory(JFactory_base* factory)
{
	factory->SetJEventLoop(this);
	factory->SetJApplication(app);
	factories.push_back(factory);

	return NOERROR;
}

//-------------
// RemoveFactory
//-------------
jerror_t JEventLoop::RemoveFactory(JFactory_base* factory)
{
	vector<JFactory_base*>::iterator iter = factories.begin();
	for(;iter!=factories.end(); iter++){
		if(*iter == factory){
			factories.erase(iter);
			break;
		}
	}

	return NOERROR;
}

//-------------
// GetFactory
//-------------
JFactory_base* JEventLoop::GetFactory(const string data_name, const char *tag, bool allow_deftag)
{
	// Try replacing specified tag with a default supplied at run time. This is the default
	// behavior and should mirror how the Get() method works.
	if(allow_deftag){
		// Check if a tag was specified for this data type to use for the default.
		const char *mytag = tag==NULL ? "":tag; // prtection against NULL tags
		if(strlen(mytag)==0){
			map<string, string>::const_iterator iter = default_tags.find(data_name);
			if(iter!=default_tags.end())tag = iter->second.c_str();
		}
	}

	// Search for specified factory and return pointer to it
	vector<JFactory_base*>::iterator iter = factories.begin();
	for(; iter!=factories.end(); iter++){
		if(data_name == (*iter)->GetDataClassName()){
			if(!strcmp((*iter)->Tag(), tag)){
				return *iter;
			}
		}
	}

	// No factory found. Return NULL
	return NULL;
}

//-------------
// GetFactoryNames
//-------------
void JEventLoop::GetFactoryNames(vector<string> &factorynames)
{
	/// Fill the given vector<string> with the complete
	/// list of factories. The values are in "name:tag"
	/// format where the ":tag" is ommitted for factories
	/// with no tag. 
	vector<JFactory_base*>::iterator factory = factories.begin();
	for(; factory!=factories.end(); factory++){
		string name = (*factory)->GetDataClassName();
		string tag = (*factory)->Tag()==NULL ? "":(*factory)->Tag();
		if(tag.size()>0)name = name + ":" + tag;
		factorynames.push_back(name);
	}	
}

//-------------
// GetFactoryNames
//-------------
void JEventLoop::GetFactoryNames(map<string,string> &factorynames)
{
	/// Fill the given map<string, string> with the complete
	/// list of factories. The key is the name of the data class
	/// produced by the factory. The value is the tag.
	vector<JFactory_base*>::iterator factory = factories.begin();
	for(; factory!=factories.end(); factory++){
		string name = (*factory)->GetDataClassName();
		string tag = (*factory)->Tag();
		factorynames[name] = tag;
	}	
}

//-------------
// ClearFactories
//-------------
jerror_t JEventLoop::ClearFactories(void)
{
	/// Loop over all factories and call their Reset() methods.
	/// Amoung other things, this will clear their evnt_called flags
	/// This is called from JEventLoop at the
	/// begining of a new event.

	vector<JFactory_base*>::iterator iter = factories.begin();
	for(; iter!=factories.end(); iter++){
		(*iter)->Reset();
	}

	return NOERROR;
}

//-------------
// PrintFactories
//-------------
jerror_t JEventLoop::PrintFactories(int sparsify)
{
	/// Print a list of all registered factories to the screen
	/// along with a little info about each.

	jout<<endl;
	jout<<"Registered factories: ("<<factories.size()<<" total)"<<endl;
	jout<<endl;
	jout<<"Name:             nrows:  tag:"<<endl;
	jout<<"---------------- ------- --------------"<<endl;

	for(unsigned int i=0; i<factories.size(); i++){
		JFactory_base *factory = factories[i];

		try{
			if(sparsify)
				if(factory->GetNrows()<1)continue;
		}catch(...){}
		
		// To make things look pretty, copy all values into the buffer "str"
		string str(79,' ');
		string name = factory->GetDataClassName();
		str.replace(0, name.size(), name);

		char num[32]="";
		try{
			sprintf(num, "%d", factory->GetNrows());
		}catch(...){}
		str.replace(22-strlen(num), strlen(num), num);

		string tag = factory->Tag()==NULL ? "":factory->Tag();
		//const char *tag = factory->Tag();
		if(tag.size()>0){
			tag = "\"" + tag + "\"";
			//char tag_str[256];
			//sprintf(tag_str, "\"%s\"", tag);
			//str.replace(26, strlen(tag_str), tag_str);
			str.replace(26, tag.size(), tag);
		}
		
		jout<<str<<endl;
	}
	
	jout<<endl;

	return NOERROR;
}

//-------------
// Print
//-------------
jerror_t JEventLoop::Print(const string data_name, const char *tag)
{
	/// Dump the data to stdout for the specified factory
	///
	/// Find the factory corresponding to data_name and send
	/// the return value of its toString() method to stdout.

	// Search for specified factory and return pointer to it's data container
	JFactory_base *factory = GetFactory(data_name,tag);
	if(!factory){
		jerr<<" ERROR -- Factory not found for class \""<<data_name<<"\""<<endl;
		return NOERROR;
	}
	
	string str = factory->toString();
	if(str=="")return NOERROR;
	
	jout<<factory->GetDataClassName()<<":"<<factory->Tag()<<endl;
	jout<<str<<endl;;

	return NOERROR;
}

//-------------
// GetJCalibration
//-------------
JCalibration* JEventLoop::GetJCalibration(void)
{
	return app->GetJCalibration(event.GetRunNumber());
}

//-------------
// GetJGeometry
//-------------
JGeometry* JEventLoop::GetJGeometry(void)
{
	return app->GetJGeometry(event.GetRunNumber());
}

//-------------
// PrintErrorCallStack
//-------------
void JEventLoop::PrintErrorCallStack(void)
{
	// Create a list of the call strings while finding the longest one
	vector<string> routines;
	unsigned int max_length = 0;
	for(unsigned int i=0; i<error_call_stack.size(); i++){
		string routine = error_call_stack[i].factory_name;
		if(error_call_stack[i].tag){
			if(strlen(error_call_stack[i].tag)){
				routine = routine + ":" + error_call_stack[i].tag;
			}
		}
		if(routine.size()>max_length) max_length = routine.size();
		routines.push_back(routine);
	}

	stringstream sstr;
	sstr<<" Factory Call Stack"<<endl;
	sstr<<"============================"<<endl;
	for(unsigned int i=0; i<error_call_stack.size(); i++){
		string routine = routines[i];
		sstr<<" "<<routine<<string(max_length+2 - routine.size(),' ');
		if(error_call_stack[i].filename){
			sstr<<"--  "<<" line:"<<error_call_stack[i].line<<"  "<<error_call_stack[i].filename;
		}
		sstr<<endl;
	}
	sstr<<"----------------------------"<<endl;
	
	jout<<sstr.str();
}

//-------------
// Initialize
//-------------
void JEventLoop::Initialize(void)
{
	initialized = true;

	// Copy the event processor list to our local vector
	RefreshProcessorListFromJApplication();

	app->GetJParameterManager()->GetParameters(default_tags, "DEFTAG:");
	app->GetJParameterManager()->GetParameter( "RECORD_CALL_STACK", record_call_stack);
	
	auto_activated_factories = app->GetAutoActivatedFactories();
}

//-------------
// Loop
//-------------
jerror_t JEventLoop::Loop(void)
{
	/// Loop over events until Quit() method is called or we run
	/// out of events.
	
	do{
		// Let main thread know we're alive
		*heartbeat = 0.0;

		// Handle pauses and quits
		while(pause){
			*heartbeat = 0.0;	// Let main thread know we're alive
			usleep(500000);
			if(quit)break;
		}
		if(quit)break;
		
		// Read in a new event
		switch(OneEvent()){
			case NO_MORE_EVENT_SOURCES:
				// No more events. Time to quit
				quit = 1;
				break;
			case NOERROR:
				// Don't need to do anything here
				break;
			default:
				break;
		}
	
	}while(!quit);
	
	return NOERROR;
}

//-------------
// OneEvent
//-------------
jerror_t JEventLoop::OneEvent(void)
{
	/// Read in and process one event.

	if(!initialized)Initialize();
	
	// Get timer value at start of event
	struct itimerval tmr;
	getitimer(ITIMER_REAL, &tmr);
	double start_time = tmr.it_value.tv_sec + tmr.it_value.tv_usec/1.0E6;

	// Clear evnt_called flag in all factories
	ClearFactories();

	// Try to read in an event
	jerror_t err = app->NextEvent(event);
	
	// Here is a bit of voodoo. Calling setjmp() records the entire stack
	// so that a subsequent call to longjmp() will return us right here.
	// If we're returning here from a longjmp() call, then the return
	// value of setjmp() will be non-zero. The longjmp() call is made
	// from the SIGNINT interrupt handler so infinite loops can be
	// broken out of and the program will still gracefully exit.
	if(setjmp(SETJMP_ENV)){
		jerr<<endl<<"Uh-oh, seems the way-back machine was activated. Bailing this thread"<<endl<<endl;
		err = NO_MORE_EVENT_SOURCES;
	}
	
	switch(err){
		case NOERROR:
			break;
		case NO_MORE_EVENT_SOURCES:
			jout<<endl<<"No more event sources"<<endl;
			break;
		case EVENT_NOT_IN_MEMORY:
			jerr<<endl<<"Event not in memory"<<endl;
			break;
		default:
			break;
	}
	if(err != NOERROR && err !=EVENT_NOT_IN_MEMORY)return err;
		
	// Initialize the factory call stacks
	error_call_stack.clear();
	if(record_call_stack)call_stack.clear();
	
	// Loop over the list of factories to "auto activate" and activate them
	for(unsigned int i=0; i<auto_activated_factories.size(); i++){
		pair<string, string> &facname = auto_activated_factories[i];
		JFactory_base *fac = GetFactory(facname.first, (facname.second).c_str());
		if(fac)fac->GetNrows();
	}

	// Call Event Processors
	int event_number = event.GetEventNumber();
	int run_number = event.GetRunNumber();
	vector<JEventProcessor*>::iterator p = processors.begin();

	for(; p!=processors.end(); p++){
		JEventProcessor *proc = *p;

		// Call brun routine if run number has changed or it's not been called
		proc->LockState();
		if(run_number!=proc->GetBRUN_RunNumber()){
			if(proc->brun_was_called() && !proc->erun_was_called()){
				proc->erun();
				proc->Set_erun_called();
			}
			proc->Clear_brun_called();
		}
		if(!proc->brun_was_called()){
			proc->brun(this, run_number);
			proc->Set_brun_called();
			proc->Clear_erun_called();
			proc->SetBRUN_RunNumber(run_number);
		}
		proc->UnlockState();

		// Call the event routine
		try{
			proc->evnt(this, event_number);
		}catch(JException *exception){
			error_call_stack_t cs = {"JEventLoop", "OneEvent", __FILE__, __LINE__};
			error_call_stack.push_back(cs);
			PrintErrorCallStack();
			throw exception;
		}
	}

	if(auto_free)event.FreeEvent();
	
	// Get timer value at end of event and record rates
	getitimer(ITIMER_REAL, &tmr);
	double end_time = tmr.it_value.tv_sec + tmr.it_value.tv_usec/1.0E6;
	delta_time_single = end_time - start_time;
	delta_time_rate += delta_time_single;
	Nevents_rate++;
	delta_time += delta_time_single;
	Nevents++;
	if(delta_time_rate>2.0 && Nevents_rate>0){
		rate_instantaneous = (double)Nevents_rate/delta_time_rate;
		Nevents_rate = 0;
		delta_time_rate = 0.0;
	}
	if(delta_time>0.5){
		rate_integrated = (double)Nevents/delta_time;
	}
	
	// We want to print the parameters after the first event so that defaults
	// set by init or brun methods or even data objects created during the
	// processing of the event will be included. This is not foolproof since
	// some of these things may not occur until many events in. This also 
	// pushes any error messages from the first event further up the screen.
	if(!print_parameters_called){
		print_parameters_called = true;
		app->GetJParameterManager()->PrintParameters();
	}

	return NOERROR;
}

//-------------
// QuitProgram
//-------------
void JEventLoop::QuitProgram(void)
{
	app->Quit();
}

//-------------
// FindByID
//-------------
const JObject* JEventLoop::FindByID(JObject::oid_t id)
{
	// Loop over all factories and all objects until the one
	// with the speficied id is found. Return NULL if it is not found
	for(unsigned int i=0; i<factories.size(); i++){
		const JObject *obj = factories[i]->GetByID(id);
		if(obj)return obj;
	}

	return NULL;
}

//-------------
// FindOwner
//-------------
JFactory_base* JEventLoop::FindOwner(const JObject *obj)
{
	// Loop over all factories and all objects until
	// the specified one is found. Return NULL if it is not found
	if(!obj)return NULL;
	for(unsigned int i=0; i<factories.size(); i++){
		const JObject *my_obj = factories[i]->GetByID(obj->id);
		if(my_obj)return factories[i];
	}

	return NULL;
}

//-------------
// FindOwner
//-------------
JFactory_base* JEventLoop::FindOwner(JObject::oid_t id)
{
	// Loop over all factories and all objects until
	// the speficied one is found. Return NULL if it is not found
	for(unsigned int i=0; i<factories.size(); i++){
		const JObject *my_obj = factories[i]->GetByID(id);
		if(my_obj)return factories[i];
	}

	return NULL;
}


