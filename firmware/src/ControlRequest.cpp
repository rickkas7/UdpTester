#include "ControlRequest.h"

ControlRequest *ControlRequest::_instance;



ControlRequest &ControlRequest::instance() {
    // May be called during global object initialization, beware!
    if (!_instance) {
        _instance = new ControlRequest();
    }
    return *_instance;
}

void ControlRequest::setup() {
    os_mutex_create(&mutex);
    setupDone = true;
}

void ControlRequest::addHandler(std::function<int(ControlRequestState &state, JSONValue &outerObj, JSONBufferWriter &writer)> handler) {
    // May be called during global object initialization, beware!
    handlers.push_back(handler);
}


int ControlRequest::customHandler(ctrl_request *req) {
    int result = SYSTEM_ERROR_NOT_SUPPORTED;

    JSONValue outerObj = JSONValue::parseCopy(req->request_data, req->request_size);

    memset(responseBuffer, 0, sizeof(responseBuffer));

    ControlRequestState state;
    state.responseBuffer = responseBuffer;
    state.responseBufferSize = sizeof(responseBuffer);

    {
        JSONObjectIterator iter(outerObj);
        while(iter.next()) {
            if (iter.name() == "op") {
                state.op = (const char *)iter.value().toString();
                break;
            }
        }
    }

    if (state.op.length() > 0) {
        JSONBufferWriter writer(responseBuffer, sizeof(responseBuffer) - 1);
    
        writer.beginObject();

        for (std::vector<std::function<int(ControlRequestState &state, JSONValue &outerObj, JSONBufferWriter &writer)>>::iterator it = handlers.begin(); it != handlers.end(); it++) {
            result = (*it)(state, outerObj, writer);
            if (result != SYSTEM_ERROR_NOT_SUPPORTED && result != RESULT_CONTINUE) {
                break;
            }
        }

        writer.endObject();
    }

    if (result == RESULT_CONTINUE) {
        result = SYSTEM_ERROR_NONE;
    }

    size_t size = strlen(responseBuffer);
    if (size > 2) {
        if (system_ctrl_alloc_reply_data(req, size, nullptr) == 0) {
            memcpy(req->reply_data, responseBuffer, size);
        } else {
            result = SYSTEM_ERROR_NO_MEMORY;
        }        
    }

    // Caller must call this:
    // system_ctrl_set_result(req, result, nullptr, nullptr, nullptr);

    return result;
}

ControlRequestMessageQueue *ControlRequestMessageQueue::_instance = nullptr;

// [static]
ControlRequestMessageQueue &ControlRequestMessageQueue::instance() {
    if (!_instance) {
        _instance = new ControlRequestMessageQueue();
    }
    return *_instance;
}


ControlRequestMessageQueue::ControlRequestMessageQueue() {
    os_queue_create(&queue, sizeof(char *), queueSize, NULL);
}

ControlRequestMessageQueue::~ControlRequestMessageQueue() {
    // Singleton is never deleted, so code is missing to delete the queue and queue element storage
}

bool ControlRequestMessageQueue::put(const char *json) {
    int res = -1;

    char *entry = strdup(json);
    if (entry) {
        for(int tries = 1; tries <= 4; tries++) {
            res = os_queue_put(queue, &entry, 0, NULL);
            if (res == 0) {
                break;
            }
            char *temp;
            os_queue_take(queue, &temp, 0, NULL);
        }
    }  
    return res == 0;
}

bool ControlRequestMessageQueue::putObject(std::function<void(JSONWriter &writer)>fn) {
    char buf[ControlRequest::responseBufferSize - 1];

    JSONBufferWriter writer(buf, sizeof(buf) - 1);

    writer.beginObject();
    fn(writer);
    writer.endObject();
    writer.buffer()[std::min(writer.bufferSize(), writer.dataSize())] = 0;

    return put(buf);
}


bool ControlRequestMessageQueue::take(char *buf, size_t bufSize) {
    char *json = nullptr;

    if (os_queue_take(queue, &json, 0, NULL) == 0 && json) {
        size_t len = strlen(json);
        if (len <= (bufSize - 1)) {
            strcpy(buf, json);
        }
        return true;
    }
    return false;
}
    

// op=msg Get outstanding messages from the message queue
// Returns a single JSON message. If there are no outstanding messages, returns an empty string
REGISTER_CONTROL_REQUEST_HANDLER([](ControlRequestState &state, JSONValue &outerObj, JSONBufferWriter &writer) {
    int result = SYSTEM_ERROR_NOT_SUPPORTED;

    if (state.op == "msg") {
        // Normally you use writer, but since we need to use pre-formatted JSON, we just replace
        // the entire buffer
        ControlRequestMessageQueue::instance().take(state.responseBuffer, state.responseBufferSize);
        result = SYSTEM_ERROR_NONE;
    }

    return result;
});


// This must be implemented in user code
/*
void ctrl_request_custom_handler(ctrl_request *req) {
    int result = ControlRequest::instance().customHandler(req);

    if (result == SYSTEM_ERROR_NOT_SUPPORTED) {
        result = SYSTEM_ERROR_NONE;
    }
    system_ctrl_set_result(req, result, nullptr, nullptr, nullptr);
}
*/

