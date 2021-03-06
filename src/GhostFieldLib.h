/*************************************************************
 * 
 * TODO: accord VALUES type to Arduino Data Types
 * TODO: write/read serial-uid from eeprom
 * TODO: add SERIAL msg receive callback
 * TODO: add PJON msg receive callback
 * TODO: add waiting phase, wait handcheck with PJON MASTER
 * 
 * ***********************************************************/
#include <Arduino.h>
/*************************************
  * JSON
  * 
  * Open the Arduino Library Manager
  * Search for “ArduinoJson”
  * https://github.com/bblanchon/ArduinoJson.git
  * 
 *************************************/
#include <ArduinoJson.h>
/*************************************
  * PJON
  * 
  * Open the Arduino Library Manager
  * Search for “PJON”
  * https://github.com/gioblu/PJON.git
  * 
 *************************************/
// Include using mac adress
#define PJON_INCLUDE_MAC
// Include only SoftwareBitBang strategy
#include <PJONSoftwareBitBang.h>
/**
 **  CONFIG *******
**/
#define GHOSTFIELD_COM_PIN A0
#define GHOSTFIELD_DEVICE_MASTER 1
#define GHOST_FIELD_SERIAL_BINARY 0
/**
 **  LIB VERSION *******
**/
#define GHOSTFIELD_LIB_VERSION "0.0.5"
/**
 **  SERIAL SPEED *******
**/
#define GHOSTFIELD_SERIAL_SPEED 115200
/**
 **  MAC PREFIX *******
**/
#define GHOSTFIELD_MAC_PREFIX 0x22
/**
 **  MAC SUFIX *******
**/
#define GHOSTFIELD_MAC_SUFIX 0x00
/**
 **  DEVICE TYPES *******
**/
#define GHOSTFIELD_DEVICE_CONTROLLER 0x01
#define GHOSTFIELD_DEVICE_UPS_SWITCH_DRIVER 0x02
#define GHOSTFIELD_DEVICE_PC_SWITCH_DRIVER 0x03
#define GHOSTFIELD_DEVICE_TV_IR_DRIVER 0x04
#define GHOSTFIELD_DEVICE_PROJECTOR_SERIAL_DRIVER 0x05
#define GHOSTFIELD_DEVICE_RFID_DRIVER 0x06
#define GHOSTFIELD_DEVICE_RFID_TAG_DRIVER 0x07
#define GHOSTFIELD_DEVICE_RFID_WEIGAND_DRIVER 0x08
#define GHOSTFIELD_DEVICE_RFID_WEIGAND_LAN_DRIVER 0x09
#define GHOSTFIELD_DEVICE_PROXIMITY_DRIVER 0x0A
/**
 **  CHILD TYPES *******
**/
#define GHOSTFIELD_CHILD_ERROR 0x00
#define GHOSTFIELD_CHILD_UID 0x01
#define GHOSTFIELD_CHILD_TIMER 0x02
#define GHOSTFIELD_CHILD_UP_TIME 0x03
#define GHOSTFIELD_CHILD_OFF_TIME 0x04
#define GHOSTFIELD_CHILD_BINARY_IN 0x05
#define GHOSTFIELD_CHILD_BINARY_OUT 0x06
#define GHOSTFIELD_CHILD_BUZZER 0x07
#define GHOSTFIELD_CHILD_RFID_READER 0x08
#define GHOSTFIELD_CHILD_RFID_TAG_SENSOR 0x09
#define GHOSTFIELD_CHILD_RADAR 0x0A
#define GHOSTFIELD_CHILD_IR_EMITER 0x0B
#define GHOSTFIELD_CHILD_SERIAL 0x0C
#define GHOSTFIELD_CHILD_LAN 0x0D
#define GHOSTFIELD_CHILD_WEBSERVICE 0x0E
/**
 **  COMMAND TYPES *******
**/
#define GHOSTFIELD_CMD_PRESENTATION 0x00    // Sent by a device when they present attached childrens.
#define GHOSTFIELD_CMD_SET          0x01    // This message is sent from or to a child when a child value should be updated
#define GHOSTFIELD_CMD_REQ          0x02    // Requests a variable value (usually from an device destined for controller).
/**
 **  PAYLOAD TYPES *******
**/
#define GHOSTFIELD_VALUE_ERROR	            0x00	// {childType:Hex8; errorCode:Hex8}
#define GHOSTFIELD_VALUE_UID	            0x01	// Hex24
#define GHOSTFIELD_VALUE_TIME	            0x02	// Hex64
#define GHOSTFIELD_VALUE_BINARY	            0x03	// bool (Hex8: 0x00 or 0x01)
#define GHOSTFIELD_VALUE_RFID_UID	        0x04	// Hex32
#define GHOSTFIELD_VALUE_RFID_TAG	        0x05	// {present:bool;uid:V_RFID_UID}
#define GHOSTFIELD_VALUE_DISTANCE	        0x06	// Hex16
#define GHOSTFIELD_VALUE_IRCODE	            0x07	// Hex64
#define GHOSTFIELD_VALUE_SERIALCODE	        0x08	// Hex64
#define GHOSTFIELD_VALUE_LAN_CONFIG	        0x09	// {}
#define GHOSTFIELD_VALUE_WEBSERVICE_CONFIG	0x0A	// {}
/**
 **  GhostFieldDevice CLASS *******


|   prefix     |  device type    |	    serial #       |	sufix    |
|     22       |      00         |	    000001         |	  00     |
|   byte[0]    |    byte[1]      |	 byte[2]-...-[4]   |    byte[5]  |

**/
class GhostFieldDevice
{
private:
public:
    void begin();
    uint8_t serial[3];
    uint8_t device_type;
    uint8_t mac[6];
};
void GhostFieldDevice::begin()
{
    this->mac[0] = GHOSTFIELD_MAC_PREFIX;
    this->mac[1] = this->device_type;
    this->mac[2] = 0x00;
    this->mac[3] = 0x00;
    this->mac[4] = 0x00;
    this->mac[5] = 0x00;
    // ********
    //Serial.print("begin device with type: ");
    //Serial.println(this->device_type);
    // ********
};
/**
 **  GhostFieldDeviceChild CLASS *******
**/
class GhostFieldDeviceChild
{
private:
public:
    GhostFieldDeviceChild ();
    GhostFieldDeviceChild (uint8_t _type, uint8_t _valueType );
    uint8_t valueType;
    uint8_t type;
    uint8_t id;
};        
GhostFieldDeviceChild::GhostFieldDeviceChild (){};
GhostFieldDeviceChild::GhostFieldDeviceChild (uint8_t _type, uint8_t _valueType )
{
    this->type = _type;
    this->valueType = _valueType;
};
/**
 **  GhostFieldMsg CLASS *******


|    Device mac      |	child #	 |   CMD   |  Type	 |     Payload
| 22:00:00:00:01:00	 |      01	 |   01	   |   06	 |       36.5
|  byte[0]-...-[5]	 | byte[6]	 | byte[7] | byte[8] |	byte[9]-...-[22]

**/
class GhostFieldMsg
{
private:
    /* data */
public:
    uint8_t child_id;
    uint8_t cmd;
    uint8_t type;
};
/**
 **  Main *******
**/
GhostFieldDevice gDevice;
GhostFieldDeviceChild gChildUid(GHOSTFIELD_CHILD_UID, GHOSTFIELD_VALUE_UID);
GhostFieldDeviceChild gChildError( GHOSTFIELD_CHILD_ERROR, GHOSTFIELD_VALUE_ERROR);
//
int ittChild = 0;
bool serialPresent = false;
bool pjonConnected = false;
/**
 **  PJON *******
**/
// *******
PJONSoftwareBitBang bus;
// *******
GhostFieldDeviceChild ghostFieldDeviceChildAdd(GhostFieldDeviceChild gChild)
{
    // increment child id
    gChild.id = ittChild;
    ittChild++;
    // ********
    //Serial.print("child added id: ");
    //Serial.print(gChild.id);
    //Serial.print(" type: ");
    //Serial.println(gChild.type);
    // ********
    return gChild;
};
String ghostFieldByteToString(byte itemByte){
    String _tmpItem = "";
    _tmpItem += String(itemByte < 0x10 ? "0" : "");
    _tmpItem += String(itemByte, HEX);
    return _tmpItem;
}
void ghostFieldDumpByteArray(byte *buffer, byte bufferSize)
{
    /**
     * Helper routine to dump a byte array as hex values to Serial.
     */
    for (byte i = 0; i < bufferSize; i++)
    {
        Serial.print(buffer[i] < 0x10 ? "0" : "");
        if(i == bufferSize -1) {
            Serial.println(buffer[i], HEX);
        } else {
            Serial.print(buffer[i], HEX);
            Serial.print(":");
        }
    }
    //Serial.println();
}
void ghostFieldMsgSendOverSerial(byte *buffer, byte bufferSize)
{
#if  !GHOST_FIELD_SERIAL_BINARY 
    // creat a document
    StaticJsonDocument<200> doc;
    // create an object
    //JsonObject root = doc.to<JsonObject>();
    JsonArray mac = doc.createNestedArray("mac");
    JsonArray payload = doc.createNestedArray("payload");
#endif
    // ********
    //Serial.print("bufferSize: ");
    //Serial.println(bufferSize);
    // ********
    byte _tmpSize = bufferSize + 6 ;
    byte _tmp[_tmpSize];    
    for (byte i = 0; i < _tmpSize; i++)
    {        
        if (i > 5)
        {
            _tmp[i] = buffer[i - 6];
            if(i == 6)
                doc["child"] = ghostFieldByteToString(_tmp[i]);
            else if( i == 7)
                doc["cmd"] = ghostFieldByteToString(_tmp[i]);
            else if( i == 8)
                doc["type"] = ghostFieldByteToString(_tmp[i]);
            else
                payload.add(ghostFieldByteToString(_tmp[i]));
            
            
        }
        else if (i < 6)
        {
            _tmp[i] = gDevice.mac[i];
            mac.add(ghostFieldByteToString(_tmp[i]));
        }        
    }
#if  GHOST_FIELD_SERIAL_BINARY    
    Serial.write(_tmp, _tmpSize);
#else
    // ********
    //ghostFieldDumpByteArray(_tmp, _tmpSize);
    // ********        
    // serialize a document
    serializeJsonPretty(doc, Serial);
    //serializeJson(doc, Serial);
    Serial.println("");
#endif    
}
void ghostFieldMsgSendOverPjon(uint8_t deviceID, byte *buffer, byte bufferSize)
{
    unsigned int response = bus.send_packet_blocking(deviceID, buffer, bufferSize);
    return;
    if (response == PJON_ACK)
        Serial.println("PJON send ok");
    if (response == PJON_BUSY)
        Serial.println("PJON is busy");
    if (response == PJON_FAIL)
        Serial.println("PJON send fail");
}
void ghostFieldDeviceChildSetValue(GhostFieldDeviceChild gChild, byte *buffer, byte bufferSize)
{
    // e.g: gChildRfid = ghostFieldDeviceChildAdd(gChildRfid, GHOSTFIELD_CHILD_RFID, GHOSTFIELD_V_TAG);
    // ********
    //Serial.print("set for deviceChild id: ");
    //Serial.println(gChild.id);
    //Serial.print("bufferSize: ");
    //Serial.println(bufferSize);
    // ********
    byte _tmpSize = bufferSize + 3 ;
    byte _tmp[_tmpSize];
    for (byte i = 0; i < _tmpSize; i++)
    {
        if (i > 2)
        {
            _tmp[i] = buffer[i - 3];
        }
        else if (i == 0)
        {
            _tmp[i] = gChild.id;
        }
        else if (i == 1)
        {
            _tmp[i] = GHOSTFIELD_CMD_SET;
        }
        else if (i == 2)
        {
            _tmp[i] = gChild.valueType;
        }
    }
    // ********
    //dump_byte_array(_tmp, _tmpSize);
    // ********
    if (serialPresent)
    {
        ghostFieldMsgSendOverSerial(_tmp, _tmpSize);
    }

    if (pjonConnected)
    {
        // TODO: add waiting phase, wait handcheck with PJON MASTER
        ghostFieldMsgSendOverPjon(GHOSTFIELD_DEVICE_MASTER, _tmp, _tmpSize);
    }
}
void ghostFieldDeviceSetup(uint8_t device_type)
{
    // ********
    Serial.begin(GHOSTFIELD_SERIAL_SPEED);
    serialPresent = Serial;
    // ********
    //Serial.println("ghostFieldDevice setup ... ");
    //Serial.print("serial speed: ");
    //Serial.println(GHOSTFIELD_SERIAL_SPEED);
    // ********
    gDevice.device_type = device_type;
    gDevice.begin();
    // ********
    gChildError = ghostFieldDeviceChildAdd(gChildError);
    gChildUid = ghostFieldDeviceChildAdd(gChildUid);
    // ********
    bus.include_mac(true); // Include MAC address
    bus.set_mac(gDevice.mac);
    bus.strategy.set_pin(GHOSTFIELD_COM_PIN);
    bus.begin();
    // ********
};
void ghostFieldDeviceLoop()
{
    bus.update();
};