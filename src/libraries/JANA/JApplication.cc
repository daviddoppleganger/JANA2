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

#include <JANA/JApplication.h>

#include <JANA/JEventProcessor.h>
#include <JANA/JEventSource.h>
#include <JANA/JEventSourceGenerator.h>
#include <JANA/JFactoryGenerator.h>

<<<<<<< HEAD
#include <JANA/Services/JParameterManager.h>
#include <JANA/Services/JPluginLoader.h>
#include <JANA/Services/JComponentManager.h>
#include <JANA/Engine/JArrowProcessingController.h>
#include <JANA/Engine/JDebugProcessingController.h>
#include <JANA/Utils/JCpuInfo.h>
=======
//---------------------------------
// ~JApplication    (Destructor)
//---------------------------------
JApplication::~JApplication()
{
	for( auto p: _factoryGenerators_to_delete     ) delete p;
	if( _pmanager           ) delete _pmanager;
	if( _threadManager      ) delete _threadManager;
	if( _eventSourceManager ) delete _eventSourceManager;
}
>>>>>>> 5aff40ac1dd9cc997b10a38789910289c45843b1

JApplication *japp = nullptr;


JApplication::JApplication(JParameterManager* params) {

    if (params == nullptr) {
        _params = std::make_shared<JParameterManager>();
    }
    else {
        _params = std::shared_ptr<JParameterManager>(params);
    }

    _service_locator.provide(_params);
    _service_locator.provide(std::make_shared<JLoggingService>());
    _service_locator.provide(std::make_shared<JPluginLoader>(this));
    _service_locator.provide(std::make_shared<JComponentManager>(this));

    _plugin_loader = _service_locator.get<JPluginLoader>();
    _component_manager = _service_locator.get<JComponentManager>();

    _logger = _service_locator.get<JLoggingService>()->get_logger("JApplication");
    _logger.show_classname = false;
}


JApplication::~JApplication() {}


// Loading plugins

void JApplication::AddPlugin(std::string plugin_name) {
    _plugin_loader->add_plugin(plugin_name);
}

void JApplication::AddPluginPath(std::string path) {
    _plugin_loader->add_plugin_path(path);
}


// Building a ProcessingTopology

void JApplication::Add(JEventSource* event_source) {
    _component_manager->add(event_source);
}

void JApplication::Add(JEventSourceGenerator *source_generator) {
    /// Add the given JFactoryGenerator to the list of queues
    ///
    /// @param source_generator pointer to source generator to add. Ownership is passed to JApplication
    _component_manager->add(source_generator);
}

void JApplication::Add(JFactoryGenerator *factory_generator) {
    /// Add the given JFactoryGenerator to the list of queues
    ///
    /// @param factory_generator pointer to factory generator to add. Ownership is passed to JApplication
    _component_manager->add(factory_generator);
}

void JApplication::Add(JEventProcessor* processor) {
    _component_manager->add(processor);
}

void JApplication::Add(std::string event_source_name) {
    _component_manager->add(event_source_name);
}


<<<<<<< HEAD
// Controlling processing
=======
	// Setup all queues and attach plugins
	Initialize();

	// If something went wrong in Initialize() then we may be quitting early.
	if(_quitting) return;

	// Create all remaining threads (one may have been created in Init)
	LOG_INFO(_logger) << "Creating " << _nthreads << " processing threads ..." << LOG_END;
	_threadManager->CreateThreads(_nthreads);

	// Optionally set thread affinity
	try{
		int affinity_algorithm = 0;
		_pmanager->SetDefaultParameter("AFFINITY", affinity_algorithm, "Set the thread affinity algorithm. 0=none, 1=sequential, 2=core fill");
		_threadManager->SetThreadAffinity( affinity_algorithm );
	}catch(...){
		LOG_ERROR(_logger) << "Unknown exception in JApplication::Run attempting to set thread affinity" << LOG_END;
	}

	// Print summary of all config parameters (if any aren't default)
	GetJParameterManager()->PrintParameters(true);

	// Start all threads running
	jout << "Start processing ..." << endl;
	mRunStartTime = std::chrono::high_resolution_clock::now();
	_threadManager->RunThreads();

	// Monitor status of all threads
	while( !_quitting ){

		// If we are finishing up (all input sources are closed, and are waiting for all events to finish processing)
		// This flag is used by the integrated rate calculator
		// The JThreadManager is in charge of telling all the threads to end
		if(!_draining_queues)
			_draining_queues = _eventSourceManager->AreAllFilesClosed();

		// Check if all threads have finished
		if(_threadManager->AreAllThreadsEnded())
		{
			std::cout << "All threads have ended.\n";
			break;
		}

		// Sleep a few cycles
		std::this_thread::sleep_for( std::chrono::milliseconds(500) );

		// Print status
		if( _ticker_on ) PrintStatus();
	}

	// Join all threads
	jout << "Event processing ended. " << endl;
	if (!_skip_join) {
		cout << "Merging threads ..." << endl;
		_threadManager->JoinThreads();
	}

	// Finish event processors and delete any we own
	for( auto sProcessor : _eventProcessors           ) sProcessor->Finish(); // (this may not be necessary since it is always next to destructor)
    for( auto sProcessor : _eventProcessors_to_delete ) delete sProcessor;
    _eventProcessors.clear();
    _eventProcessors_to_delete.clear();

	// Report Final numbers
	PrintFinalReport();
}
>>>>>>> 5aff40ac1dd9cc997b10a38789910289c45843b1

void JApplication::Initialize() {

    /// Initialize the application in preparation for data processing.
    /// This is called by the Run method so users will usually not
    /// need to call this directly.

    try {
        // Only run this once
        if (_initialized) return;

        // Attach all plugins
        _plugin_loader->attach_plugins(_component_manager.get());

        // Set desired nthreads
        _desired_nthreads = JCpuInfo::GetNumCpus();
        _params->SetDefaultParameter("nthreads", _desired_nthreads, "The total number of worker threads");
        _params->SetDefaultParameter("jana:extended_report", _extended_report);

        _component_manager->resolve_event_sources();

        int engine_choice = 0;
        _params->SetDefaultParameter("jana:engine", engine_choice, "0: Arrow engine, 1: Debug engine");

<<<<<<< HEAD
        if (engine_choice == 0) {
            auto topology = JArrowTopology::from_components(_component_manager, _params, _desired_nthreads);
            auto japc = std::make_shared<JArrowProcessingController>(topology);
            _service_locator.provide(japc);  // Make concrete class available via SL
            _processing_controller = _service_locator.get<JArrowProcessingController>();  // Get deps from SL
            _service_locator.provide(_processing_controller);  // Make abstract class available via SL
        }
        else {
            auto jdpc = std::make_shared<JDebugProcessingController>(_component_manager.get());
            _service_locator.provide(jdpc);  // Make the concrete class available via SL
            _processing_controller = _service_locator.get<JDebugProcessingController>();  // Get deps from SL
            _service_locator.provide(_processing_controller); // Make abstract class available via SL
        }

        _processing_controller->initialize();
        _initialized = true;
    }
    catch (JException& e) {
        LOG_FATAL(_logger) << e << LOG_END;
        exit(-1);
    }
}

void JApplication::Run(bool wait_until_finished) {
=======
//---------------------------------
// Add - JFactoryGenerator
//---------------------------------
void JApplication::Add(JFactoryGenerator *factory_generator, bool auto_delete)
{
	/// Add the given JFactoryGenerator to the list of queues
	///
	/// @param factory_generator pointer to factory generator to add. Ownership is passed to JApplication
    /// @param auto_delete Ownership is passed to JApplication (default)

	_factoryGenerators.push_back( factory_generator );
    if( auto_delete ) _factoryGenerators_to_delete.push_back( factory_generator );
}

//---------------------------------
// Add - JEventProcessor
//---------------------------------
void JApplication::Add(JEventProcessor *processor, bool auto_delete)
{
    /// Add the given JFactoryGenerator to the list of queues
    ///
    /// @param processor pointer to an event processor to add.
    /// @param auto_delete Ownership is passed to JApplication (default)
	_eventProcessors.push_back( processor );
	if( auto_delete ) _eventProcessors_to_delete.push_back( processor );
	mNumProcessorsAdded++;
}
>>>>>>> 5aff40ac1dd9cc997b10a38789910289c45843b1

    Initialize();
    if(_quitting) return;

    // Print summary of all config parameters (if any aren't default)
    _params->PrintParameters(false);

    LOG_INFO(_logger) << GetComponentSummary() << LOG_END;
    LOG_INFO(_logger) << "Starting processing ..." << LOG_END;
    _processing_controller->run(_desired_nthreads);

    if (!wait_until_finished) {
        return;
    }

    // Monitor status of all threads
    while (!_quitting) {

        // If we are finishing up (all input sources are closed, and are waiting for all events to finish processing)
        // This flag is used by the integrated rate calculator
        if (!_draining_queues) {
            bool draining = true;
            for (auto evt_src : _component_manager->get_evt_srces()) {
                draining &= (evt_src->GetStatus() == JEventSource::SourceStatus::Finished);
            }
            _draining_queues = draining;
        }

        // Run until topology is deactivated, either because it finished or because another thread called stop()
        if (_processing_controller->is_stopped() || _processing_controller->is_finished()) {
            LOG_INFO(_logger) << "All threads have ended." << LOG_END;
            break;
        }

        // Sleep a few cycles
        std::this_thread::sleep_for(_ticker_interval);

        // Print status
        if( _ticker_on ) PrintStatus();
    }

    // Join all threads
    if (!_skip_join) {
        LOG_INFO(_logger) << "Merging threads ..." << LOG_END;
        _processing_controller->wait_until_stopped();
    }

    LOG_INFO(_logger) << "Event processing ended." << LOG_END;
    PrintFinalReport();
}


void JApplication::Scale(int nthreads) {
    _processing_controller->scale(nthreads);
}

void JApplication::Stop(bool wait_until_idle) {
    _processing_controller->request_stop();
    if (wait_until_idle) {
        _processing_controller->wait_until_stopped();
    }
}

void JApplication::Quit(bool skip_join) {
    _skip_join = skip_join;
    _quitting = true;
    if (_processing_controller != nullptr) {
        Stop(skip_join);
    }
}

void JApplication::SetExitCode(int exit_code) {
    /// Set a value of the exit code in that can be later retrieved
    /// using GetExitCode. This is so the executable can return
    /// a meaningful error code if processing is stopped prematurely,
    /// but the program is able to stop gracefully without a hard
    /// exit. See also GetExitCode.

    _exit_code = exit_code;
}

int JApplication::GetExitCode() {
	/// Returns the currently set exit code. This can be used by
	/// JProcessor/JFactory classes to communicate an appropriate
	/// exit code that a jana program can return upon exit. The
	/// value can be set via the SetExitCode method.

	return _exit_code;
}

JComponentSummary JApplication::GetComponentSummary() {
    /// Returns a data object describing all components currently running
    return _component_manager->get_component_summary();
}

// Performance/status monitoring

void JApplication::SetTicker(bool ticker_on) {
    _ticker_on = ticker_on;
}

void JApplication::PrintStatus() {
    if (_extended_report) {
        _processing_controller->print_report();
    }
    else {
        std::lock_guard<std::mutex> lock(_status_mutex);
        update_status();
        LOG_INFO(_logger) << "Status: " << _perf_summary->total_events_completed << " events processed  "
                          << JTypeInfo::to_string_with_si_prefix(_perf_summary->latest_throughput_hz) << "Hz ("
                          << JTypeInfo::to_string_with_si_prefix(_perf_summary->avg_throughput_hz) << "Hz avg)" << LOG_END;
    }
}

void JApplication::PrintFinalReport() {
    _processing_controller->print_final_report();
}

/// Performs a new measurement if the time elapsed since the previous measurement exceeds some threshold
void JApplication::update_status() {
    auto now = std::chrono::high_resolution_clock::now();
    if ((now - _last_measurement) >= _ticker_interval || _perf_summary == nullptr) {
        _perf_summary = _processing_controller->measure_performance();
        _last_measurement = now;
    }
}

/// Returns the number of threads currently being used.
/// Note: This data gets stale. If you need event counts and rates
/// which are more consistent with one another, call GetStatus() instead.
uint64_t JApplication::GetNThreads() {
    std::lock_guard<std::mutex> lock(_status_mutex);
    update_status();
    return _perf_summary->thread_count;
}

/// Returns the number of events processed since Run() was called.
/// Note: This data gets stale. If you need event counts and rates
/// which are more consistent with one another, call GetStatus() instead.
uint64_t JApplication::GetNEventsProcessed() {
    std::lock_guard<std::mutex> lock(_status_mutex);
    update_status();
    return _perf_summary->total_events_completed;
}

/// Returns the total integrated throughput so far in Hz since Run() was called.
/// Note: This data gets stale. If you need event counts and rates
/// which are more consistent with one another, call GetStatus() instead.
float JApplication::GetIntegratedRate() {
    std::lock_guard<std::mutex> lock(_status_mutex);
    update_status();
    return _perf_summary->avg_throughput_hz;
}

/// Returns the 'instantaneous' throughput in Hz since the last perf measurement was made.
/// Note: This data gets stale. If you need event counts and rates
/// which are more consistent with one another, call GetStatus() instead.
float JApplication::GetInstantaneousRate()
{
    std::lock_guard<std::mutex> lock(_status_mutex);
    update_status();
    return _perf_summary->latest_throughput_hz;
}


