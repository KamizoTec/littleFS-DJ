# littleFS-DJ
🌟 FS-DJ v4.0 – LittleFS Web Editor for ESP8266 & ESP32 🌟

**FS-DJ** is a lightweight (~15 KB) browser-based LittleFS file editor for **ESP8266** and **ESP32** wich allows you to explore the FS and change files, create folders etc

Edit `index.html` or any other LittleFS file **live directly on the device** using a clean web interface —  
**no external tools required**.

🌍 Works on:  
✔ Works on ESP8266 & ESP32  
✔ Compatible with **old browsers**, including **Android 4.x browsers**



<p align="center">
  <img src="screenshots/editor.jpg" alt="FS-DJ Web Editor" width="900">
</p>



******************** Features *************************

📁 Browse and create LittleFS files and directories
📝 Load, edit, save, save-as files directly in the browser
♻️ Backup files with one click
🗑️ Delete files
⚡ Fast AsyncWebServer (non-blocking)
📊 Live System Info (ESP8266 / ESP32 auto-detection)
💾 LittleFS usage info (total / used / free)
⚠️ on ESP start the file-checker removes invalid filenames to protect the JSON structure


📦 Requirements -------------------------------
Arduino Libraries :

ESPAsyncWebServer
ESPAsyncTCP (ESP8266)
AsyncTCP (ESP32)
LittleFS

⚠️ Make sure LittleFS is enabled for your board. 
ARDUINO IDE : CRT/Shift/P than Upload little FS to 

📂 Filesystem Structure /data folder ------------------

editor.html / required

webpage.h / optinal/  fallback Editor page if little FS have problems or is corrupt

/favicon.ico / optional


🚀 Installation -------------------------------------

Open the sketch in Arduino IDE

Select your board ESP8266 / ESP32 

Upload the sketch

create the little FS Filesystem

Upload LittleFS data (editor.html,index.html, favicon.ico)

Open the device IP address in your browser

📝 Creating a New File ----------------------------------

LittleFS does not support creating empty files directly via the UI.
Use this workflow:

Load any existing file

Clear the editor content

put your new content

Click Save As 

Enter a new filename

press OK

Done ✅

📁 Creating a New Folder -----------------------------------

FS-DJ automatically creates directories when saving files

Use this workflow:

Load any existing file or Clear the editor content

put your new content

Click Save As /folder/file

Enter a new folder and filename

press OK

Done ✅

📊 System Info Panel ----------------------------------

Displays:

Device type (ESP8266 / ESP32)

LittleFS total / used / free space

Free heap memory

Heap fragmentation (ESP8266 only)

CPU frequency

SDK version

Core version

Reset reason (platform-specific)

Live uptime (no reload needed)

SDK version 


🧪 Known ESP8266 havior

ESP8266 does not display empty directories (LittleFS behavior)

ESP32 requires explicit directory creation


📜 License ------------------------------------------

MIT License / Use it, fork it, break it, improve it 🚀

 Credits

Created & tuned by KamizoTec 2026
With real-world ESP8266 / ESP32 testing and zero bloat.
