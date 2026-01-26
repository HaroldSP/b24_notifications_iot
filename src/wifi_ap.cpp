// WiFi Access Point and Web Server implementation

#include "wifi_ap.h"
#include "storage.h"
#include "wifi_telegram.h"
#include "bitrix24.h"
#include "display_graphics.h"
#include "pomodoro_globals.h"
#include <WiFi.h>
#include <WebServer.h>

// AP credentials
const char* AP_SSID = "ESP32-C6-Config";
const char* AP_PASSWORD = "config12345";  // Simple password for AP

// Web server instance
WebServer server(80);

// AP state
static bool apActive = false;

// HTML page with 3 sections
const char* htmlPage = R"(
<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Настройка устройства</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: Arial, sans-serif;
            background: #f5f5f5;
            padding: 20px;
            max-width: 600px;
            margin: 0 auto;
        }
        h1 { color: #333; margin-bottom: 30px; text-align: center; }
        .section {
            background: white;
            border-radius: 8px;
            padding: 20px;
            margin-bottom: 20px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        .section h2 {
            color: #444;
            margin-bottom: 15px;
            font-size: 18px;
        }
        .instruction {
            background: #e8f4f8;
            padding: 12px;
            border-radius: 4px;
            margin-bottom: 15px;
            font-size: 13px;
            line-height: 1.5;
            color: #555;
        }
        .form-group {
            margin-bottom: 15px;
        }
        label {
            display: block;
            margin-bottom: 5px;
            color: #666;
            font-size: 14px;
        }
        input[type="text"],
        input[type="password"] {
            width: 100%;
            padding: 10px;
            border: 1px solid #ddd;
            border-radius: 4px;
            font-size: 14px;
        }
        button {
            background: #4CAF50;
            color: white;
            padding: 12px 24px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 14px;
            width: 100%;
            margin-top: 10px;
        }
        button:hover {
            background: #45a049;
        }
        .status {
            margin-top: 10px;
            padding: 10px;
            border-radius: 4px;
            text-align: center;
            font-size: 13px;
        }
        .status.success {
            background: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }
        .status.error {
            background: #f8d7da;
            color: #721c24;
            border: 1px solid #f5c6cb;
        }
        .status.info {
            background: #d1ecf1;
            color: #0c5460;
            border: 1px solid #bee5eb;
        }
    </style>
</head>
<body>
    <h1>⚙️ Настройка устройства</h1>
    
    <!-- Section 1: WiFi -->
    <div class="section">
        <h2>1. Настройка WiFi</h2>
        <form action="/wifi" method="POST">
            <div class="form-group">
                <label for="ssid">SSID (имя сети):</label>
                <input type="text" id="ssid" name="ssid" required>
            </div>
            <div class="form-group">
                <label for="password">Пароль:</label>
                <input type="password" id="password" name="password" required>
            </div>
            <button type="submit">Сохранить и подключиться</button>
            <div id="wifi-status" class="status" style="display:none;"></div>
        </form>
    </div>
    
    <!-- Section 2: Telegram -->
    <div class="section">
        <h2>2. Настройка Telegram бота</h2>
        <div class="instruction">
            <strong>Инструкция по созданию бота:</strong><br>
            1. Откройте Telegram и найдите бота @BotFather<br>
            2. В боковом меню нажмите "Create a New Bot" (или отправьте команду /newbot)<br>
            3. Введите имя бота (например: "My Bot") и нажмите "Create Bot"<br>
            4. Введите username бота (должен заканчиваться на "bot", например: my_bot)<br>
            5. После создания бота, BotFather покажет токен - нажмите кнопку "Copy"<br>
            6. Найдите вашего бота по username (например: @my_bot) и откройте его профиль<br>
            7. Нажмите кнопку "Message" для начала диалога<br>
            8. В открывшемся чате нажмите кнопку "START"
        </div>
        <form action="/telegram" method="POST">
            <div class="form-group">
                <label for="botToken">Токен бота:</label>
                <input type="text" id="botToken" name="botToken" required>
            </div>
            <div class="form-group">
                <div class="instruction" style="margin-bottom: 10px;">
                    <strong>Как получить Chat ID:</strong><br>
                    9. Найдите в Telegram бота @chatidbot (или любой другой бот для получения Chat ID)<br>
                    10. Откройте чат с ботом и отправьте команду /start<br>
                    11. Бот пришлёт вам ваш Chat ID (число, например: 22900036) - скопируйте его
                </div>
                <label for="chatId">Chat ID:</label>
                <input type="text" id="chatId" name="chatId" required>
            </div>
            <button type="submit">Сохранить</button>
            <div id="telegram-status" class="status" style="display:none;"></div>
        </form>
    </div>
    
    <!-- Section 3: Bitrix24 -->
    <div class="section">
        <h2>3. Настройка Bitrix24</h2>
        <div class="instruction">
            <strong>Инструкция:</strong><br>
            1. Войдите в ваш Bitrix24 портал<br>
            2. Перейдите в Настройки → Карта сайта → Разработчикам → Другое → Входящий вебхук<br>
            3. Настройка прав: добавьте необходимые права (можете добавить все права)<br>
            4. Скопируйте Вебхук для вызова rest api (например: https://yourcompany.bitrix24.ru/rest/123/abcdefghijklmnop/)<br>
            5. Нажмите кнопку "Сохранить"<br>
            6. Скопируйте URL вашего портала (например: https://yourcompany.bitrix24.ru)<br>
            7. Вставьте эти данные в форму ниже<br>
            <strong style="color: #d32f2f;">⚠️ НИГДЕ НЕ ДЕЛИТЕСЬ ЭТИМ КЛЮЧОМ!</strong>
        </div>
        <form action="/bitrix24" method="POST">
            <div class="form-group">
                <label for="hostname">URL портала Bitrix24:</label>
                <input type="text" id="hostname" name="hostname" placeholder="https://yourcompany.bitrix24.ru" required>
            </div>
            <div class="form-group">
                <label for="restEndpoint">REST Endpoint:</label>
                <input type="text" id="restEndpoint" name="restEndpoint" placeholder="/rest/123/abcdefghijklmnop/" required>
            </div>
            <button type="submit">Сохранить</button>
            <div id="bitrix24-status" class="status" style="display:none;"></div>
        </form>
    </div>
    
    <script>
        // Handle WiFi form submission
        document.querySelector('form[action="/wifi"]').addEventListener('submit', function(e) {
            e.preventDefault();
            const formData = new FormData(this);
            const statusDiv = document.getElementById('wifi-status');
            statusDiv.style.display = 'block';
            statusDiv.className = 'status info';
            statusDiv.textContent = 'Подключение...';
            
            fetch('/wifi', {
                method: 'POST',
                body: formData
            })
            .then(response => response.text())
            .then(data => {
                if (data.includes('success')) {
                    statusDiv.className = 'status success';
                    statusDiv.textContent = '✓ Успешно сохранено и подключено к WiFi!';
                } else {
                    statusDiv.className = 'status error';
                    statusDiv.textContent = '✗ Ошибка: ' + data;
                }
            })
            .catch(error => {
                statusDiv.className = 'status error';
                statusDiv.textContent = '✗ Ошибка подключения';
            });
        });
        
        // Handle Telegram form submission
        document.querySelector('form[action="/telegram"]').addEventListener('submit', function(e) {
            e.preventDefault();
            const formData = new FormData(this);
            const statusDiv = document.getElementById('telegram-status');
            statusDiv.style.display = 'block';
            statusDiv.className = 'status info';
            statusDiv.textContent = 'Сохранение...';
            
            fetch('/telegram', {
                method: 'POST',
                body: formData
            })
            .then(response => response.text())
            .then(data => {
                if (data.includes('success')) {
                    statusDiv.className = 'status success';
                    statusDiv.textContent = '✓ Успешно сохранено!';
                } else {
                    statusDiv.className = 'status error';
                    statusDiv.textContent = '✗ Ошибка: ' + data;
                }
            })
            .catch(error => {
                statusDiv.className = 'status error';
                statusDiv.textContent = '✗ Ошибка подключения';
            });
        });
        
        // Handle Bitrix24 form submission
        document.querySelector('form[action="/bitrix24"]').addEventListener('submit', function(e) {
            e.preventDefault();
            const formData = new FormData(this);
            const statusDiv = document.getElementById('bitrix24-status');
            statusDiv.style.display = 'block';
            statusDiv.className = 'status info';
            statusDiv.textContent = 'Сохранение...';
            
            fetch('/bitrix24', {
                method: 'POST',
                body: formData
            })
            .then(response => response.text())
            .then(data => {
                if (data.includes('success')) {
                    statusDiv.className = 'status success';
                    statusDiv.textContent = '✓ Успешно сохранено!';
                } else {
                    statusDiv.className = 'status error';
                    statusDiv.textContent = '✗ Ошибка: ' + data;
                }
            })
            .catch(error => {
                statusDiv.className = 'status error';
                statusDiv.textContent = '✗ Ошибка подключения';
            });
        });
    </script>
</body>
</html>
)";

// Handle root page
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

// Handle WiFi form submission
void handleWiFi() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }
  
  String ssid = server.arg("ssid");
  String password = server.arg("password");
  
  if (ssid.length() == 0 || password.length() == 0) {
    server.send(400, "text/plain", "SSID and password are required");
    return;
  }
  
  // Save credentials
  saveWiFiCredentials(ssid.c_str(), password.c_str());
  
  // Reload credentials in wifi_telegram module
  reloadCredentials();
  
  // Try to connect
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    server.send(200, "text/plain", "success: Connected to " + ssid + " with IP " + WiFi.localIP().toString());
  } else {
    server.send(200, "text/plain", "error: Credentials saved but connection failed. Please check your WiFi settings.");
  }
}

// Handle Telegram form submission
void handleTelegram() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }
  
  String botToken = server.arg("botToken");
  String chatId = server.arg("chatId");
  
  if (botToken.length() == 0 || chatId.length() == 0) {
    server.send(400, "text/plain", "Bot token and Chat ID are required");
    return;
  }
  
  saveTelegramCredentials(botToken.c_str(), chatId.c_str());
  
  // Reload credentials in wifi_telegram module
  reloadCredentials();
  
  server.send(200, "text/plain", "success: Telegram credentials saved");
}

// Handle Bitrix24 form submission
void handleBitrix24() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }
  
  String hostname = server.arg("hostname");
  String restEndpoint = server.arg("restEndpoint");
  
  if (hostname.length() == 0 || restEndpoint.length() == 0) {
    server.send(400, "text/plain", "Hostname and REST endpoint are required");
    return;
  }
  
  // Ensure hostname starts with https://
  if (!hostname.startsWith("http://") && !hostname.startsWith("https://")) {
    hostname = "https://" + hostname;
  }
  
  // Clean up restEndpoint: remove any protocol/hostname, keep only the path
  // If user pasted full URL like "https://domain.bitrix24.ru/rest/123/abc/", extract just "/rest/123/abc/"
  if (restEndpoint.indexOf("://") >= 0) {
    // Contains protocol, extract path part
    int pathStart = restEndpoint.indexOf("/", restEndpoint.indexOf("://") + 3);
    if (pathStart >= 0) {
      restEndpoint = restEndpoint.substring(pathStart);
    } else {
      // No path found, assume it's just the endpoint
      restEndpoint = "/" + restEndpoint;
    }
  }
  
  // Ensure restEndpoint starts with / and ends with /
  if (!restEndpoint.startsWith("/")) {
    restEndpoint = "/" + restEndpoint;
  }
  // Ensure it ends with / (required for Bitrix24 REST API)
  if (!restEndpoint.endsWith("/")) {
    restEndpoint = restEndpoint + "/";
  }
  
  saveBitrix24Credentials(hostname.c_str(), restEndpoint.c_str());
  
  // Reload credentials in bitrix24 module
  reloadBitrix24Credentials();
  
  server.send(200, "text/plain", "success: Bitrix24 credentials saved");
}

// Start Access Point mode
void startAPMode() {
  if (apActive) {
    return;  // Already active
  }
  
  Serial.println("Starting Access Point mode...");
  
  // Disconnect from WiFi if connected
  WiFi.disconnect();
  delay(100);
  
  // Start AP
  WiFi.mode(WIFI_AP);
  bool apStarted = WiFi.softAP(AP_SSID, AP_PASSWORD);
  
  if (apStarted) {
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP started! SSID: ");
    Serial.print(AP_SSID);
    Serial.print(", IP: ");
    Serial.println(IP);
    
    // Setup web server routes
    server.on("/", handleRoot);
    server.on("/wifi", handleWiFi);
    server.on("/telegram", handleTelegram);
    server.on("/bitrix24", handleBitrix24);
    
    server.begin();
    apActive = true;
    Serial.println("Web server started on http://" + IP.toString());
    
    // Show AP prompt screen (stays visible until long press)
    currentViewMode = VIEW_MODE_AP_PROMPT;
    drawAPPrompt();  // Draw immediately
  } else {
    Serial.println("Failed to start AP!");
    apActive = false;
  }
}

// Stop Access Point mode
void stopAPMode() {
  if (!apActive) {
    return;  // Not active
  }
  
  Serial.println("Stopping Access Point mode...");
  server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  apActive = false;
  Serial.println("AP stopped");
}

// Handle web server requests
void handleAPWebServer() {
  if (apActive) {
    server.handleClient();
  }
}

// Check if AP is active
bool isAPActive() {
  return apActive;
}

// Get AP SSID
const char* getAPSSID() {
  return AP_SSID;
}

// Get AP password
const char* getAPPassword() {
  return AP_PASSWORD;
}

// Get AP IP address
String getAPIPAddress() {
  if (apActive) {
    IPAddress ip = WiFi.softAPIP();
    return ip.toString();
  }
  return "192.168.4.1";  // Default
}
