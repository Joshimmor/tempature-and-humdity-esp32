#pragma once
#include <Arduino.h>
#include <vector>

struct WifiCredential{
    String ssid;
    String password;
    int priority;
    bool connectedLast;
};

class WifiManager {
    public:
        explicit WifiManager( const char* path = "/wifi.csv");
        bool load();
        bool connectAny(u_int32_t perNetworkMS = 12000 , uint8_t maxRounds =2);
        bool isConnected() const;
        IPAddress ip() const;
    private:
        const char* _path;
        WifiCredential* _cachedNetwork;
        void sortPiorityNetworks();
        std::vector<WifiCredential> _creds;
        static bool parseLine(const String& line, WifiCredential& out);
        static String trimCopy( String s);
        bool connectOne(const WifiCredential& cred, uint32_t timeout);
        int recentSsidIndex();
        bool save();


};