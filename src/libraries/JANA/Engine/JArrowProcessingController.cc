
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/Engine/JArrowProcessingController.h>
#include <JANA/Engine/JArrowPerfSummary.h>
#include <JANA/Utils/JCpuInfo.h>
#include <JANA/JLogger.h>

#include <memory>

using millisecs = std::chrono::duration<double, std::milli>;
using secs = std::chrono::duration<double>;

void JArrowProcessingController::acquire_services(JServiceLocator * sl) {
    auto ls = sl->get<JLoggingService>();
    m_logger = ls->get_logger("JArrowProcessingController");
    m_worker_logger = ls->get_logger("JWorker");
    m_scheduler_logger = ls->get_logger("JScheduler");

    // Obtain timeouts from parameter manager
    auto params = sl->get<JParameterManager>();
    params->SetDefaultParameter("jana:timeout", m_timeout_s, "Max. time (in seconds) system will wait for a thread to update its heartbeat before killing it and launching a new one.");
    params->SetDefaultParameter("jana:warmup_timeout", m_warmup_timeout_s, "Max. time (in seconds) system will wait for the initial events to complete before killing program.");
    // Originally "THREAD_TIMEOUT" and "THREAD_TIMEOUT_FIRST_EVENT"
}

void JArrowProcessingController::initialize() {

    m_scheduler = new JScheduler(m_topology->arrows);
    m_scheduler->logger = m_scheduler_logger;
    LOG_INFO(m_logger) << m_topology->mapping << LOG_END;
}

void JArrowProcessingController::run(size_t nthreads) {

    m_topology->set_active(true);
    scale(nthreads);
}

void JArrowProcessingController::scale(size_t nthreads) {

    bool pin_to_cpu = (m_topology->mapping.get_affinity() != JProcessorMapping::AffinityStrategy::None);
    size_t next_worker_id = m_workers.size();

    while (next_worker_id < nthreads) {

        size_t next_cpu_id = m_topology->mapping.get_cpu_id(next_worker_id);
        size_t next_loc_id = m_topology->mapping.get_loc_id(next_worker_id);

        auto worker = new JWorker(m_scheduler, next_worker_id, next_cpu_id, next_loc_id, pin_to_cpu);
        worker->logger = m_worker_logger;
        m_workers.push_back(worker);
        next_worker_id++;
    }

    for (size_t i=0; i<nthreads; ++i) {
        m_workers.at(i)->start();
    };
    for (size_t i=nthreads; i<next_worker_id; ++i) {
        m_workers.at(i)->request_stop();
    }
    for (size_t i=nthreads; i<next_worker_id; ++i) {
        m_workers.at(i)->wait_for_stop();
    }
    m_topology->metrics.reset();
    m_topology->metrics.start(nthreads);
}

void JArrowProcessingController::request_stop() {
    for (JWorker* worker : m_workers) {
        worker->request_stop();
    }
    // Tell the topology to stop timers and deactivate arrows
    m_topology->set_active(false);
}

void JArrowProcessingController::wait_until_stopped() {
    for (JWorker* worker : m_workers) {
        worker->request_stop();
    }
    for (JWorker* worker : m_workers) {
        worker->wait_for_stop();
    }
}

bool JArrowProcessingController::is_stopped() {
    for (JWorker* worker : m_workers) {
        if (worker->get_runstate() != JWorker::RunState::Stopped) {
            return false;
        }
    }
    // We have determined that all Workers have actually stopped
    return true;
}

bool JArrowProcessingController::is_finished() {
    return !m_topology->is_active();
}

bool JArrowProcessingController::is_timed_out() {

    // Note that this makes its own (redundant) call to measure_internal_performance().
    // Probably want to refactor so that we only make one such call per ticker iteration.
    // Since we are storing our metrics summary anyway, we could call measure_performance()
    // and have print_report(), print_final_report(), is_timed_out(), etc use the cached version
    auto metrics = measure_internal_performance();

    int timeout_s;
    if (metrics->total_uptime_s < m_warmup_timeout_s * m_topology->event_pool_size / metrics->thread_count) {
        // We are at the beginning and not all events have necessarily had a chance to warm up
        timeout_s = m_warmup_timeout_s;
    }
    else if (!m_topology->limit_total_events_in_flight) {
        // New events are constantly emitted, each of which may contain jfactorysets which need to be warmed up
        timeout_s = m_warmup_timeout_s;
    }
    else {
        timeout_s = m_timeout_s;
    }

    // Find all workers whose last heartbeat exceeds timeout
    bool found_timeout = false;
    for (size_t i=0; i<metrics->workers.size(); ++i) {
        if (metrics->workers[i].last_heartbeat_ms > (timeout_s * 1000)) {
            found_timeout = true;
            m_workers[i]->declare_timeout();
            // This assumes the workers and their summaries are ordered the same.
            // Which is true, but I don't like it.
        }
    }
    return found_timeout;
}

JArrowProcessingController::~JArrowProcessingController() {
    request_stop();
    wait_until_stopped();
    for (JWorker* worker : m_workers) {
        delete worker;
    }
    delete m_topology;
    delete m_scheduler;
}

void JArrowProcessingController::print_report() {
    auto metrics = measure_internal_performance();
    LOG_INFO(m_logger) << "Running" << *metrics << LOG_END;
}

void JArrowProcessingController::print_final_report() {
    auto metrics = measure_internal_performance();
    LOG_INFO(m_logger) << "Final Report" << *metrics << LOG_END;
}

std::unique_ptr<const JArrowPerfSummary> JArrowProcessingController::measure_internal_performance() {

    // Measure perf on all Workers first, as this will prompt them to publish
    // any ArrowMetrics they have collected
    m_perf_summary.workers.clear();
    for (JWorker* worker : m_workers) {
        WorkerSummary summary;
        worker->measure_perf(summary);
        m_perf_summary.workers.push_back(summary);
    }

    size_t monotonic_event_count = 0;
    for (JArrow* arrow : m_topology->sinks) {
        monotonic_event_count += arrow->get_metrics().get_total_message_count();
    }

    // Uptime
    m_topology->metrics.split(monotonic_event_count);
    m_topology->metrics.summarize(m_perf_summary);

    double worst_seq_latency = 0;
    double worst_par_latency = 0;

    m_perf_summary.arrows.clear();
    for (JArrow* arrow : m_topology->arrows) {
        JArrowMetrics::Status last_status;
        size_t total_message_count;
        size_t last_message_count;
        size_t total_queue_visits;
        size_t last_queue_visits;
        JArrowMetrics::duration_t total_latency;
        JArrowMetrics::duration_t last_latency;
        JArrowMetrics::duration_t total_queue_latency;
        JArrowMetrics::duration_t last_queue_latency;

        arrow->get_metrics().get(last_status, total_message_count, last_message_count, total_queue_visits,
                                 last_queue_visits, total_latency, last_latency, total_queue_latency, last_queue_latency);

        auto total_latency_ms = millisecs(total_latency).count();
        auto total_queue_latency_ms = millisecs(total_queue_latency).count();

        ArrowSummary summary;
        summary.arrow_type = arrow->get_type();
        summary.is_parallel = arrow->is_parallel();
        summary.is_active = arrow->is_active();
        summary.thread_count = arrow->get_thread_count();
        summary.arrow_name = arrow->get_name();
        summary.chunksize = arrow->get_chunksize();
        summary.messages_pending = arrow->get_pending();
        summary.is_upstream_active = !arrow->is_upstream_finished();
        summary.threshold = arrow->get_threshold();
        summary.status = arrow->get_status();

        summary.total_messages_completed = total_message_count;
        summary.last_messages_completed = last_message_count;
        summary.queue_visit_count = total_queue_visits;

        summary.avg_queue_latency_ms = (total_queue_visits == 0)
                                       ? std::numeric_limits<double>::infinity()
                                       : total_queue_latency_ms / total_queue_visits;

        summary.avg_queue_overhead_frac = total_queue_latency_ms / (total_queue_latency_ms + total_latency_ms);

        summary.avg_latency_ms = (total_message_count == 0)
                               ? std::numeric_limits<double>::infinity()
                               : summary.avg_latency_ms = total_latency_ms/total_message_count;

        summary.last_latency_ms = (last_message_count == 0)
                                ? std::numeric_limits<double>::infinity()
                                : millisecs(last_latency).count()/last_message_count;

        if (arrow->is_parallel()) {
            worst_par_latency = std::max(worst_par_latency, summary.avg_latency_ms);
        } else {
            worst_seq_latency = std::max(worst_seq_latency, summary.avg_latency_ms);
        }
        m_perf_summary.arrows.push_back(summary);
    }

    // bottlenecks
    m_perf_summary.avg_seq_bottleneck_hz = 1e3 / worst_seq_latency;
    m_perf_summary.avg_par_bottleneck_hz = 1e3 * m_perf_summary.thread_count / worst_par_latency;

    auto tighter_bottleneck = std::min(m_perf_summary.avg_seq_bottleneck_hz, m_perf_summary.avg_par_bottleneck_hz);

    m_perf_summary.avg_efficiency_frac = (tighter_bottleneck == 0)
                                      ? std::numeric_limits<double>::infinity()
                                      : m_perf_summary.avg_throughput_hz / tighter_bottleneck;

    return std::unique_ptr<JArrowPerfSummary>(new JArrowPerfSummary(m_perf_summary));
}

std::unique_ptr<const JPerfSummary> JArrowProcessingController::measure_performance() {
    return measure_internal_performance();
}




