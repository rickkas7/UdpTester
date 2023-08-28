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
unsigned long startMillis;
int packetsReceived;
int packetsLost;
int bytesReceived;
int packetsSinceLastCheck = 0;


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

        for(int tries = 0; tries < 20; tries++) {
            int size = udp.receivePacket(packetBuf, sizeof(packetBuf));
            if (size >= 0) {
                if (size >= 4) {
                    bytesReceived += size;
                    packetsReceived++;

                    int seq = *(int *)packetBuf;
                    if (seq == nextSeq) {
                        // Log.info("packet seq=%d size=%d", seq, size);
                        packetsSinceLastCheck++;
                    }
                    else {
                        Log.info("packet seq=%d size=%d, expected seq=%d, lost=%d", seq, size, nextSeq, seq - nextSeq);
                        packetsLost += (seq - nextSeq);
                    }
                    nextSeq = seq + 1;
                }
                else {
                    Log.info("packet size=%d", size);
                }
            }
            else {
                break;
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

    if (startMillis) {
        static unsigned long nextReport = 0;

        if (millis() - nextReport >= 10000) {
            nextReport = millis();

            double elapsed = (double)(millis() - startMillis) / 1000.0;
            if (elapsed > 0) {
                double kbytesSec = (double)bytesReceived / 1024.0 / elapsed;

                double lossRate = (double)packetsLost * 100.0 / (double) packetsReceived;

                Log.info("received=%d lost=%d elapsed=%.1lf kbytes/sec=%.1lf lossRate=%.1lf", 
                    packetsReceived, packetsLost, elapsed, kbytesSec, lossRate);            

                ControlRequestMessageQueue::instance().putObject([packetsReceived, packetsLost, elapsed, kbytesSec, lossRate](JSONWriter &writer) {
                    writer.name("packetsReceived").value(packetsReceived);
                    writer.name("packetsLost").value(packetsLost);
                    writer.name("elapsed").value(elapsed);
                    writer.name("transferRate").value(kbytesSec);
                    writer.name("lossRate").value(lossRate);
                });

                if (packetsSinceLastCheck == 0) {
                    Log.info("server stopped sending packets");
                    startMillis = 0;
                }

                packetsSinceLastCheck = 0;
            }
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
        nextSeq = 0;
        startMillis = millis();
        bytesReceived = 0;
        packetsReceived = 0;
        packetsLost = 0;
        result = SYSTEM_ERROR_NONE;
    }

    return result;
});


REGISTER_CONTROL_REQUEST_HANDLER([](ControlRequestState &state, JSONValue &outerObj, JSONBufferWriter &writer) {
    int result = SYSTEM_ERROR_NOT_SUPPORTED;

    if (state.op == "stop") {
        Log.info("stop");
        startMillis = 0;
        result = SYSTEM_ERROR_NONE;
    }

    return result;
});
