#include "WifiManager.h"
#include <WiFi.h>
#include <LittleFS.h>
// init of Wifi manager and set the file path for the network cred database
WifiManager::WifiManager(const char* path = "/wifi.csv"): _path(path){}

// load the creds into to memory for use
bool WifiManager::load(){
    _creds.clear();

  if (!LittleFS.begin(true)) {
    Serial.println("[WiFiMgr] LittleFS mount failed");
    return false;
  }

  if (!LittleFS.exists(_path)) {
    Serial.printf("[WiFiMgr] Missing file: %s\n", _path);
    return false;
  }

  File f = LittleFS.open(_path,"r");
  if(!f) {
    Serial.printf("[WifiMgr] File faield to ioeb : %s\n",_path);
    return false;
  }
// read database file line by line for comma seperated values of network creds
// This should be encrypted one day
  uint16_t fileLine = 0;
  while(f.available()){
    fileLine ++;
    // Read line by line
    String line = f.readStringUntil('\n');
    line.trim();
    if( line.length() == 0 || line.startsWith("#")) continue;
    // parse creds and add them to a vec call _creds
    WifiCredential credential;
    if(parseLine(line,credential)){
        _creds.push_back(credential);
    }else{
        Serial.printf("[WiFiMgr] Bad line in network credential file: %s\n check line: %d for error", _path, fileLine);
    }
  }
  delete &fileLine;
  f.close();

  Serial.printf("[WiFiMgr] Loaded %u networks\n", (unsigned)_creds.size());
  return !_creds.empty();
}

// Parses each line of csv to WifiCreds Struct
bool WifiManager::parseLine(const String& line, WifiCredential& credential){
    //Should be four columns
    // SSID Password Piority Last Connected
    // See header file for def of struct
    int comma = line.indexOf(',');
    int comma2 = line.indexOf(',',comma +1);
    int comma3 = line.indexOf(',',comma2 +1);
    if (comma < 0 || comma2 < 0 || comma3 < 0) return false;
    // SSID
    credential.ssid = trimCopy(line.substring(0, comma));
    // Password if any
    credential.password = trimCopy(line.substring(comma+1,comma2));
    // Piority level for use in the Connectany func
    credential.priority = trimCopy(line.substring(comma2,comma3)).toInt();
    // connected last hopefully will streamline connection process 
    credential.connectedLast = trimCopy(line.substring(comma3)) == "true";
    return credential.ssid.length() > 0;
}
// helper function to manage string spaces in parseLine
String WifiManager::trimCopy(String s) {
  s.trim();
  return s;
}

// Connection function, takes creds and attemps a login using built in WIFI.h module
bool WifiManager::connectOne(const WifiCredential& credential, uint32_t timeoutMs = 5000 ){
    // Resets Wifi settings in preparation for connection
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.disconnect(true,true);
    delay(100);

    Serial.printf("[WifiMgr] Attempting to connect to the network with the SSID: %s",credential.ssid);
    // Attempts connection
    if(credential.password.length() == 0 ){ 
        WiFi.begin(credential.ssid.c_str());
    }else{
        WiFi.begin(credential.ssid.c_str(),credential.password.c_str());
    }
    // checks status every 250 ms returns if connectec
    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        wl_status_t st = WiFi.status();
        if (st == WL_CONNECTED) {
        Serial.printf("[WiFiMgr] Connected: %s  IP=%s\n",
                    credential.ssid.c_str(), WiFi.localIP().toString().c_str());
        return true;
        }
        delay(250);
    }

  Serial.printf("[WiFiMgr] Timeout on: %s\n", credential.ssid.c_str());
  return false;
}

bool WifiManager::connectAny( uint32_t perNetworkMS = 12000, uint8_t maxRounds = 2 ){
  if(_creds.empty()){
    Serial.println("[WiFiMgr] No networks loaded, cannot connect\n");
    return false;
  }
  if(isConnected()){
    Serial.println("[WiFiMgr] Already connected to a network\n");
    return true;
  }
  if( _cachedNetwork != nullptr || recentSsidIndex() >= 0){
    if(connectOne(*_cachedNetwork)){
      Serial.printf("[WiFiMgr] Successfully connected to cached network: %s\n",_cachedNetwork->ssid);  
      return true;
    }
    Serial.printf("[WiFiMgr]Could not successfully connect to cached network: %s\n",_cachedNetwork->ssid);
  }
  sortPiorityNetworks();
  for(int i = _creds.size()-1; i >= 0; i--){
    if(connectOne(_creds.at(i),2000)) return true;
  }
  Serial.print("Unable to connect to any of the provided Networks launching server for user interaction \n");
  return false;
}

int WifiManager::recentSsidIndex(){
  int i = 0;
  for(WifiCredential cred : _creds){
    if(cred.connectedLast){
      _cachedNetwork = &cred;
      return i;
    }
    i++;
  }
  return -1;
}

void WifiManager::sortPiorityNetworks(){
  for(int i = 0; i < _creds.size()-1;i++){
      for(int j = i+1; j < _creds.size();j++){
        if(_creds.at(i).priority > _creds.at(j).priority){
          WifiCredential tempCred = _creds.at(i);
          _creds.at(i) = _creds.at(j);
          _creds.at(j) = tempCred;
        }
    }
  }
  recentSsidIndex();
}

// Not Properly impl yet
// updates cred list on successful connection 
bool WifiManager::save() {
  File f = LittleFS.open(_path, "w");
  if (!f) return false;

  f.println("# ssid,password,priority,connectedLast");
  for (const auto& c : _creds) {
    f.printf("%s,%s,%d,%d\n",
      c.ssid.c_str(),
      c.password.c_str(),
      c.priority,
      c.connectedLast ? 1 : 0
    );
  }
  f.close();
  return true;
}