/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#line 1 "/home/techplex/projects/pers/ruuvi-scanner-argon/src/main.ino"

#include "Particle.h"
#include <ArduinoJson.h>

// This example does not require the cloud so you can run it in manual mode or
// normal cloud-connected mode
// SYSTEM_MODE(MANUAL);

void setup();
int doScan(String extra);
void loop();
#line 9 "/home/techplex/projects/pers/ruuvi-scanner-argon/src/main.ino"
const size_t SCAN_RESULT_MAX = 30;

BleScanResult scanResults[SCAN_RESULT_MAX];
int scanResultsSize = 0;

void setup() {
    Serial.println("test");
    Particle.publish("setup", PRIVATE);

    Particle.function("doScan", doScan); //bool success = 

    Particle.publish("setup_complete", PRIVATE);

    // BleScanParams scanParams;
    // BLE.setScanParameters(scanParams);

}

// Cloud functions must return int and take a String
int doScan(String extra) {
    int count = BLE.scan(scanResults, SCAN_RESULT_MAX);
    scanResultsSize = count;
    return count;
}

// void onScanReport(const BleScanResult& report)
// {
//     report.address
// }

const uint16_t ESTIMOTE_MFCT_UUID = 0x015d;
const uint16_t RUUVI_MFCT_UUID    = 0x0499;
const uint16_t APRIL_MFCT_UUID    = 0xfe59;


uint8_t buf[BLE_MAX_ADV_DATA_LEN];
void loop() {
    // Particle.publish("loop", PRIVATE);

    if (scanResultsSize)
    {
        Serial.println("Scan Results");

        for (int i=0; i<scanResultsSize; i++) {

            Serial.print(scanResults[i].address.toString());
            Serial.print(" ");

            scanResults[i].advertisingData.get(
                BleAdvertisingDataType::SERVICE_UUID_16BIT_COMPLETE,
                buf,
                BLE_MAX_ADV_DATA_LEN);
            // BleUuid()
            Serial.print(buf[0], HEX);
            Serial.print(" ");

            Serial.println();

            
        }

        scanResultsSize = 0;
    }
    
    

    // for (int ii = 0; ii < count; ii++) {
    //     uint8_t buf[BLE_MAX_ADV_DATA_LEN];
    //     size_t len;

    //     // We're looking for devices that have a heart rate service (0x180D)
    //     len = scanResults[ii].advertisingData.get(BleAdvertisingDataType::SERVICE_UUID_16BIT_COMPLETE, buf, BLE_MAX_ADV_DATA_LEN);
    //     if (len > 0) {
    //         //
    //         for(size_t jj = 0; jj < len; jj += 2) {
    //             if (*(uint16_t *)&buf[jj] == BLE_SIG_UUID_HEART_RATE_SVC) { // 0x180D
    //                 // Found a device with a heart rate service

    //                 Log.info("rssi=%d address=%02X:%02X:%02X:%02X:%02X:%02X ",
    //                         scanResults[ii].rssi,
    //                         scanResults[ii].address[0], scanResults[ii].address[1], scanResults[ii].address[2],
    //                         scanResults[ii].address[3], scanResults[ii].address[4], scanResults[ii].address[5]);
    //             }
    //         }
    //     }
    // }
}


