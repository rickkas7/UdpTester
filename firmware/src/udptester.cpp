#include "Particle.h"

#include "ControlRequest.h"

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler;

bool wifiReady = false;
IPAddress localIP;
const uint16_t localPort = 8200;
UDP udp;

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

