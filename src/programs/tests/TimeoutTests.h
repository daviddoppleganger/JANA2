
// Copyright 2021, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_TIMEOUTTESTS_H
#define JANA2_TIMEOUTTESTS_H

#include <JANA/JEventSource.h>
#include <JANA/JEventProcessor.h>

struct SourceWithTimeout : public JEventSource {

    int timeout_on_event_nr = -1;
    int first_event_delay_ms = 0;
    std::atomic_int event_count {0};

    SourceWithTimeout(std::string source_name,
                      JApplication *app,
                      int timeout_on_event_nr=-1,
                      int first_delay_ms=0 )

        : JEventSource(std::move(source_name), app)
        , timeout_on_event_nr(timeout_on_event_nr)
        , first_event_delay_ms(first_delay_ms)
    { }

    static std::string GetDescription() {
        return "ComponentTests Fake Event Source";
    }

    std::string GetType() const override {
        return JTypeInfo::demangle<decltype(*this)>();
    }

    void Open() override {
    }

    void GetEvent(std::shared_ptr<JEvent>) override {
        event_count += 1;
        std::cout << "Processing event # " << event_count << std::endl;
        std::flush(std::cout);

        if (event_count == 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(first_event_delay_ms));
        }

        if (event_count == 100) {
            throw RETURN_STATUS::kNO_MORE_EVENTS;
        }

        if (event_count == timeout_on_event_nr) {
            while (true) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }; // Endless loop
        }
    }
};



struct ProcessorWithTimeout : public JEventProcessor {

    std::atomic_int processed_count {0};
    int timeout_on_event_nr;
    int first_event_delay_ms;

    explicit ProcessorWithTimeout(int timeout_on_event_nr=-1,
                                  int first_event_delay_ms = 0)
        : timeout_on_event_nr(timeout_on_event_nr)
        , first_event_delay_ms(first_event_delay_ms)
    { }

    void Init() override {}

    void Process(const std::shared_ptr<const JEvent>&) override {
        processed_count += 1;
        if (processed_count == 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(first_event_delay_ms));
        }
        if (processed_count == timeout_on_event_nr) {
            while (true) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }; // Endless loop
        }
    }

    void Finish() override {}
};

#endif //JANA2_TIMEOUTTESTS_H
