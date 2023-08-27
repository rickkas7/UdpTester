#include "Particle.h"

#include "ControlRequest.h"

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler;

bool wifiReady = false;
IPAddress localIP;
const uint16_t localPort = 8200;
UDP udp;
uint8_t packetBuf[10000];
int nextSeq = 0;

void setup() {

}

void loop() {
    if (WiFi.ready()) {
        if (!wifiReady) {
            wifiReady = true;

            localIP = WiFi.localIP();
            Log.info("wifi ready %s:%d", localIP.toString().c_str(), localPort);

            ControlRequestMessageQueue::instance().putObject([](JSONWriter &writer) {
                writer.name("wifi").value(true);
                writer.name("ip").value(localIP.toString().c_str());
                writer.name("port").value(localPort);
            });

            udp.begin(localPort);
        }

        int size = udp.receivePacket(packetBuf, sizeof(packetBuf));
        if (size >= 0) {
            if (size >= 4) {
                int seq = *(int *)packetBuf;
                if (seq == nextSeq) {
                    Log.info("packet seq=%d size=%d", seq, size);
                }
                else {
                    Log.info("packet seq=%d size=%d, expected seq=%d, delta=%d", seq, size, nextSeq, seq - nextSeq);
                }
                nextSeq = seq + 1;
            }
            else {
                Log.info("packet size=%d", size);
            }
        }
    }
    else {
        if (wifiReady) {
            wifiReady = false;

            ControlRequestMessageQueue::instance().putObject([](JSONWriter &writer) {
                writer.name("wifi").value(false);
            });
        }
    }


}

void ctrl_request_custom_handler(ctrl_request *req) {

    int result = ControlRequest::instance().customHandler(req);

    if (result == SYSTEM_ERROR_NOT_SUPPORTED) {
        result = SYSTEM_ERROR_NONE;
    }
    system_ctrl_set_result(req, result, nullptr, nullptr, nullptr);
}


REGISTER_CONTROL_REQUEST_HANDLER([](ControlRequestState &state, JSONValue &outerObj, JSONBufferWriter &writer) {
    int result = SYSTEM_ERROR_NOT_SUPPORTED;

    if (state.op == "info") {
        Log.info("info");
        writer.name("wifi").value(WiFi.ready());
        if (localIP) {
            writer.name("ip").value(localIP.toString().c_str());
            writer.name("port").value(localPort);
        }
        result = SYSTEM_ERROR_NONE;
    }

    return result;
});


REGISTER_CONTROL_REQUEST_HANDLER([](ControlRequestState &state, JSONValue &outerObj, JSONBufferWriter &writer) {
    int result = SYSTEM_ERROR_NOT_SUPPORTED;

    if (state.op == "start") {
        Log.info("start");
        result = SYSTEM_ERROR_NONE;
    }

    return result;
});


REGISTER_CONTROL_REQUEST_HANDLER([](ControlRequestState &state, JSONValue &outerObj, JSONBufferWriter &writer) {
    int result = SYSTEM_ERROR_NOT_SUPPORTED;

    if (state.op == "stop") {
        Log.info("stop");
        result = SYSTEM_ERROR_NONE;
    }

    return result;
});
