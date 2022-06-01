#ifndef LIBPFM_WRAPPER_HPP
#define LIBPFM_WRAPPER_HPP

#include <err.h>
#include <string.h>
#include <unistd.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <perfmon/pfmlib_perf_event.h>

namespace {
    static std::vector<std::pair<std::string, int>> events;
    static size_t maxLength = 0;
}

namespace PFMWrapper {
void initialize() {
    int ret = pfm_initialize();
	if (ret != PFM_SUCCESS) {
		errx(1, "cannot initialize library: %s", pfm_strerror(ret));
    }
}

void cleanup() {
    pfm_terminate();
}

void addEvent(std::string event, int level = PFM_PLM0 | PFM_PLM3) {
    pfm_perf_encode_arg_t encoding;
    perf_event_attr attributes;

    memset(&encoding, 0, sizeof(pfm_perf_encode_arg_t));
    memset(&attributes, 0, sizeof(perf_event_attr));

    encoding.attr = &attributes;
    encoding.size = sizeof(pfm_perf_encode_arg_t);

    int ret = pfm_get_os_event_encoding(event.c_str(), level, PFM_OS_PERF_EVENT, &encoding);
    if (ret != PFM_SUCCESS) {
        errx(1, "cannot get encoding: %s", pfm_strerror(ret));
    }

    attributes.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
    attributes.disabled = 1;

    int descriptor = perf_event_open(&attributes, getpid(), -1, -1, 0);
	if (descriptor < 0) {
		err(1, "cannot create event");
    }

    events.emplace_back(event, descriptor);

    if (event.length() > maxLength) {
        maxLength = event.length();
    }
}

void start() {
    for (const auto& [_, descriptor] : events) {
        if (ioctl(descriptor, PERF_EVENT_IOC_ENABLE, 0)) {
            err(1, "ioctl(enable) failed");
        }
    }
}

void stop() {
    for (const auto& [_, descriptor] : events) {
        if (ioctl(descriptor, PERF_EVENT_IOC_DISABLE, 0)) {
            err(1, "ioctl(disable) failed");
        }
    }
}

void print() {
    std::cerr << "============= PFMWrapper Print =============" << std::endl;

    uint64_t buffer[3];
    for (const auto& [event, descriptor] : events) {
        if (read(descriptor, buffer, sizeof(buffer)) != sizeof(buffer)) {
            err(1, "cannot read results");
        }

        uint64_t count = (uint64_t) ((double) buffer[0] * buffer[1] / buffer[2]);
        std::cerr << std::left << std::setw(maxLength) << std::setfill('.') << event << "... " << count << std::endl;
    }

    std::cerr << "============================================" << std::endl;
}
};

#endif