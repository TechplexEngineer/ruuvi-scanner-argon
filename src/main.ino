
#include "Particle.h"
#include <ArduinoJson.h>
#include <unordered_map>

// This example does not require the cloud so you can run it in manual mode or
// normal cloud-connected mode
// SYSTEM_MODE(MANUAL);

const size_t SCAN_RESULT_MAX = 30;

BleScanResult scanResults[SCAN_RESULT_MAX];
int scanResultsSize = 0;
    /*
    * BLE scan parameters:
    *     - BLE_SCAN_TYPE
    *           0x00: Passive scanning, no scan request packets shall be sent.(default)
    *           0x01: Active scanning, scan request packets may be sent.
    *           0x02 - 0xFF: Reserved for future use.
    *     - BLE_SCAN_INTERVAL: This is defined as the time interval from when the Controller started its last LE scan until it begins the subsequent LE scan.
    *           Range: 0x0004 to 0x4000
    *           Default: 0x0010 (10 ms)
    *           Time = N * 0.625 msec
    *           Time Range: 2.5 msec to 10.24 seconds
    *     - BLE_SCAN_WINDOW: The duration of the LE scan. The scan window shall be less than or equal to the scan interval.
    *           Range: 0x0004 to 0x4000
    *           Default: 0x0010 (10 ms)
    *           Time = N * 0.625 msec
    *           Time Range: 2.5 msec to 10240 msec
    */
	const uint8_t BLE_SCAN_TYPE =       		0x00;  // Passive scanning
	const uint16_t BLE_SCAN_INTERVAL =    		0x0A0; // 100 ms
	const uint16_t BLE_SCAN_WINDOW =     		0x0A0; // 100 ms
	const uint16_t BLE_SCAN_TIMEOUT =     		10;    // 100 ms (10 units of 10 ms)

bool isScanning = false;

const uint16_t ESTIMOTE_MFCT_UUID    = 0x015d;
const uint16_t ESTIMOTE_SERVICE_UUID = 0xfe9a;

const uint16_t RUUVI_MFCT_UUID       = 0x0499;
// const uint16_t RUUVI_SERVICE_UUID    = 0x;

const uint16_t APRIL_MFCT_UUID       = 0xfe59;
// const uint16_t APRIL_SERVICE_UUID    = 0x;

void setup() {
    Serial.println("test");
    Particle.publish("setup", PRIVATE);

    Particle.function("doScan", doScan); //bool success = 
    Particle.function("stopScan", stopScan);
    Particle.function("startScan", startScan);

    

    Particle.publish("setup_complete", PRIVATE);

    BleScanParams scanParams;
    scanParams.version = 	BLE_API_VERSION;
    scanParams.size = 		sizeof(BleScanParams);
    scanParams.active = 	false;
    scanParams.interval = 	BLE_SCAN_INTERVAL;
    scanParams.window = 	BLE_SCAN_WINDOW;
    scanParams.timeout = 	BLE_SCAN_TIMEOUT;
    BLE.setScanParameters (&scanParams);

}

// Cloud functions must return int and take a String
int doScan(String extra) {
    int count = BLE.scan(scanResults, SCAN_RESULT_MAX);
    scanResultsSize = count;
    return count;
}

int startScan(String extra) {
    Serial.println("Starting Scan");
    isScanning = true;
    return 0;
}

double convertToGs(int16_t raw)
{
    // 2g, high resolution mode means 1 mg/digit
    double val = raw;
    return val / 1000;
}
// auto parser_map_t = std::unordered_map<uint16_t, IParser::ptr_t>;

void scanResultCallback(const BleScanResult *scanResult, void *context) {
    
    // String name = scanResult->advertisingData.deviceName();
    // if (name.length() > 0) {
    //     Serial.printlnf(" | deviceName: %s", name.c_str());
    // } else {
    //     Serial.println("");
    // }

    // uint8_t service[2];
    // scanResult->advertisingData.get(
    //             BleAdvertisingDataType::SERVICE_UUID_16BIT_COMPLETE,
    //             service,
    //             2);
    // BleUuid serviceUUID(service[0] | (service[1] << 8));   
    // Serial.printf(" | Svc %s", serviceUUID.toString().c_str());
    

    // 

    uint8_t data[BLE_MAX_ADV_DATA_LEN];
    size_t len = scanResult->advertisingData.get(
                BleAdvertisingDataType::MANUFACTURER_SPECIFIC_DATA,
                data,
                BLE_MAX_ADV_DATA_LEN);
    
    if (len == 0) {
        // did not find MANUFACTURER_SPECIFIC_DATA, maybe labeled as SERVICE_DATA
        len = scanResult->advertisingData.get(
                BleAdvertisingDataType::SERVICE_DATA,
                data,
                BLE_MAX_ADV_DATA_LEN);
    }
    if (len == 0) {
        // Serial.println("Unable to find data");
        return;
    }
    uint16_t mfgid = data[0] | (data[1] << 8);
    

    if (mfgid == RUUVI_MFCT_UUID) {
        Serial.printlnf("FOUND A RUUVI");
        Serial.printlnf("  MAC: %s", scanResult->address.toString().c_str());
        Serial.printlnf("  RSSI: %ddBm", scanResult->rssi);
        Serial.printlnf("  MFG %04X", mfgid);
        Serial.printlnf("  HDR %02x", data[2+0]);
        double humidity_rh = data[2+1] * 0.5;
        Serial.printlnf("  HUM %f %%RH", humidity_rh);
        double temp_c = 0;
        temp_c += (int8_t)data[2+2]; //msb is sign
        temp_c += (data[2+3]/100.0);
        Serial.printlnf("  TMP %f C", temp_c);
    
        uint32_t pressure_pa = (data[2+4] << 8) | data[2+5];
        pressure_pa += 50000;
        double pressure_hpa = pressure_pa/100.0; //hPa
        Serial.printlnf("  PRE %f hpa", pressure_hpa );
        Serial.printlnf("  ACX %f", convertToGs((data[2+6] << 8)  | data[2+7]));
        Serial.printlnf("  ACY %f", convertToGs((data[2+8] << 8)  | data[2+9]));
        Serial.printlnf("  ACZ %f", convertToGs((data[2+10] << 8) | data[2+11]));
        Serial.printlnf("  BAT %f", ((data[2+12] << 8) | data[2+13])/1000.0);

        Serial.println("");
    }
}

// Cloud functions must return int and take a String
int stopScan(String extra) {
    Serial.println("Stopping Scan");
    isScanning = false;
    return BLE.stopScanning();
}




uint8_t buf[BLE_MAX_ADV_DATA_LEN];
void loop() {
    // Particle.publish("loop", PRIVATE);

    if (isScanning) {
        BLE.scan(scanResultCallback, NULL);
    }

    // if (scanResultsSize)
    // {
    //     Serial.println("Scan Results");

    //     for (int i=0; i<scanResultsSize; i++) {

    //         Serial.print(scanResults[i].address.toString());
    //         Serial.print(" ");

    //         scanResults[i].advertisingData.get(
    //             BleAdvertisingDataType::SERVICE_UUID_16BIT_COMPLETE,
    //             buf,
    //             BLE_MAX_ADV_DATA_LEN);
    //         // BleUuid()
    //         Serial.print(buf[0], HEX);
    //         Serial.print(" ");

    //         Serial.println();

            
    //     }

    //     scanResultsSize = 0;
    // }
    
    

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


