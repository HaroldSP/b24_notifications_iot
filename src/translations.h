// UI Text Translations
// English strings are saved as comments for reference

#ifndef TRANSLATIONS_H
#define TRANSLATIONS_H

// Main Menu
#define TXT_B24 "B24"  // English: "B24" (abbreviation, stays the same)
#define TXT_POMODORO ""  // English: "" (icon only, no text)
#define TXT_PALETTE ""  // English: "" (icon only, no text)
#define TXT_AP_ON "AP: вкл"  // English: "AP: on"
#define TXT_AP_OFF "AP: выкл"  // English: "AP: off"

// Headers
#define TXT_BITRIX24 "Bitrix24"  // English: "Bitrix24" (brand name, stays the same)

// Loading
#define TXT_LOADING "Загрузка..."  // English: "Loading..."

// Color Preview
#define TXT_WORK "РАБОТА"  // English: "WORK"
#define TXT_REST "ОТДЫХ"  // English: "REST"

// Telegram Prompt
#define TXT_OPEN_TG_BOT "Откройте TG бот"  // English: "Open TG bot"

// AP Mode
#define TXT_AP_MODE_ACTIVE "Режим AP активен"  // English: "AP Mode Active"
#define TXT_CONNECT_TO_WIFI "1) Подключитесь к WiFi"  // English: "1) Connect to WiFi"
#define TXT_PASSWORD_IS "2) Пароль:"  // English: "2) Password is"
#define TXT_GO_TO_AP_PAGE "3) Откройте страницу AP"  // English: "3) Go to AP page"

// Bitrix24 Section Labels
#define TXT_ALL_MSGS "Все сообщ.: "  // English: "All msgs: "
#define TXT_ALL_TASKS "Все задачи: "  // English: "All tasks: "

// Bitrix24 Section Titles (these are passed as parameters, translations handled in code)
// Section 1: "Unread" -> "Непрочитанные"
// Section 2: "Tasks" -> "Задачи"
// Section 3: "Expired Tasks" -> "Просроченные задачи"
// Section 3 subtitle: "All active tasks where you are responsible" -> "Все активные задачи, где вы ответственный"
// Section 3 subtitle (selected group): "[Group Name]" -> displayed as-is (from API)

#endif // TRANSLATIONS_H
