
#ifndef _JEVEVENTPROCESSOR_PY_H_
#define _JEVEVENTPROCESSOR_PY_H_

#include<iostream>
using std::cout;
using std::endl;

#include "pybind11/pybind11.h"
namespace py = pybind11;


#include <JANA/JEventProcessor.h>
#include <JANA/JEvent.h>

class JEventProcessorPY : public JEventProcessor{

    public:

    JEventProcessorPY(py::object &py_obj):pyobj(py_obj){

        cout << "JEventProcessorPY constructor called with py:object"  << endl;

        pymProcess = pyobj.attr("Init");
        pymInit = pyobj.attr("Process");

    }

    void Init(void){

        cout << "JEventProcessorPY::Init called " << endl;
        pymInit();

    }

    void Process(const std::shared_ptr<const JEvent>& aEvent){

        cout << "JEventProcessorPY::Process called " << endl;
        pymProcess();

    }

    py::object &pyobj;
    py::object pymProcess;
    py::object pymInit;

};

#endif  // _JEVEVENTPROCESSOR_PY_H_
