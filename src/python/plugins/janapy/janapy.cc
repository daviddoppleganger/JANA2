//
//    File: janapy/janapy.cc
// Created: Fri Dec 21 23:05:11 EST 2018
// Creator: davidl (on Darwin amelia.jlab.org 17.7.0 i386)
//
// ------ Last repository commit info -----
// [ Date: Tue Feb 26 18:13:39 2019 -0500 ]
// [ Author: Nathan Brei <nbrei@halld3.jlab.org> ]
// [ Source: src/plugins/janapy/janapy.cc ]
// [ Revision: c15aad0b0dec2e6f8f29d1f727b2daec6c7cf376 ]
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Jefferson Science Associates LLC Copyright Notice:
// Copyright 251 2014 Jefferson Science Associates LLC All Rights Reserved. Redistribution
// and use in source and binary forms, with or without modification, are permitted as a
// licensed user provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products derived
//    from this software without specific prior written permission.
// This material resulted from work developed under a United States Government Contract.
// The Government retains a paid-up, nonexclusive, irrevocable worldwide license in such
// copyrighted data to reproduce, distribute copies to the public, prepare derivative works,
// perform publicly and display publicly and to permit others to do so.
// THIS SOFTWARE IS PROVIDED BY JEFFERSON SCIENCE ASSOCIATES LLC "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
// JEFFERSON SCIENCE ASSOCIATES, LLC OR THE U.S. GOVERNMENT BE LIABLE TO LICENSEE OR ANY
// THIRD PARTES FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#include <thread>
#include <cstdio>
#include <chrono>
#include <Python.h>
using namespace std;


#include <JANA/JApplication.h>
#include <JANA/Utils/JCpuInfo.h>
#include <JANA/Services/JParameterManager.h>

#include "pybind11/pybind11.h"
namespace py = pybind11;
#include "JEventProcessorPY.h"



void JANA_PythonModuleInit(JApplication *sApp);

static bool PY_INITIALIZED = false; // See JANA_PythonModuleInit

static PyObject *JANA_PYMODULE_OBJ = nullptr;

// This is temporary and will likely be changed once the new arrow
// system is fully adopted.
static JApplication *pyjapp = nullptr;

#ifndef _DBG__
#define _DBG__ std::cout<<__FILE__<<":"<<__LINE__<<std::endl
#define _DBG_ std::cout<<__FILE__<<":"<<__LINE__<<" "
#endif

extern "C"{
void InitPlugin(JApplication *app){
	InitJANAPlugin(app);

	// Launch thread to handle Python interpreter.
//	std::thread thr(JANA_PythonModuleInit, app);
//	thr.detach();

	// Wait for interpreter to initialize and possibly run script.
	// This allows more control from python by stalling the initialization
	// so it has a chance to modify things a bit before initialization
	// completes and data processing starts.
	while( !PY_INITIALIZED ) std::this_thread::sleep_for (std::chrono::milliseconds(100));
}
} // "C"

//.....................................................

// Trivial wrappers for JApplication (and friends)
void     janapy_AddPlugin(string plugin_name) { pyjapp->AddPlugin(plugin_name); }
void     janapy_AddPluginPath(string path) { pyjapp->AddPluginPath(path); }

void     janapy_AddEventSource(string source) { pyjapp->Add(source); }

void     janapy_Start(void) { PY_INITIALIZED = true; }
void     janapy_Initialize(void) { pyjapp->Initialize(); }
void     janapy_Run(bool wait_until_finished=true) { pyjapp->Run(wait_until_finished); }
void     janapy_Scale(int nthreads) { pyjapp->Scale(nthreads); }
void     janapy_Stop(bool wait_until_idle=false) { pyjapp->Stop(wait_until_idle); }
void     janapy_Resume(void) { pyjapp->Resume(); }
void     janapy_Quit(bool skip_join=false) { pyjapp->Quit(skip_join); }
void     janapy_SetExitCode(int exit_code) { pyjapp->SetExitCode(exit_code); }
int      janapy_GetExitCode(void) { return pyjapp->GetExitCode(); }

bool     janapy_IsInitialized(void){return pyjapp->IsInitialized();}
bool     janapy_IsQuitting(void) { return pyjapp->IsQuitting(); }
bool     janapy_IsDrainingQueues(void) { return pyjapp->IsDrainingQueues(); }

void     janapy_SetTicker(bool ticker_on = true){pyjapp->SetTicker(ticker_on);}
void     janapy_PrintStatus(){pyjapp->PrintStatus();}
void     janapy_PrintFinalReport(){ pyjapp->PrintFinalReport();}
uint64_t janapy_GetNThreads(){return pyjapp->GetNThreads();}
uint64_t janapy_GetNEventsProcessed(){return pyjapp->GetNEventsProcessed();}
float    janapy_GetIntegratedRate(){return pyjapp->GetIntegratedRate();}
float    janapy_GetInstantaneousRate(){return pyjapp->GetInstantaneousRate();}

// TODO: expose GetComponentSummary as separate routines

string   janapy_GetParameterValue(string key) { return pyjapp->GetJParameterManager()->Exists(key) ? pyjapp->GetParameterValue<string>(key):"Not Defined"; }
void     janapy_SetParameterValue(string key, string val) { pyjapp->SetParameterValue<string>( key, val ); }

size_t   janapy_GetNcores(void) { return JCpuInfo::GetNumCpus(); }

void     janapy_AddProcessor(py::object &pyproc )
{
    cout << "JANAPY2_AddProcessor called!" << endl;
    JEventProcessorPY *proc = pyproc.cast<JEventProcessorPY *>();
    pyjapp->Add(proc);
}

//================================================================================
// Module definition
// The arguments of this structure tell Python what to call your extension,
// what it's methods are and where to look for it's method definitions
PYBIND11_MODULE(janapy, m) {

    // JEventProcessor
    py::class_<JEventProcessorPY>(m, "JEventProcessor")
.def(py::init<py::object&>())
.def("Init", &JEventProcessorPY::Init)
.def("Process", &JEventProcessorPY::Process);

    // C-wrapper routines
    m.def("AddPlugin",                   &janapy_AddPlugin,                   "Add a plugin to the list of plugins to be attached (call before calling Run)");
    m.def("AddPluginPath",               &janapy_AddPluginPath,               "Add directory to plugin search path");

    m.def("AddEventSource",              &janapy_AddEventSource,              "Add an event source (e.g. filename). Can be given multiple arguments and/or called multiple times.");


    m.def("Start",                       &janapy_Start,                       "Allow JANA system to start processing data. (Not needed for short scripts.)");
    m.def("Initialize",                  &janapy_Initialize,                  "Initialize JApplication");
    m.def("Run",                         &janapy_Run,                         "Begin processing events (use when running python as an extension)");
    m.def("Scale",                       &janapy_Scale,                       "Change the number of processing threads");
    m.def("Stop",                        &janapy_Stop,                        "Tell JANA to (temporarily) stop event processing. If optional agrument is True then block until all threads are stopped.");
    m.def("Resume",                      &janapy_Resume,                      "Tell JANA to resume event processing.");

    m.def("SetExitCode",                 &janapy_SetExitCode,                 "Set the process exit code (only useful if executable supports)");
    m.def("GetExitCode",                 &janapy_GetExitCode,                 "Get the exit code the process will emit (if executable supports it)");

    m.def("IsInitialized",               &janapy_IsInitialized,               "Returns true if JApplication has been initialized");
    m.def("IsQuitting",                  &janapy_IsQuitting,                  "Returns true if JApplication is currently shutting down to prepare for quitting");
    m.def("IsDrainingQueues",            &janapy_IsDrainingQueues,            "Returns true if JApplication is currently draining its queues");


    m.def("SetTicker",                   &janapy_SetTicker,                   "Return current number of JThread objects.");
    m.def("PrintStatus",                 &janapy_PrintStatus,                 "Return current number of JThread objects.");
    m.def("PrintFinalReport",            &janapy_PrintFinalReport,            "Return current number of JThread objects.");
    m.def("GetNThreads",                 &janapy_GetNThreads,                 "Return current number of JThread objects.");
    m.def("GetNeventsProcessed",         &janapy_GetNEventsProcessed,         "Return the number of events processed so far.");
    m.def("GetIntegratedRate",           &janapy_GetIntegratedRate,           "Return integrated rate.");
    m.def("GetInstantaneousRate",        &janapy_GetInstantaneousRate,        "Return instantaneous rate.");

    m.def("GetParameterValue",           &janapy_GetParameterValue,           "Return value of given configuration parameter.");
    m.def("SetParameterValue",           &janapy_SetParameterValue,           "Set configuration parameter.");

    m.def("GetNcores",                   &janapy_GetNcores,                   "Return number of cores reported by system (full + logical).");


    m.def("AddProcessor",                &janapy_AddProcessor,                "Add an event processor");

    // Create the JApplication object
    pyjapp = new JApplication();
}


//================================================================================
// Module definition
// The arguments of this structure tell Python what to call your extension,
// what it's methods are and where to look for it's method definitions
//static struct PyModuleDef janapy__definition = {
//    PyModuleDef_HEAD_INIT,
//    "jana",
//    "JANA2 Python module.",
//    -1,
//    JANAPYMethods
//};


	// Create JApplication.
//	pyjapp = new JApplication;


//-------------------------------------
// JANA_PythonModuleInit
//-------------------------------------
void JANA_PythonModuleInit(JApplication *sApp)
{
	/// This will initialize the embedded Python interpreter and import the
	/// JANA module (defined by the routines in this file). This gets called
	/// when the janapy plugin gets initialized. It will then automatically
	/// look for a file whose name is given by either the JANA_PYTHON_FILE
	/// config. parameter or environment variable in that order. If neither
	/// exists, it looks for a file name "jana.py" in the current working
	/// directory and if found, executes that. If you do not wish to execute
	/// a python file, then you can set the JANA_PYTHON_FILE value to an empty
	/// string (or better yet, just not attach the janapy plugin!).
	///
	/// Note that janapy creates a dedicated thread that the python interpreter
	/// runs in.
	///
	/// IMPORTANT: The janapy InitPlugin routine will block (resulting in the whole
	/// JANA system pausing) until either this routine finishes or the python
	/// script it invokes calls "jana.Run()". This is to give the python script
	/// a chance to modify running conditions prior to event processing starting.
	/// If the python script intends to run throughout the life of the process,
	/// then it MUST call jana.Run() at some point. If the script is small and only
	/// runs for a short time, then you don't need to call it since it will be
	/// called automatically when the script ends.

	// Check if interpreter is already initialized and do nothing at all if it is.
	if( Py_IsInitialized() ){
		jout << "Python already initialized! Skipping initialization of janapy!" << jendl;
		PY_INITIALIZED = true;
		return;
	}

	pyjapp = sApp;

	// Initialize interpreter and register the jana module
	jout << "Initializing embedded Python ... " << jendl;
	PyEval_InitThreads();
	Py_Initialize();

	// Get name of python file to execute
	string fname = "jana.py";
	try{
		fname = pyjapp->GetParameterValue<string>("JANA_PYTHON_FILE");
	}catch(...){
		auto JANA_PYTHON_FILE = getenv("JANA_PYTHON_FILE");
		if( JANA_PYTHON_FILE ) fname = JANA_PYTHON_FILE;
	}

//	PyGILState_STATE gstate = PyGILState_Ensure();
//	PyEval_ReleaseLock();
//	PyThreadState *_save = PyEval_SaveThread();
//	Py_BEGIN_ALLOW_THREADS

	auto fil = std::fopen(fname.c_str(), "r");
	if( fil ) {
		jout << "Executing Python script: " << fname << " ..." << jendl;

		wchar_t *wargv = Py_DecodeLocale(fname.c_str(), NULL);
		PySys_SetArgv( 1, (wchar_t**)&wargv );
		PyMem_RawFree(wargv);
		PyRun_AnyFileEx( fil, nullptr, 1 );
	}else if( fname != "jana.py" ){
		jerr << "Unable to open \"" << fname << "\"! Quitting." << jendl;
		pyjapp->Quit();
		PY_INITIALIZED = true;
		std::this_thread::sleep_for(std::chrono::seconds(1));
		exit(-1);
	}

//	Py_END_ALLOW_THREADS

	PY_INITIALIZED = true;
}

