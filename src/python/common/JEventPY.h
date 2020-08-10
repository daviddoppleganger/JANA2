//
// This class serves as an interface between the Python JEvent and
// the C++ JEvent classes. It wraps a C++ JEvent object by keeping
// a pointer to it and dispatches calls from the Python object to
// the C++ JEvent object it is wrapping.
//
// Note: Unlike the JEventProcessorPY class, the JEventPY methods
// are all expected to have calls initiated from Python that then
// lead to C++ methods being called.
//

#ifndef _JEVENT_PY_H_
#define _JEVENT_PY_H_

#include <mutex>
#include <iostream>
using std::cout;
using std::endl;

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
namespace py = pybind11;


#include <JANA/JEvent.h>

class JEventPY {

    public:

    //---------------------------------
    // Constructor used by C++: simply saves copy of JEvent pointer
    JEventPY(){}

    //---------------------------------
    // This constructor is not used but pybind11 requires it in case one
    // instantiated one of these from python. (We always instantiate from C++)
    JEventPY(py::object &py_obj){}

    //---------------------------------
    ~JEventPY() {
        if( verbose>1 ) LOG  << "JEventPY destructor called : " << this  << LOG_END;
    }
    //---------------------------------
    void SetJEvent(const std::shared_ptr<const JEvent>& aEvent){
        if( verbose>1 ) LOG  << "JEventPY::SetJEvent called : " << this  << LOG_END;
        this->jevent = aEvent;
    }

    //---------------------------------
    void SetVerbose(int verbose){
        this->verbose = verbose;
    }

    //---------------------------------
    uint32_t GetRunNumber(void){
        if( verbose>1 ) LOG  << "JEventPY::GetRunNumber called " << LOG_END;
        return jevent->GetRunNumber();
    }

    //---------------------------------
    uint64_t GetEventNumber(void){
        if( verbose>1 ) LOG  << "JEventPY::GetEventNumber called " << LOG_END;
        return jevent->GetEventNumber();
    }

    //---------------------------------
    py::object Get(py::object &cls, const std::string& tag = "") const {
        if (verbose > 1) LOG << "JEventPY::Get called requesting objects of type: "<< py::str(cls) << LOG_END;

        // TODO: We need a mechanism (service?) for plugins to provide
        //       a list of python objects it knows about. All plugins
        //       would then be given the opportunity to provide the
        //       python objects if they can.

        // For now, just check if it is jana.JTestEventData and return
        // vector of ints if it is.
        if( py::str(cls) == "jana.JTestEventData"){
            // vector<const JTestEventData*> v;
            // jevent->Get(v, tag);
            //----- TEMPORARY: Just return some ints ------
            std::vector<int> myvec = {1,2,3,4};
            return py::cast(myvec);
        }

        // TODO: Throw python exception if type not known
        //       for now, return empty vector
        std::vector<int> myvec;
        return py::cast(myvec);
    }

    //---------------------------------
    py::object GetSingle(const std::string type, const std::string& tag = "") const{
        if( verbose>1 ) LOG  << "JEventPY::GetSingle called " << LOG_END;
        return pybind11::none();
    }

    //---------------------------------
    py::object GetAll(const std::string type, const std::string& tag = "") const{
        if( verbose>1 ) LOG  << "JEventPY::GetAll called " << LOG_END;
        return pybind11::none();
    }

    //---------------------------------
    py::object Insert(py::object &pyobj, const std::string& tag = "") const{
        if( verbose>1 ) LOG  << "JEventPY::Insert called " << LOG_END;
        return pybind11::none();
    }

    std::shared_ptr<const JEvent> jevent;
    int verbose=1;
};

#endif  // _JEVENT_PY_H_
