#pragma once

#include "Particle.h"

#include <vector>


class ControlRequestState {
public:
    String op;
    char *responseBuffer;
    size_t responseBufferSize;
};

class ControlRequestStatus {
public:
    ControlRequestStatus(const char *key, std::function<void(const String &key, JSONBufferWriter &writer)> fn) : key(key), fn(fn) {
    }

    String key;
    std::function<void(const String &key, JSONBufferWriter &writer)> fn;
};


class ControlRequest {
public:
    static ControlRequest &instance();

    void addHandler(std::function<int(ControlRequestState &state, JSONValue &outerObj, JSONBufferWriter &writer)> handler);

    void setup();

    int customHandler(ctrl_request *req);

    void lock() { os_mutex_lock(mutex); };
    bool trylock() { return os_mutex_trylock(mutex)==0; };
    void unlock() { os_mutex_unlock(mutex); };

    static const size_t responseBufferSize = 512;
    static const int RESULT_CONTINUE = 1;

protected:
    void threadFunction();

    bool setupDone = false;
    char responseBuffer[responseBufferSize];
    std::vector<std::function<int(ControlRequestState &state, JSONValue &outerObj, JSONBufferWriter &writer)>> handlers;
    os_mutex_t mutex = 0;

    static ControlRequest *_instance;
};


class ControlRequestRegister {
public:
    ControlRequestRegister(std::function<int(ControlRequestState &state, JSONValue &outerObj, JSONBufferWriter &writer)> handler) {
        ControlRequest::instance().addHandler(handler);
    }
};

#define REGISTER_CONTROL_REQUEST_HANDLER(handler) REGISTER_CONTROL_REQUEST_HANDLER_(_registerControlHandler, __COUNTER__, handler)

#define REGISTER_CONTROL_REQUEST_HANDLER_(name, count, handler) \
    REGISTER_CONTROL_REQUEST_HANDLER__(name, count, handler)

#define REGISTER_CONTROL_REQUEST_HANDLER__(name, id, handler) \
    static ControlRequestRegister name##id(handler);

/**
 * @brief Class for sending a message from the firmware to the other side
 * 
 * These are queued and picked up by the other side by polling since there isn't a way
 * to send control requests from the device.
 */
class ControlRequestMessageQueue {
public:
    static ControlRequestMessageQueue &instance();

    bool put(const char *json);

    bool putObject(std::function<void(JSONWriter &writer)>fn);

    bool take(char *buf, size_t bufSize);


protected:
    ControlRequestMessageQueue();
    virtual ~ControlRequestMessageQueue();

    ControlRequestMessageQueue(const ControlRequestMessageQueue &) = delete;
    ControlRequestMessageQueue &operator=(const ControlRequestMessageQueue &src) = delete;

    static const size_t queueSize = 20;
    os_queue_t queue;
    static ControlRequestMessageQueue *_instance;
};