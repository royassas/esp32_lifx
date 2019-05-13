
struct LifxPacket {
  uint16_t size; //little endian
  uint16_t protocol; //little endian
  uint32_t reserved1;
  byte bulbAddress[6];
  uint16_t reserved2;
  byte site[6];
  uint16_t reserved3;
  uint64_t timestamp;
  uint16_t packet_type; //little endian
  uint16_t reserved4;
  
  byte data[128];
  int data_size;
};

const unsigned int LifxProtocol_AllBulbsResponse = 21504; // 0x5400
const unsigned int LifxProtocol_AllBulbsRequest  = 13312; // 0x3400
const unsigned int LifxProtocol_BulbCommand      = 5120;  // 0x1400

const unsigned int LifxPacketSize      = 36;
const unsigned int LifxPort            = 56700;  // local port to listen on

const unsigned int LifxBulbLabelLength = 32;

const unsigned int LifxBulbTagsLength = 8;
const unsigned int LifxBulbTagLabelsLength = 32;

const unsigned int LifxBulbLocationIDLength = 16;
const unsigned int LifxBulbLocationLabelLength = 32;
const unsigned int LifxBulbLocationUpdatedAtLength = 8;

const unsigned int LifxBulbGroupIDLength = 16;
const unsigned int LifxBulbGroupLabelLength = 32;
const unsigned int LifxBulbGroupUpdatedAtLength = 8;

// firmware versions, etc
const unsigned int LifxBulbVendor = 1;
const unsigned int LifxBulbProduct = 1;
const unsigned int LifxBulbVersion = 1;
const unsigned int LifxFirmwareVersionMajor = 1;
const unsigned int LifxFirmwareVersionMinor = 5;

const byte SERVICE_UDP = 0x01;
const byte SERVICE_TCP = 0x02;

// packet types
const byte GET_PAN_GATEWAY = 0x02;
const byte PAN_GATEWAY = 0x03;

const byte GET_WIFI_FIRMWARE_STATE = 0x12;
const byte WIFI_FIRMWARE_STATE = 0x13;

const byte GET_POWER_STATE = 0x14;
const byte GET_POWER_STATE2 = 0x74;
const byte SET_POWER_STATE = 0x75;
const byte SET_POWER_STATE2 = 0x15;
const byte POWER_STATE = 0x16;
const byte POWER_STATE2 = 0x76;

const byte GET_BULB_LABEL = 0x17;
const byte SET_BULB_LABEL = 0x18;
const byte BULB_LABEL = 0x19;

const byte GET_VERSION_STATE = 0x20;
const byte VERSION_STATE = 0x21;

const byte GET_BULB_TAGS = 0x1a;
const byte SET_BULB_TAGS = 0x1b;
const byte BULB_TAGS = 0x1c;

const byte GET_BULB_TAG_LABELS = 0x1d;
const byte SET_BULB_TAG_LABELS = 0x1e;
const byte BULB_TAG_LABELS = 0x1f;

const byte DEVICE_GET_LOCATION = 0x30;
const byte DEVICE_SET_LOCATION = 0x31;
const byte DEVICE_STATE_LOCATION = 0x32;

const byte DEVICE_GET_GROUP = 0x33;
const byte DEVICE_SET_GROUP = 0x34;
const byte DEVICE_STATE_GROUP = 0x35;

const byte GET_LIGHT_STATE = 0x65;
const byte SET_LIGHT_STATE = 0x66;
const byte LIGHT_STATUS = 0x6b;

const byte GET_MESH_FIRMWARE_STATE = 0x0e;
const byte MESH_FIRMWARE_STATE = 0x0f;


#define EEPROM_BULB_LABEL_START 0 // 32 bytes long
#define EEPROM_BULB_TAGS_START 32 // 8 bytes long
#define EEPROM_BULB_TAG_LABELS_START 40 // 32 bytes long
#define EEPROM_LOCATION_START 72 // 56 bytes long
#define EEPROM_GROUP_START 128 // 56 bytes long
// future data for EEPROM will start at 184...

#define EEPROM_CONFIG "AL1" // 3 byte identifier for this sketch's EEPROM settings
#define EEPROM_CONFIG_START 253 // store EEPROM_CONFIG at the end of EEPROM

// helpers
#define SPACE " "
