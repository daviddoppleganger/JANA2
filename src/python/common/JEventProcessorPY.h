//
// This class serves as an interface between the Python jana.JEventProcessor
// and the C++ JEventProcessor classes.
//
// This will redirect calls from JANA to the JEventProcessor callbacks so
// they go to the Python version and vice versa.
//
// Note on how this works:
// ------------------------
// It should first be stated that pybind11 provides macros like PYBIND11_OVERLOAD
// that are intended to make connecting the virtual methods of the derived
// Python class with the C++ class. Early testing with this resulted in seg. faults
// and bus errors that were nearly impossible to debug. Those were abandoned for
// the system descrived here.
//
// The python subclass constructor will call the JEventProcessorPY constructor with
// a single py:object reference (py:object vomes from pybind11). This reference
// represents the python object. It is stored as a member of JEventProcessorPY
// and can be used to access the python object. It is actually used right away
// in the JEventProcessorPY constructor to get references to the callback methods
// of the python object that it can use later when its own JEventProcessor callbacks
// are called from JANA.
//
// When the references to the callback methods of the python class are obtained,
// a bool for each is also set to indicate whether it is actually implemented
// in the python subclass. If it is not, then the default JEventProcessor method
// is explicitly called (which probably does nothing).
//
// Calls that initiate from python and need to get directed to C++ methods in
// JANA are generally done on methods of the JEventPY object. See JEventPY.h for
// details on how that is handled.
//

#ifndef _JEVENTPROCESSOR_PY_H_
#define _JEVENTPROCESSOR_PY_H_

#include <mutex>
#include <iostream>
using std::cout;
using std::endl;

#include <pybind11/pybind11.h>
namespace py = pybind11;


#include <JANA/JEventProcessor.h>
#include <JANA/JEvent.h>
#include "JEventPY.h"

class JEventProcessorPY : public JEventProcessor{

    public:

    //---------------------------------
    JEventProcessorPY(py::object &py_obj):pyobj(py_obj){

        if( verbose>0 ) LOG << "JEventProcessorPY constructor called with py:object : " << this  << LOG_END;

        // Get references to callback functions in python object
        try { pymInit    = pyobj.attr("Init"   );  has_pymInit    = true; }catch(...){}
        try { pymProcess = pyobj.attr("Process");  has_pymProcess = true; }catch(...){}
        try { pymFinish  = pyobj.attr("Finish" );  has_pymFinish  = true; }catch(...){}

        // This is used when Process is called to create a jana.JEvent object
        pyJEventClass = py::module::import("jana").attr("JEvent");

    }

    //---------------------------------
    ~JEventProcessorPY() {
        if( verbose>0 ) LOG << "JEventProcessorPY destructor called : " << this  << LOG_END;
    }

    //---------------------------------
    void SetVerbose(int verbose){
        this->verbose = verbose;
    }

    //---------------------------------
    void Init(void){
        /// Redirects to Init method of python jana.JEventProcessor subclass
        /// if implemented. Otherwise calls default method JEventProcessor::Init().
        if( verbose>1 ) LOG << "JEventProcessorPY::Init called " << LOG_END;
        if( has_pymInit ) {
            lock_guard<mutex> lck(pymutex);
            pymInit();
        }else{
            // Method not implemented in python class. Call default
            JEventProcessor::Init();
        }

    }

    //---------------------------------
    void Process(const std::shared_ptr<const JEvent>& aEvent){
        /// Redirects to Process method of python jana.JEventProcessor subclass
        /// if implemented. Otherwise calls default method JEventProcessor::Process(aEvent).
        if( verbose>1 ) LOG  << "JEventProcessorPY::Process called " << LOG_END;

        // According to the Python documentation we should be wrapping the call to pmProcess() below
        // in the following that activate the GIL lock. In practice, this seemed to allow each thread
        // to call pymProcess(), once, but then the program stalled. Hence, we use our own mutex.
        // PyGILState_STATE gstate = PyGILState_Ensure();
        // PyGILState_Release(gstate);
        if( has_pymProcess ) {
            try{
                // Here we need to create a jana.JEvent object in python that wraps
                // the JEvent passed to us by JANA. We first create the python object,
                // then get a pointer to its underlying JEventPY part by casting it.
                // We can then set the JEvent explicitly in the JEventPY. Note that
                // normally one would just want to pass aEvent to the JEventPY constructor.
                // However, that doesn't seem to work here since it is a C++ object
                // and the python constructor won't accept such an argument.
                // Note also that the python jana.JEvent object is automatically deleted
                // when the py_obj goes out of scope. This will not really effect aEvent
                // since its shared_ptr handles deleting it later.
                py::object py_obj = pyJEventClass(py::none()); // instantiate python jana.JEvent object
                auto* jeventpy = py_obj.cast<JEventPY*>();     // Get pointer to the C++ JEventPY object
                jeventpy->SetJEvent(aEvent);                   // Record actual JEvent in JEventPY wrapper object
                lock_guard<mutex> lck(pymutex);                // Restrict one thread at a time (see above)
                pymProcess(py_obj);                            // Call the python Process method
            }catch(std::exception &ex){
                _DBG_<<"Exception caught: " << ex.what() << std::endl;
            }
        }else{
            // Method not implemented in python class. Call default
            JEventProcessor::Process( aEvent );
        }
    }

    //---------------------------------
    void Finish(void){
        /// Redirects to Finish method of python jana.JEventProcessor subclass
        /// if implemented. Otherwise calls default method JEventProcessor::Finish().
        if( verbose>1 ) LOG  << "JEventProcessorPY::Finish called " << LOG_END;

        if( has_pymFinish ) {
            lock_guard<mutex> lck(pymutex);
            pymFinish();
        }else{
            // Method not implemented in python class. Call default
            JEventProcessor::Finish();
        }
    }

    //---------------------------------
    py::object &pyobj; // _self_
    py::object pymInit;
    py::object pymProcess;
    py::object pymFinish;
    bool has_pymInit    = false;
    bool has_pymProcess = false;
    bool has_pymFinish  = false;

    py::object pyJEventClass = py::none();

    mutex pymutex;
    int verbose=1;
};

#endif  // _JEVENTPROCESSOR_PY_H_

//-----------------------------------------------------------------------------
// UNUSED
//
// The following is left over from a failed attempt to use pybind11's
// PYBIND11_OVERLOAD macros to support mapping methods between python
// and C++ classes. It failed and this should probably be deleted, but
// I'm leaving it here for the time being in case we want to pick it
// up and give it another try at some point.
#if 0

// This part goes in janapy.h as part of the module definition
py::class_<JEventProcessor,JEventProcessorPY>(m, "JEventProcessor") \
.def(py::init<>()) \
.def("Init",    &JEventProcessor::Init) \
.def("Process", &JEventProcessor::Process) \
.def("Finish",  &JEventProcessor::Finish); \

//----- The following would actually go in this file

// Inherit constructors (see pybind11 docs https://pybind11.readthedocs.io/en/stable/advanced/classes.html)
        using JEventProcessor::JEventProcessor;

        // ----  Trampolines for virtual methods
        // Init
        void Init() override {
            PYBIND11_OVERLOAD(
                    void,            /* Return type */
                    JEventProcessor, /* Parent class */
                    Init             /* Name of function in C++ (must match Python name) */
                    );
        }

        // Process
        void Process(const std::shared_ptr<const JEvent>& aEvent) override {
            PYBIND11_OVERLOAD(
                    void,            /* Return type */
                    JEventProcessor, /* Parent class */
                    Process,         /* Name of function in C++ (must match Python name) */
                    aEvent  /* Argument(s) */
                    );
        }

        // Init
        void Finish() override {
            PYBIND11_OVERLOAD(
                    void,            /* Return type */
                    JEventProcessor, /* Parent class */
                    Finish           /* Name of function in C++ (must match Python name) */
                    );
        }
#endif
//-----------------------------------------------------------------------------
