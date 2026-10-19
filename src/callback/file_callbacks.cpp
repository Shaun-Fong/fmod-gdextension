#include "file_callbacks.h"

namespace Callbacks {

    GodotFileRunner* GodotFileRunner::get_singleton() {
        static GodotFileRunner singleton;
        return &singleton;
    }

    void GodotFileRunner::queueReadRequest(FMOD_ASYNCREADINFO* request, ReadPriority priority) {
        // High-priority requests have to be processed first.
        if (priority == ReadPriority::HIGH) {
            // lock so we can't add and remove elements from the queue at the same time.
            std::lock_guard<std::mutex> lk(read_mut);
            requests.push_front(request);
        } else {
            // lock so we can't add and remove elements from the queue at the same time.
            std::lock_guard<std::mutex> lk(read_mut);
            requests.push_back(request);
        }
        read_cv.notify_one();
    }

    FMOD_RESULT GodotFileRunner::cancelReadRequest(FMOD_ASYNCREADINFO* request) {
        // lock so we can't add and remove elements from the queue at the same time.
        bool removed {false};
        {
            std::lock_guard<std::mutex> lk(read_mut);
            removed = requests.erase(request);
        }

        // The request is answered outside of the queue lock: the FMOD callback can queue a new
        // read request, which would deadlock on the non recursive mutex.
        if (removed) {
            request->bytesread = 0;
            request->done(request, FMOD_RESULT::FMOD_ERR_FILE_DISKEJECTED);
            return FMOD_RESULT::FMOD_ERR_FILE_DISKEJECTED;
        }

        // We lock and check if the current request is the one being canceled.
        // In this case, we wait until it's done.
        {
            std::unique_lock<std::mutex> lk(cancel_mut);
            cancel_cv.wait(lk, [this, request] { return request != current_request; });
        }

        return FMOD_RESULT::FMOD_OK;
    }

    void GodotFileRunner::run() {
        while (!stop) {
            // waiting for the container to have one request
            {
                std::unique_lock<std::mutex> lk(read_mut);
                read_cv.wait(lk, [this] { return !requests.is_empty() || stop; });
            }

            while (true) {
                FMOD_ASYNCREADINFO* request {nullptr};
                // lock so we can't add and remove elements from the queue at the same time.
                // The current request is published while the read mutex is still held, so a
                // concurrent cancel cannot report the request as done a second time.
                {
                    std::lock_guard<std::mutex> lk(read_mut);
                    if (requests.is_empty()) { break; }
                    request = requests.front()->get();
                    requests.pop_front();

                    std::lock_guard<std::mutex> cancel_lk(cancel_mut);
                    current_request = request;
                }

                // We get the Godot File object from the handle
                GodotFileHandle* handle {reinterpret_cast<GodotFileHandle*>(request->handle)};
                if (handle == nullptr) {
                    request->bytesread = 0;
                    request->done(request, FMOD_RESULT::FMOD_ERR_FILE_NOTFOUND);
                } else {
                    godot::Ref<godot::FileAccess> file {handle->file};

                    // update the position of the cursor
                    file->seek(request->offset);

                    // We read and store the requested data in an array.
                    godot::PackedByteArray buffer {file->get_buffer(request->sizebytes)};
                    int size {static_cast<int>(buffer.size())};
                    const uint8_t* data {buffer.ptr()};

                    // We copy the data to FMOD buffer
                    if (size > 0) { memcpy(request->buffer, data, size * sizeof(uint8_t)); }
                    request->bytesread = size;

                    // Remember to return an error if the end of the file is reached
                    FMOD_RESULT result = (size < request->sizebytes) ? FMOD_RESULT::FMOD_ERR_FILE_EOF : FMOD_RESULT::FMOD_OK;
                    request->done(request, result);
                }

                // Request no longer processed
                {
                    std::lock_guard<std::mutex> lk(cancel_mut);
                    current_request = nullptr;
                }
                cancel_cv.notify_all();
            }
        }

        // Answer the requests that were never processed, otherwise FMOD would wait forever
        // for an asynchronous read that cannot complete anymore.
        while (true) {
            FMOD_ASYNCREADINFO* request {nullptr};
            {
                std::lock_guard<std::mutex> lk(read_mut);
                if (requests.is_empty()) { break; }
                request = requests.front()->get();
                requests.pop_front();
            }
            request->bytesread = 0;
            request->done(request, FMOD_RESULT::FMOD_ERR_FILE_DISKEJECTED);
        }
    }

    void GodotFileRunner::start() {
        stop = false;
        fileThread = std::thread(&GodotFileRunner::run, this);
    }

    void GodotFileRunner::finish() {
        stop = true;
        // we need to notify the loop one last time, otherwise it will stay stuck in the wait method.
        read_cv.notify_one();
        fileThread.join();
    }

    FMOD_RESULT F_CALL godotFileOpen(const char* name, unsigned int* filesize, void** handle, void* userdata) {
        godot::Ref<godot::FileAccess> access = godot::FileAccess::open(name, godot::FileAccess::ModeFlags::READ);

        if (access.is_null() || access->get_error() != godot::Error::OK) {
            return FMOD_RESULT::FMOD_ERR_FILE_NOTFOUND;
        }

        *filesize = access->get_length();
        GodotFileHandle* fileHandle {new GodotFileHandle {access}};
        *handle = reinterpret_cast<void*>(fileHandle);
        return FMOD_RESULT::FMOD_OK;
    }

    FMOD_RESULT F_CALL godotFileClose(void* handle, void* userdata) {
        if (handle == nullptr) { return FMOD_RESULT::FMOD_OK; }
        delete reinterpret_cast<GodotFileHandle*>(handle);
        return FMOD_RESULT::FMOD_OK;
    }

    FMOD_RESULT F_CALL godotSyncRead(FMOD_ASYNCREADINFO* info, void* userdata) {
        GodotFileRunner* runner {GodotFileRunner::get_singleton()};
        int priority {info->priority};

        GodotFileRunner::ReadPriority priorityRank;
        if (priority >= 50) {
            priorityRank = GodotFileRunner::ReadPriority::HIGH;
        } else {
            priorityRank = GodotFileRunner::ReadPriority::NORMAL;
        }
        runner->queueReadRequest(info, priorityRank);
        return FMOD_RESULT::FMOD_OK;
    }

    FMOD_RESULT F_CALL godotSyncCancel(FMOD_ASYNCREADINFO* info, void* userdata) {
        GodotFileRunner* runner {GodotFileRunner::get_singleton()};
        return runner->cancelReadRequest(info);
    }
}// namespace Callbacks