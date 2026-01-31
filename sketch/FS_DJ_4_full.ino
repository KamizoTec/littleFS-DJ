//🌟🌟🌟🌟🌟🌟🌟 KamizoTec Little FS EDITOR V4.0 🌟🌟🌟🌟🌟🌟

#ifdef ESP32
  #include <WiFi.h>                 
  #include <ESPAsyncWebServer.h>    
  #include <AsyncTCP.h>              
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>            
  #include <ESPAsyncWebServer.h>     
  #include <ESPAsyncTCP.h>           
#else
  #error "Unsupported platform!"
#endif

#include <FS.h>
#include <LittleFS.h>

// this is a fallback Editor webpage in case the FS filesystem is corrupt or not installed
#include "webpage.h" //optinal 

//const char* ssid = "your-SSID"; 
//const char* password = "your-PASSWORD";

AsyncWebServer server(80);

// this is the global part from the fileckecker to remove unwanted characters and protect the JSON file
// It is not strictly necessary if you only use permitted characters.

// statistic check filesystem
int filesChecked = 0;
int filesRepaired = 0;
int dirsChecked = 0;
int dirsRepaired = 0;
//allowed characters for files and folders : a/A to z/Z and 0 to 9, - _ / . 
bool validChar(char c) {
    return ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '_' || c == '-' || c == '.' || c == '/');
}
bool validFilename(const String& name) {
    for (size_t i = 0; i < name.length(); i++) {
        if (!validChar(name[i])) return false;
    }
    return true;
}
//-------filechecker global end --------------------------------------

void setup() {
  Serial.begin(115200);

WiFi.setHostname("FS-DJ"); //ESP networkname
WiFi.begin(ssid, password);

int attempts = 0;
const int maxAttempts = 10;

while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
    Serial.print(".");
    delay(1000);
    attempts++;
}

if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
} else {
    Serial.println("\nWiFi connection failed");
}
  
//------------------------------------------------
if (LittleFS.begin()) {
    Serial.println("LittleFS mount sucess !");
//------------------------------------------------
// filechecker setup code to protect ESP JSON from dangeres characters 
    Serial.println("Start file system check...");

    auto cleanName = [&](const String& name) {
        String out;
        out.reserve(name.length());
        for (size_t i = 0; i < name.length(); i++) {
            char c = name[i];
            out += validChar(c) ? c : '_';
        }
        if (out.length() == 0) out = "file";
        return out;
    };

    auto avoidCollision = [&](const String& dir, const String& name) {
        String path = dir + name;
        if (!LittleFS.exists(path)) return path;

        Serial.printf(" name collision: %s already exists\n", path.c_str());

        bool isDir = name.endsWith("/");
        String pureName = isDir ? name.substring(0, name.length() - 1) : name;

        int dot = isDir ? -1 : pureName.lastIndexOf('.');
        String base = (dot > 0) ? pureName.substring(0, dot) : pureName;
        String ext  = (dot > 0) ? pureName.substring(dot) : "";

        int counter = 1;
        while (true) {
            String newName = base + "_" + counter + ext;
            if (isDir) newName += "/";

            String newPath = dir + newName;
            Serial.printf("rename: %s\n", newPath.c_str());

            if (!LittleFS.exists(newPath)) {
                return newPath;
            }
            counter++;
        }
    };
//---------------------------------------
std::function<void(const String&)> scanDir;
scanDir = [&](const String& path) {

// ESP32 version ***********************
#ifdef ESP32
Serial.println("filesystem (ESP32)");
    File dir = LittleFS.open(path);
    if (!dir || !dir.isDirectory()) {
        Serial.printf("WARN: %s is no directory or it could not be opened\n",
                      path.c_str());
        return;
    }

    Serial.printf("scan folders: %s\n", path.c_str());
    dir.rewindDirectory();

    while (true) {
        File file = dir.openNextFile();
        if (!file) break;

        String name = String(file.name());
        bool isDir = file.isDirectory();
        file.close();

        String fullPath;
        if (name.startsWith("/"))
            fullPath = name;
        else
            fullPath = path + name;

        Serial.printf("found: %s (%s)\n",
                      fullPath.c_str(),
                      isDir ? "DIR" : "FILE");
        if (isDir) {
            dirsChecked++;

            // Ordnername bereinigen
            String clean = cleanName(name);
            String newPath = path + clean + "/";

            if (clean != name) {
                dirsRepaired++;

                Serial.printf("fix folder: %s -> %s\n",
                              name.c_str(), clean.c_str());

                newPath = avoidCollision(path, clean + "/");

                if (LittleFS.rename(fullPath, newPath)) {
                    Serial.printf("renamed folder: %s -> %s\n",
                                  fullPath.c_str(), newPath.c_str());
                    fullPath = newPath;
                } else {
                    Serial.printf("Folder rename FAILED: %s -> %s\n",
                                  fullPath.c_str(), newPath.c_str());
                }
            }

            if (!fullPath.endsWith("/"))
                fullPath += "/";
            scanDir(fullPath);   // Rekursion
        }
        else {
            filesChecked++;

            String clean = cleanName(name);

            if (clean != name) {
                filesRepaired++;

                Serial.printf("fix filename: %s -> %s\n",
                              name.c_str(), clean.c_str());

                String newPath = avoidCollision(path, clean);

                if (LittleFS.rename(fullPath, newPath)) {
                    Serial.printf("rename file: %s -> %s\n",
                                  fullPath.c_str(), newPath.c_str());
                } else {
                    Serial.printf("file rename FAILED: %s -> %s\n",
                                  fullPath.c_str(), newPath.c_str());
                }
            }
        }
    }

    dir.close();
//----------------------------------------------------
 // ESP8266 VERSION
#elif defined(ESP8266)
Serial.println("filesystem (ESP8266)");
Dir dir = LittleFS.openDir("/");

while (dir.next()) {
    String fullPath = dir.fileName();   // z.B. /css/style@old.css
    Serial.printf("found: %s\n", fullPath.c_str());

    filesChecked++;

    int slash = fullPath.lastIndexOf('/');
    String path = (slash >= 0) ? fullPath.substring(0, slash + 1) : "/";
    String name = (slash >= 0) ? fullPath.substring(slash + 1) : fullPath;

    String clean = cleanName(name);

    if (clean != name) {
        filesRepaired++;

        String newPath = avoidCollision(path, clean);

        Serial.printf("fix fielname: %s -> %s\n",
                      fullPath.c_str(), newPath.c_str());

        if (!LittleFS.rename(fullPath, newPath)) {
            Serial.printf("rename FAILED: %s\n", fullPath.c_str());
        }
    }
}
#endif

};
    scanDir("/"); //start scan command

Serial.println("=== Check done ===");
Serial.printf("folders checked:   %d\n", dirsChecked);
Serial.printf("foldernames fixed: %d\n", dirsRepaired);
Serial.printf("files checked:  %d\n", filesChecked);
Serial.printf("filenames fixed:%d\n", filesRepaired);
Serial.println("===============================");

//***************Whitelist check END ************************
} else {
    Serial.println("LittleFS mount ERROR!");

}
//***************W filechecker setup END ************************


// ALL Server routes ---------------------------------------------

    server.serveStatic("/", LittleFS, "/"); // universal / route everything (if you want)
//------------------------------------------------------------
/*
// if you want to use only index with no fallback from webpage.h choose this option
// serve your index.html from the little FS data folder

    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        String htmlContent;

        File file = LittleFS.open("/index.html", "r");  // INDEX.HTML from Data Folder
        if (file) {

            htmlContent = file.readString(); 
            file.close();
            Serial.println("HTML-load to RAM sucess!");
        }

        request->send(200, "text/html", htmlContent);
    });
 */   
//----------------------------------------------------------------------------
// if you want to use the fallback system with the editor on webpage.h

   // Route for your Index.html
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        String htmlContent;
        File file = LittleFS.open("/index.html", "r");  // Datei im Lese-Modus öffnen

        if (file) {
            // If the file was opened successfully, read the content
            htmlContent = file.readString();
            file.close();
            Serial.println("HTML file successfully loaded into RAM!");
        } else {

            // If the file was not found, load the fallback content.
            Serial.println("index.html not found! Using fallback HTML.");
            htmlContent = fallback_html; // Setze den Fallback-Inhalt
        }
        // Send the HTML content (either loaded or fallback) as a response.
        request->send(200, "text/html", htmlContent);
    });

  //-----------------------------------------------------------------
// the favicon ICON 
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/favicon.ico", "image/x-icon");
});


//**************** FS EDITOR  / SERVER ROUTES **************************
//************* LOAD / LIST /  EDIT / DELETE /SAVE / FORMAT / *********
//*****************************************************************************

server.on("/edit", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/editor.html", "text/html");
});

//-------------------------------------------------------------

server.on("/load", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!request->hasParam("file")) {
        request->send(400, "text/plain", "file missing");
        return;
    }
    // get names
    String name = request->getParam("file")->value();
    if (!name.startsWith("/")) {
        name = "/" + name;
    }

    if (!LittleFS.exists(name)) {
        request->send(404, "text/plain", "not found");
        return;
    }
    // send file
    request->send(LittleFS, name, "text/plain");
});

//-------------------------------------------------------------

server.on("/list", HTTP_GET, [](AsyncWebServerRequest *request){
    String path = "/";

    if (request->hasParam("dir")) {
        path = request->getParam("dir")->value();
    }
    if (!path.endsWith("/")) {
        path += "/";
    }
    File root = LittleFS.open(path, "r");
    if (!root || !root.isDirectory()) {
        request->send(400, "text/plain", "Not a directory");
        return;
    }
    File file = root.openNextFile();
    String json = "[";

    while (file) {
        if (json != "[") json += ",";

        json += "{\"name\":\"";
        json += path + file.name();
        json += "\",\"isDir\":";
        json += file.isDirectory() ? "true" : "false";

        // NUR FILES bekommen size
        if (!file.isDirectory()) {
            json += ",\"size\":";
            json += file.size();
        }
        json += "}";
        file = root.openNextFile();
    }
    json += "]";
    request->send(200, "application/json", json);
});

//-------------------------------------------------------------------

server.on("/delete", HTTP_POST, [](AsyncWebServerRequest *request){
    if (!request->hasParam("file", true)) {
        request->send(400, "text/plain", "file missing");
        return;
    }

    String name = request->getParam("file", true)->value();
    if (!name.startsWith("/")) name = "/" + name;

    if (LittleFS.exists(name)) {
        LittleFS.remove(name);
        request->send(200, "text/plain", "deleted");
    } else {
        request->send(404, "text/plain", "not found");
    }
});

//--------------------------------------------------------------------

server.on("/save", HTTP_POST,
    [](AsyncWebServerRequest *request) {}, NULL, // upload

    [](AsyncWebServerRequest *request,// normal request
       uint8_t *data,
       size_t len,
       size_t index,
       size_t total)
    {
        // ===== first Chunk =====
        if (index == 0) {
            String header = "";
            size_t pos = 0;

            while (pos < len && data[pos] != '\n') {
                header += (char)data[pos];
                pos++;
            }

            if (!header.startsWith("file=")) {
                request->send(400, "text/plain", "filename missing");
                return;
            }

            String name = header.substring(5);
            name.trim();

            // ---- SOFT SAFETY ----
            if (!validFilename(name)) {
                request->send(400, "text/plain", "bad filename , allowed a/A to z/Z 0 to 9 _-/.");
                return;
            }

            if (!name.startsWith("/")) {
                name = "/" + name;
            }

            Serial.print("Save file: ");
            Serial.println(name);
            // ---- make folder ----
            int slash = name.lastIndexOf('/');
            if (slash > 0) {
                String dir = name.substring(0, slash);
                if (!LittleFS.exists(dir)) {
                    LittleFS.mkdir(dir);   // bewusst SOFT, kein Abbruch
                }
            }

            request->_tempFile = LittleFS.open(name, "w");
            if (!request->_tempFile) {
                request->send(500, "text/plain", "open failed");
                return;
            }
            pos++; 

            if (pos < len) {
                request->_tempFile.write(data + pos, len - pos);
            }
        }
        // ===== second Chunks =====
        else {
            if (request->_tempFile) {
                request->_tempFile.write(data, len);
            }
        }
        // ===== last Chunk =====
        if (index + len == total) {
            if (request->_tempFile) {
                request->_tempFile.close();
                request->send(200, "text/plain", "save OK");
                Serial.println("===== file saved =====");
            }
        }
    }
);

//--------------ALL SYSTEM INFOs  --------------------------------------------

server.on("/systeminfo", HTTP_GET, [](AsyncWebServerRequest *request) {
    String info;

#if defined(ESP8266)
    info += "Device: ESP8266\n\n";

    FSInfo fs;
    LittleFS.info(fs);

    info += "LittleFS:\n";
    info += "  Total: " + String(fs.totalBytes) + " bytes\n";
    info += "  Used:  " + String(fs.usedBytes) + " bytes\n";
    info += "  Free:  " + String(fs.totalBytes - fs.usedBytes) + " bytes\n\n";

#elif defined(ESP32)
    info += "Device: ESP32\n\n";

    // ESP32 LittleFS: FSInfo gibt es nicht, direkt totalBytes() und usedBytes() nutzen
    size_t total = LittleFS.totalBytes();
    size_t used  = LittleFS.usedBytes();

    info += "LittleFS:\n";
    info += "  Total: " + String(total) + " bytes\n";
    info += "  Used:  " + String(used) + " bytes\n";
    info += "  Free:  " + String(total - used) + " bytes\n\n";

#else
    info += "Device: Unknown\n\n";
#endif

    // Memory info
    info += "Memory:\n";
    info += "  Free heap: " + String(ESP.getFreeHeap()) + " bytes\n";
#if defined(ESP8266)
    info += "  Heap fragmentation: " + String(ESP.getHeapFragmentation()) + " %\n\n";
#endif

    // System info
    info += "System:\n";
    info += "  CPU freq: " + String(ESP.getCpuFreqMHz()) + " MHz\n";
    info += "  Core version: " + String(ESP.getCoreVersion()) + "\n";
    info += "  SDK version:\n";
    info += "    " + String(ESP.getSdkVersion()) + "\n";


#if defined(ESP8266)
    info += "  Reset reason: " + ESP.getResetReason() + "\n";
#elif defined(ESP32)
    info += "  Reset reason: ";
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON:   info += "Power on"; break;
        case ESP_RST_SW:        info += "Software reset"; break;
        case ESP_RST_PANIC:     info += "Crash / panic"; break;
        case ESP_RST_INT_WDT:   info += "Interrupt WDT"; break;
        case ESP_RST_TASK_WDT:  info += "Task WDT"; break;
        case ESP_RST_BROWNOUT:  info += "Brownout"; break;
        case ESP_RST_DEEPSLEEP: info += "Deep sleep wake"; break;
        default:                info += "Unknown"; break;
    }
    info += "\n";
#endif

    info += "  Uptime: " + String(millis() / 1000) + " s\n";

    request->send(200, "text/plain", info);
});

//-----------format Filesystem----------------------------------------------

// this can be used to format the little fs filesystem, just type your_ESP_IP/format
// or unhide the button from the Editor.html

server.on("/format", HTTP_GET, [](AsyncWebServerRequest *request){
    LittleFS.format();
    request->send(200, "text/plain", "LittleFS formatted. Rebooting...");
    delay(2000);
    ESP.restart();
});

//-----------------------------------------------------

  server.begin();
  Serial.println("start server");
} 

//--------------------------------------

void loop() {

} 
//--------------------------------------
