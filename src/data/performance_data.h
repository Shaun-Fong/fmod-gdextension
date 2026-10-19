#ifndef GODOTFMOD_PERFORMANCE_DATA_H
#define GODOTFMOD_PERFORMANCE_DATA_H

#include "classes/ref_counted.hpp"

namespace godot {
    class FmodPerformanceData : public RefCounted {
        GDCLASS(FmodPerformanceData, RefCounted);

    public:
        float dsp = 0;
        float geometry = 0;
        float stream = 0;
        float update = 0;
        float convolution1 = 0;
        float convolution2 = 0;
        float studio = 0;

        int currently_allocated = 0;
        int max_allocated = 0;

        // FMOD reports these in bytes as 64 bit values, they can overflow an int on long sessions.
        int64_t sample_bytes_read = 0;
        int64_t stream_bytes_read = 0;
        int64_t other_bytes_read = 0;

        float get_geometry() const;
        float get_stream() const;
        float get_update() const;
        float get_convolution1() const;
        float get_convolution2() const;
        float get_studio() const;
        float get_dsp() const;

        int get_currently_allocated() const;
        int get_max_allocated() const;

        int64_t get_sample_bytes_read() const;
        int64_t get_stream_bytes_read() const;
        int64_t get_other_bytes_read() const;

    protected:
        static void _bind_methods();
    };

}// namespace godot

#endif// GODOTFMOD_PERFORMANCE_DATA_H
