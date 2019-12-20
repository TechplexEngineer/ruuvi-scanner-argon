
#include "Particle.h"
#include <unordered_map>
#include <set>
#include <iterator>
#include "Base64RK.h"
#include "JsonParserGeneratorRK.h"


// This example does not require the cloud so you can run it in manual mode or
// normal cloud-connected mode
// SYSTEM_MODE(MANUAL);

const size_t SCAN_RESULT_MAX = 30;

BleScanResult scanResults[SCAN_RESULT_MAX];

Timer timer(1000 * 1, timer_handler);
std::unordered_map<std::string, std::string> advert_map;
std::set<std::string> knownBeacons;
int positionInSet = 0;



int scanResultsSize = 0;
    /*
    * BLE scan parameters:
    *     - BLE_SCAN_TYPE
    *           0x00: Passive scanning, no scan request packets shall be sent.(default)
    *           0x01: Active scanning, scan request packets may be sent.
    *
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

bool isScanning = true;
static bool doPublish = false;

const uint16_t ESTIMOTE_MFCT_UUID    = 0x015d;
const uint16_t ESTIMOTE_SERVICE_UUID = 0xfe9a;

const uint16_t RUUVI_MFCT_UUID       = 0x0499;
// const uint16_t RUUVI_SERVICE_UUID    = 0x;

const uint16_t APRIL_MFCT_UUID       = 0xfe59;
// const uint16_t APRIL_SERVICE_UUID    = 0x;

void timer_handler()
{
    doPublish = true;
}


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
    // int count = BLE.scan(scanResults, SCAN_RESULT_MAX);
    // scanResultsSize = count;
    return BLE_GAP_ADV_SET_DATA_SIZE_MAX;
}

int startScan(String extra) {
    isScanning = true;
    return 0;
}

double convertToGs(int16_t raw)
{
    // 2g, high resolution mode means 1 mg/digit
    double val = raw;
    return val / 1000;
}



void scanResultCallback(const BleScanResult *scanResult, void *context) {

    uint8_t data[BLE_MAX_ADV_DATA_LEN+10];
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

        if (advert_map.size() >= 15) {
            Serial.println("OVERFLOW!");
        } else {
            if( (len +1) < (BLE_MAX_ADV_DATA_LEN+10) ) {
                data[len] = scanResult->rssi;
                len ++;
            }
            String dat = Base64::encodeToString(data, len);

            auto it = advert_map.find(std::string(scanResult->address.toString()));

            if (it != advert_map.end())
            {
                // if key already exists, replace data
                it -> second = std::string(dat, strlen(dat))
            }
            else
            {
                // key does not exist, insert new data
                advert_map.insert({
                    std::string(scanResult->address.toString()),
                    std::string(dat, strlen(dat))
                });
            }
            knownBeacons.insert(std::string(scanResult->address.toString()));
        }
    }
}

// Cloud functions must return int and take a String
int stopScan(String extra) {
    Serial.println("Stopping Scan");
    isScanning = false;
    return BLE.stopScanning();
}


uint8_t buf[BLE_MAX_ADV_DATA_LEN];

void loop()
{

    int count = 0;
    const int MAX_SEND = 3;

    if (isScanning) {
        Serial.println("Starting Scan");
        if (!timer.isActive()) {
            timer.start();
        }
        BLE.scan(scanResultCallback, NULL);
    } else {
        timer.stop();
    }

    if (doPublish) {
        doPublish = false;

        JsonWriterStatic<256> jw;

        {
            JsonWriterAutoObject obj(&jw);
            for (int i = 0; i < MAX_SEND; ++i)
            {
                auto it = std::next(knownBeacons.begin(), positionInSet % knownBeacons.size());
                std::string bid = *it;
                auto data = advert_map[bid];
                jw.insertKeyValue(bid.c_str(), data.c_str());
                positionInSet++;
            }
            // for (const auto beaconid : knownBeacons)

            // for (const auto &pair : advert_map) {
            //     jw.insertKeyValue(pair.first.c_str(), pair.second.c_str());
            //     if (++count >= MAX_ADV_SEND) {
            //         break;
            //     }
            // }
        }


        Serial.printlnf("publish  %d", advert_map.size());
        Serial.printlnf(jw.getBuffer());
        Particle.publish("/putney/main/ruuvi", jw.getBuffer(), PRIVATE);
        // advert_map.clear();
    }
}


