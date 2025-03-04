/*  КОД для NodeMCU   */

#include <FastBot.h>
#include <EEPROM.h>

#define BOT_TOKEN "7060763143:AAGFyJ0A_vfS0PEI_Q_CxZ0VNQWWH566Uxw"
#define SSID_ADDR    0                    
#define PASS_ADDR  100
#define CHATS_ADDR 200

const int ledPin = 5;

unsigned long time_wifi = 0;


bool ledStatus = 0;
bool flag = false;
bool flag_1 = false, flag_2 = false, flag_3 = false, flag_4 = false;

byte dure_index, hum_index, stage_index;
byte depth = 0;
byte supp_hum = 20;                 // хранится поддерживаемая влажность  (0/1/2)

int  stage, duration;               // хранится период между поливами     (в часах), продолжительность (в секундах)
int32_t menuID, settID, preID, menu_2ID, sett_2ID, pre_2ID;
int32_t dec_1ID = 0, dec_2ID = 0, dec_3ID = 0, dec_4ID = 0;
int val_soil_hum, val_lum, val_temp;

String WIFI_SSID = "";
String WIFI_PASS = ""; 
String chat_ids = "";
String SETT[3] { "средняя", "1 день", "1 минута" };
String durations[8] = { "20 секунд", "30 секунд", "40 секунд", "минута", "1.5 минуты", "2 минуты", "3 минуты", "5 минут" };
String arr[3]       = { "высокая", "средняя", "низкая" };
String periods[12]  = { "8 часов", "12 часов", "16 часов", "1 день", "2 дня", "3 дня", "5 дней", "неделя", "10 дней" };

int st_time = 540, end_time = 1260;         // время включения и выключения автоподсветки, в минутах относительно начала дня
byte illum = 20;                            // порог для включения автоподсветки, %

FastBot bot(BOT_TOKEN);


   {
  String str;
  int len = EEPROM.read(STR_ADDR);  // читаем длину строки
  str.reserve(len);                 // резервируем место (для оптимальности)
  for (int i = 0; i < len; i++) {
    str += (char)EEPROM.read(STR_ADDR + 1 + i);
  }
  EEPROM.commit();
  return str;
 
}

void ee_write(byte STR_ADDR, String inc) {  
  int len = inc.length();           // длина строки
  EEPROM.write(STR_ADDR, len);      // записываем её
  for (int i = 0; i < len; i++) {   // и далее саму строку посимвольно
    EEPROM.write(STR_ADDR + 1 + i, inc[i]);
  }
  EEPROM.commit();
}

bool id_in(String str, String num) {
  byte pos = str.lastIndexOf(num);
  if (pos == 255)
    return false;
  else
    return true;
}

void new_sett() {
  String str1 = "⚙️ *Текущие настройки автополива*\n\n💧 Влажность: " + String(supp_hum) + "%\n⌚️ Период: " + SETT[1] + "\n⏱ Продолжительность: " + SETT[2];
  bot.editMessage(settID, str1);
}

void new_sett_2() {
  String str2 = "⚙️ *Текущие настройки автоподсветки*\n\n";
  str2 += "🕒 Время включения:  "  + String(st_time / 60)  + ":" + String(st_time % 60 / 10)  + String(st_time % 60 % 10)  + "\n";
  str2 += "🕒 Время выключения: "  + String(end_time / 60) + ":" + String(end_time % 60 / 10) + String(end_time % 60 % 10) + "\n"; 
  str2 += "🔝 Порог включения: " + String(illum) + "%";
  bot.editMessage(sett_2ID, str2);
}

bool in(char a, String b) {
  for (int i = 0; i < b.length(); i++) if (a == b[i]) return true;
  return false;
}
bool is_time(String texts) {
  if (texts.length() == 5 and texts[2] == ':' and ((in(texts[0], "01") and in(texts[1], "0123456789")) or (texts[0] == '2' and in(texts[1], "0123"))) and in(texts[3], "012345") and in(texts[4], "01213456789")) 
  { return true; }
  else return false;
}

void delete_dec() {
  if (dec_1ID) bot.deleteMessage(dec_1ID); dec_1ID = 0;
  if (dec_2ID) bot.deleteMessage(dec_2ID); dec_2ID = 0;
  if (dec_3ID) bot.deleteMessage(dec_3ID); dec_3ID = 0;
}


// обработчик сообщений
void newMsg(FB_msg& msg) {
  
  String chat_id = msg.userID;  
  String texts = msg.text;

  Serial.println("CHATID: " + msg.userID);
  Serial.println("ChatIDs /" + chat_ids + "/");
  Serial.println("IN " + String(id_in(chat_ids, chat_id)));
  Serial.println("TEXT: "   + texts);
  Serial.println("DATA: "   + msg.data);
  Serial.println();

  if (texts == "/start") {
    bot.sendTyping(chat_id);
    String res = "👋 " + msg.username + ", добро пожаловать в бот для управления системой «Умный подоконник»\n\n";
    res += "✍️ Для авторизации пришлите код, указанный в разделе Settings->CODE вашего устройства, в формате /хххх";
    flag = true; 
    bot.sendMessage(res, chat_id);
    bot.deleteMessage(bot.lastUsrMsg(), chat_id);
  } 

  else if (texts == "/start 3242" or (texts == "/5555" and flag == true)) {
    bot.sendTyping(chat_id); 
    bot.sendMessage("✅ Авторизация успешно пройдена\n\n👇 Для получения списка доступных команд жмите /help", chat_id);
    bot.deleteMessage(bot.lastUsrMsg(), chat_id);
    flag = false;
    if (not id_in(chat_ids, chat_id)) chat_ids += String(chat_id) + ",";

    ee_write(CHATS_ADDR, chat_ids);
    Serial.print("Chat_ids is new: /" + chat_ids + "/ in EEPROM /" + ee_read(CHATS_ADDR) + "/");
  }

  else if (flag) {
    bot.sendTyping(chat_id); 
    bot.sendMessage("❌ Неверный код, попробуйте снова", chat_id);
    bot.deleteMessage(bot.lastUsrMsg(), chat_id);
  }

  else if (id_in(chat_ids, chat_id)) {
    
    bot.setChatID(msg.userID);
    
    if (texts == "/help") {
      String welcome;
      welcome = "*📃 Перечень доступных комманд:*\n\n";
      welcome += "/lighton - включить автоподсветку \n";
      welcome += "/lightoff - выключить автоподсветку \n";
      welcome += "/pumpon - включить автополив\n";
      welcome += "/pumpoff - выключить автополив\n";
      welcome += "/data - получить освещенность, температуру и влажность почвы\n\n";
      welcome += "/settime - установить время на устройстве\n";
      welcome += "/gettime - получить время с устройства\n";
      welcome += "/autolight - настроить автоподсветку\n";
      welcome += "/autowater - настроить автополив\n\n";
      welcome += "/help - доступные команды\n";
      welcome += "/info - краткая справка\n";
      bot.sendTyping(chat_id); 
      bot.sendMessage(welcome);
      bot.deleteMessage(bot.lastUsrMsg());
      welcome = "";
    }
    
    else if (texts == "/ledon") {
      bot.sendTyping(chat_id);
      digitalWrite(ledPin, LOW); 
      Serial.write(1);
      ledStatus = 1;
      bot.sendMessage("Led is ON");
      bot.deleteMessage(bot.lastUsrMsg());
    }
  
    else if (texts == "/ledoff") {
      bot.sendTyping(chat_id);
      ledStatus = 0;
      Serial.write(0);
      digitalWrite(ledPin, HIGH); 
      bot.sendMessage("Led is OFF");
      bot.deleteMessage(bot.lastUsrMsg());
    }
  
    else if (texts == "/status") {
      bot.sendTyping(chat_id);
      if (ledStatus) { bot.sendMessage("Led is ON");  bot.deleteMessage(bot.lastUsrMsg()); }
      else           { bot.sendMessage("Led is OFF"); bot.deleteMessage(bot.lastUsrMsg()); }
    }
  
    else if (texts == "/lighton") {
      bot.sendTyping(chat_id);
      Serial.write(25);
      bot.sendMessage("🟢 Автоподсветка включена");
      bot.deleteMessage(bot.lastUsrMsg());
    }
    
    else if (texts == "/lightoff") {
      bot.sendTyping(chat_id);
      Serial.write(63);
      bot.sendMessage("🔴 Автоподсветка выключена");
      bot.deleteMessage(bot.lastUsrMsg());
    }
    
    else if (texts == "/pumpon") {
      bot.sendTyping(chat_id);
      Serial.write(4);
      bot.sendMessage("🟢 Автополив растений включён");
      bot.deleteMessage(bot.lastUsrMsg());
    }
    
    else if (texts == "/pumpoff") {
      bot.sendTyping(chat_id);
      Serial.write(5);
      bot.sendMessage("🔴 Автополив растений выключен");
      bot.deleteMessage(bot.lastUsrMsg());
    }
    
    else if (texts == "/data") {
      bot.sendTyping(chat_id);
      Serial.write(7);
      clear_ser();
      delay(50);
      val_lum = Serial.parseInt(); val_temp = Serial.parseInt(); val_soil_hum = Serial.parseInt();
      String res = "💧 Влажность почвы составляет " + String(val_soil_hum) + "%\n";
      res += "☀️ Освещенность составляет " + String(val_lum) + "%\n" + "🌡 Температура составляет " + String(val_temp) + "°C";
      bot.sendMessage(res);
      bot.deleteMessage(bot.lastUsrMsg());
    }
    
    else if (texts == "/settime") {
      bot.sendTyping(chat_id);
      Serial.write(8);
      FB_Time t = bot.getTime(3);
      delay(20);
      Serial.print(t.timeString() + ' ' + t.dateString() + ' ' + String(byte(t.dayWeek)));
      String weeks[7] = {"Пн", "Вт", "Ср", "Чт", "Пт", "Сб", "Вс"};
      bot.sendMessage("🆕 Время обновлено\n\n⌚️ Текущие дата, время и день недели на устройстве:\n" + t.dateString() + "    " + t.timeString() + ' ' + weeks[byte(t.dayWeek) - 1]);
      bot.deleteMessage(bot.lastUsrMsg());
    }
    
    else if (texts == "/gettime") {
      bot.sendTyping(chat_id);
      Serial.write(9);
      clear_ser();
      delay(100);
      String times = "⌚️ Текущие дата, время и день недели на устройстве: \n" + Serial.readString();
      bot.sendMessage(times);
      bot.deleteMessage(bot.lastUsrMsg());
    }
  
    else if (texts == "/autowater") {
      bot.sendTyping(chat_id);
      bot.sendMessage("👇 Выберите влажность, ниже которой будет присылаться уведомление, период между поливами, их продолжительность \n\n❗️В конце обязательно нажмите кнопку подтвердить");
      preID = bot.lastBotMsg();
      
      String menu1 = F("💧 Влажность \n ⌚️ Период времени \n ⏱ Продолжительность \n ✅ Подтвердить");
      String call1 = F("menu_1, menu_2, menu_3, confirm");
      String menu = "⚙️ *Настройки автополива*\n\n👇 Выберите тип автополива";
      bot.inlineMenuCallback(menu, menu1, call1);
      menuID = bot.lastBotMsg();    // запомнили ID сообщения с меню
  
      clear_ser();
      Serial.write(17);
      delay(100);
      supp_hum = Serial.parseInt();
      SETT[1] = periods[Serial.parseInt() - 1];
      SETT[2] = durations[Serial.parseInt() - 1];
      
      String str1 = "⚙️ *Текущие настройки автополива*\n\n💧 Влажность: " + String(supp_hum) + "%\n⌚️ Период: " + SETT[1] + "\n⏱ Продолжительность: " + SETT[2];
      bot.sendMessage(str1);
      settID = bot.lastBotMsg(); 
      
      bot.deleteMessage(bot.lastUsrMsg());
    }
  
    else if (msg.data == "menu_1" and flag_4 == false) {
      bot.sendMessage("💬 Отправьте влажность в формате хх, ниже которой будет присылаться уведомление");
      flag_4 = true; dec_4ID = bot.lastBotMsg();
    }

    else if (msg.data == "menu_1") { Serial.println("MENU_1\n"); }
    
    else if (msg.data == "menu_2") {
      String menu3 = F("8 часов \t 12 часов \t 16 часов \n 1 день \t 2 дня \t 3 дня \n 5 дней \t неделя \t 10 дней \n ◀️ Назад");
      String call3 = F("answ_2_1, answ_2_2, answ_2_3, answ_2_4, answ_2_5, answ_2_6, answ_2_7, answ_2_8, answ_2_9, back");
      bot.editMenuCallback(menuID, menu3, call3);
      depth = 1;
    }
    
    else if (msg.data[5] == '2') {
      stage_index = int(msg.data[7]) - 49;                          // 0, 1, 2 ... 11
      int    arr[12] =     { 8, 12, 16, 24, 48, 72, 120, 168, 240};   // в часах 
      SETT[1] = periods[stage_index];
      stage   = arr[stage_index]; 
      new_sett();
    }
      
    else if (msg.data == "menu_3") {
      String menu4 = F("20 секунд \t 30 секунд \n 40 секунд \t минута \n 1.5 минуты \t 2 минуты \n 3 минуты \t 5 минут \n  ◀️ Назад");
      String call4 = F("answ_3_1, answ_3_2, answ_3_3, answ_3_4, answ_3_5, answ_3_6, answ_3_7, answ_3_8, back");
      bot.editMenuCallback(menuID, menu4, call4);
      depth = 1;
    }
    
    else if (msg.data[5] == '3') {
      dure_index = int(msg.data[7]) - 49;                          // 0, 1, 2 ... 11
      int   arr[8]       = { 20, 30, 40, 60, 90, 120, 180, 300 };   // в секундах
      SETT[2]  = durations[dure_index];
      duration = arr[dure_index]; 
      new_sett();
    }
    
    else if (msg.data == "back" && depth == 1) {
      String menu1 = F("💧 Влажность \n ⌚️ Период времени \n ⏱ Продолжительность \n ✅ Подтвердить");
      String call1 = F("menu_1, menu_2, menu_3, confirm");
      bot.editMenuCallback(menuID, menu1, call1);
      depth = 0;
    }
  
    else if (msg.data == "confirm") {
      bot.deleteMessage(menuID);
      bot.deleteMessage(preID);
      Serial.write(36);
      delay(10);
      Serial.print(String(supp_hum + 1) + "," + String(stage) + "," + String(duration));  
      delay(200);
      Serial.write(64);
      delay(10);
      Serial.print(String(hum_index + 1) + "," + String(stage_index + 1) + "," + String(dure_index + 1));
    }

    else if (flag_4) {
      if (texts.length() == 2 and in(texts[0], "0123456789") and in(texts[1], "0123456789")) {
        supp_hum = (byte(texts[0]) - 48) * 10 + byte(texts[1]) - 48;
        new_sett(); 
        if (dec_4ID) bot.deleteMessage(dec_4ID); dec_4ID = 0; 
        flag_4 = false;
      }
      bot.deleteMessage(bot.lastUsrMsg());
    }
    
    else if (texts == "/info") {
      bot.sendTyping(chat_id);
      String res = "Smart plant system\nEquipment from China\nSoftware from Russia\nDeveloped by me";
      bot.sendMessage(res);
      bot.deleteMessage(bot.lastUsrMsg());
    } 
  
    else if (texts == "/autolight") {
      bot.sendTyping(chat_id);
      bot.sendMessage("👇 Выберите время включения и выключения автоподсветки, а также порог её срабатывания\n\n❗️В конце обязательно нажмите кнопку подтвердить");
      pre_2ID = bot.lastBotMsg();
      
      String menu4 = F("🕒 Время включения \n 🕒 Время выключения \n ☀️ Освещённость \n ✅ Подтвердить");
      String call4 = F("menu_5, menu_6, menu_7, confirm_2");
      String menu = "⚙️ *Настройки автоподсветки*\n\nВыберите тип автоподсветки";
      bot.inlineMenuCallback(menu, menu4, call4);
      menu_2ID = bot.lastBotMsg();
      
      Serial.write(19);
      clear_ser();
      delay(100);
      st_time  = Serial.parseInt();
      end_time = Serial.parseInt();
      illum    = Serial.parseInt();
      
      String str2 = "⚙️ *Текущие настройки автоподсветки*\n\n";
      str2 += "🕒 Время включения:  "  + String(st_time / 60)  + ":" + String(st_time % 60 / 10)  + String(st_time % 60 % 10)  + "\n";
      str2 += "🕒 Время выключения: "  + String(end_time / 60) + ":" + String(end_time % 60 / 10) + String(end_time % 60 % 10) + "\n"; 
      str2 += "🔝 Порог включения: " + String(illum) + "%";
      bot.sendMessage(str2);
      sett_2ID = bot.lastBotMsg();
      
      bot.deleteMessage(bot.lastUsrMsg());
    }
  
    else if (msg.data == "menu_5" and flag_1 == false and flag_2 == false and flag_3 == false) {
      bot.sendMessage("💬 Отправьте время включения в формате хх:хх с ведущими нолями");
      flag_1 = true; dec_1ID = bot.lastBotMsg();
    }
  
    else if (msg.data == "menu_6" and flag_1 == false and flag_2 == false and flag_3 == false) {
      bot.sendMessage("💬 Отправьте время выключения в формате хх:хх с ведущими нолями");
      flag_2 = true; dec_2ID = bot.lastBotMsg();
    }
    
    else if (msg.data == "menu_7" and flag_1 == false and flag_2 == false and flag_3 == false) {
      bot.sendMessage("💬 Отправьте освещённость в процентах в формате хх с ведущим нулём, ниже которой подсветка будет включаться");
      flag_3 = true; dec_3ID = bot.lastBotMsg();
    }
  
    
    else if (msg.data == "menu_5" or msg.data == "menu_6" or msg.data == "menu_7") { Serial.println("MENU_5/6/7\n"); }
  
    else if (msg.data == "confirm_2") {
      bot.deleteMessage(menu_2ID);
      bot.deleteMessage(pre_2ID);
      delete_dec();
      flag_1 = false; flag_2 = false; flag_3 = false;
      Serial.write(60);
      delay(10);
      Serial.print(String(st_time) + "," + String(end_time) + "," + String(illum));  
    }
    
    else if (flag_1) {
      if (is_time(texts)) {
        st_time = ((int(texts[0]) - 48) * 10 + int(texts[1]) - 48) * 60 + (int(texts[3]) - 48) * 10 + int(texts[4]) - 48;
        new_sett_2(); delete_dec(); flag_1 = false;   
      }
      bot.deleteMessage(bot.lastUsrMsg());
    }
  
    else if (flag_2) {
      if (is_time(texts)) {
        end_time = ((int(texts[0]) - 48) * 10 + int(texts[1]) - 48) * 60 + (int(texts[3]) - 48) * 10 + int(texts[4]) - 48;
        new_sett_2(); delete_dec(); flag_2 = false;     
      }
      bot.deleteMessage(bot.lastUsrMsg());
    }
  
    else if (flag_3) {
      if (texts.length() == 2 and in(texts[0], "0123456789") and in(texts[1], "0123456789")) {
        illum = (byte(texts[0]) - 48) * 10 + byte(texts[1]) - 48;
        new_sett_2(); delete_dec(); flag_3 = false;
      }
      bot.deleteMessage(bot.lastUsrMsg());
    }
    
    else {
      bot.sendTyping(chat_id);  
      bot.sendMessage("❌ Вводите команду, а не текст");
      bot.deleteMessage(bot.lastUsrMsg());
    }
    bot.setChatID(0);
  }
  
  else {
    bot.sendTyping(chat_id);  
    bot.sendMessage("👇 Для начала работы жми \n /start", chat_id);
    bot.deleteMessage(bot.lastUsrMsg(), chat_id);
  }
}

void connectWiFi() {
  digitalWrite(ledPin, HIGH);

  get_ssid_pass();
  
  delay(2000);
  Serial.println();
  
  Serial.println("Connect SSID /" + WIFI_SSID + "/");
  Serial.println("Connect PASS /" + WIFI_PASS + "/");
  Serial.println("LEN of SSID " + String(WIFI_SSID.length()));
  Serial.println("LEN of PASS " + String(WIFI_PASS.length()));

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() > 15000) ESP.restart();
  }
  Serial.println("Connected:)");
  digitalWrite(ledPin, LOW);
}

void get_ssid_pass() {
  clear_ser();
  Serial.write(20);
  delay(50);
  WIFI_SSID = Serial.readStringUntil('|');
  WIFI_PASS = Serial.readStringUntil('|');
}

void clear_ser() { while (Serial.available()) Serial.read(); }

void handleData() {
  if (Serial.available() > 0) {
    byte val = Serial.read();
    if (val == 20) {
      delay(50);
      ee_write(SSID_ADDR, Serial.readStringUntil('\n'));
      ee_write(PASS_ADDR, Serial.readStringUntil('\n'));

      WIFI_SSID = ee_read(SSID_ADDR); Serial.println("NEW SSID /" + WIFI_SSID + "/");
      WIFI_PASS = ee_read(PASS_ADDR); Serial.println("NEW PASS /" + WIFI_PASS + "/");
      connectWiFi();
    }
    if (val == 36) {
      String names = chat_ids.substring(0, chat_ids.length() - 1);
      Serial.println(names);
      bot.sendMessage("🆘 Низкий уровень воды баке", names);
    }
    if (val == 64) {
      delay(50); String res = Serial.readString();
      String names = chat_ids.substring(0, chat_ids.length() - 1);
      Serial.println(names);
      bot.sendMessage("🆘 Низкий уровень влажности почвы - " + res + "%", names);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  EEPROM.begin(300);
  delay(50);
  
  connectWiFi();  
  bot.attach(newMsg);                         // подключаем функцию-обработчик
  bot.setTextMode(FB_MARKDOWN);  
  chat_ids = ee_read(CHATS_ADDR);
}

void loop() {
  bot.tick();   // тикаем в луп
  handleData();

  if (millis() - time_wifi > 10000) {
    if (WiFi.status() != WL_CONNECTED) {
      connectWiFi();
    }
  }
}
