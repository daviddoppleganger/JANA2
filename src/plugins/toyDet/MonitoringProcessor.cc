//
//    File: toyDet/MonitoringProcessor.cc
// Created: Wed Apr 24 16:04:14 EDT 2019
// Creator: pooser (on Linux rudy.jlab.org 3.10.0-957.10.1.el7.x86_64 x86_64)
//
// ------ Last repository commit info -----
// [ Date ]
// [ Author ]
// [ Source ]
// [ Revision ]

#include "MonitoringProcessor.h"
#include "ADCSample.h"

//---------------------------------
// MonitoringProcessor    (Constructor)
//---------------------------------
MonitoringProcessor::MonitoringProcessor() {

}

//---------------------------------
// MonitoringProcessor (Destructor)
//---------------------------------
MonitoringProcessor::~MonitoringProcessor() {
    // close output root file
    delete m_transport;
    delete m_message;
}

//------------------
// Init
//------------------
void MonitoringProcessor::Init() {
    // This is called once at program startup.

    std::cout << "Initializing ZMQ sink" << std::endl;

    m_message = new DASEventMessage(japp);
    m_transport = new ZmqTransport(japp->GetParameterValue<std::string>("toydet:output_socket"), true);
    m_transport->initialize();
}

//------------------
// Process
//------------------
void MonitoringProcessor::Process(const std::shared_ptr<const JEvent>& aEvent) {

    auto eventData = aEvent->Get<ADCSample>();

    std::lock_guard<std::mutex> lck(fillMutex);
/*
    m_message->set_event_number(aEvent->GetEventNumber());
    m_message->set_payload_size(4);
    m_message->as_indra_message()->source_id = 22;
    m_message->as_indra_message()->payload[0] = 69;
    std::cout << "ZmqSink: Sent event " << aEvent->GetEventNumber() << "  (" << m_message->get_buffer_size() << " bytes)" << std::endl;
    m_transport->send(*m_message);
    */

    auto original = aEvent->GetSingle<DASEventMessage>();
    m_transport->send(*original);
}

//------------------
// Finish
//------------------
void MonitoringProcessor::Finish(void) {
}
