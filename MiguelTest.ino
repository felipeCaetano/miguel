#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>

#define GFX_BL 2
#define TFT_BL 2

#if defined(DISPLAY_DEV_KIT)
Arduino_GFX *gfx = create_default_Arduino_GFX();
#else  /* !defined(DISPLAY_DEV_KIT) */
Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
  GFX_NOT_DEFINED /* CS */, GFX_NOT_DEFINED /* SCK */, GFX_NOT_DEFINED /* SDA */,
  40 /* DE */, 41 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */,
  45 /* R0 */, 48 /* R1 */, 47 /* R2 */, 21 /* R3 */, 14 /* R4 */,
  5 /* G0 */, 6 /* G1 */, 7 /* G2 */, 15 /* G3 */, 16 /* G4 */, 4 /* G5 */,
  8 /* B0 */, 3 /* B1 */, 46 /* B2 */, 9 /* B3 */, 1 /* B4 */
);
// option 1:
// Uncomment for ILI6485 LCD 480x272
// Arduino_RPi_DPI_RGBPanel *gfx = new Arduino_RPi_DPI_RGBPanel(
//     bus,
//     480 /* width */, 0 /* hsync_polarity */, 8 /* hsync_front_porch */, 4 /* hsync_pulse_width */, 43 /* hsync_back_porch */,
//     272 /* height */, 0 /* vsync_polarity */, 8 /* vsync_front_porch */, 4 /* vsync_pulse_width */, 12 /* vsync_back_porch */,
//     1 /* pclk_active_neg */, 9000000 /* prefer_speed */, true /* auto_flush */);
// option 2:
// Uncomment for ST7262 IPS LCD 800x480
Arduino_RPi_DPI_RGBPanel *gfx = new Arduino_RPi_DPI_RGBPanel(
  bus,
  800 /* width */, 0 /* hsync_polarity */, 8 /* hsync_front_porch */, 4 /* hsync_pulse_width */, 8 /* hsync_back_porch */,
  480 /* height */, 0 /* vsync_polarity */, 8 /* vsync_front_porch */, 4 /* vsync_pulse_width */, 8 /* vsync_back_porch */,
  1 /* pclk_active_neg */, 14000000 /* prefer_speed */, true /* auto_flush */);
#endif /* !defined(DISPLAY_DEV_KIT) */
/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/

#include "touch.h"
#include "time.h"
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -3 * 60 * 60;  // Set your timezone here
const int daylightOffset_sec = 0;

// #include <EEPROM.h>
// #define EEPROM_SIZE 512
// #define EEPROM_ADDR_WIFI_FLAG 0
// #define EEPROM_ADDR_WIFI_CREDENTIAL 4


static uint32_t screenWidth = 800;
static uint32_t screenHeight = 480;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf;
static lv_disp_drv_t disp_drv;

int16_t btn_width = 100;
int16_t btn_height = 100;

/*display flushing*/
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
#if (LV_COLOR_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

  lv_disp_flush_ready(disp);
}

void my_touch_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  if (touch_has_signal()) {
    if (touch_touched()) {
      data->state = LV_INDEV_STATE_PR;

      /*Set the coordinates*/
      data->point.x = touch_last_x;
      data->point.y = touch_last_y;
    } else if (touch_released()) {
      data->state = LV_INDEV_STATE_REL;
    }
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

WebServer server(80);

String long_text = "";



void enviarNotificacao() {
  Serial.println("Enviar Notificação");
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;
  char send_txt[1024];
  if (long_text.length() > 1) {
    Serial.println(long_text);
    snprintf(send_txt, 1024,
             "Uma Nova Mensagem!<br>\
     <p>Em: %d: %d: %d - %s</p>",
             hr, min, sec, long_text.c_str());
    Serial.println(long_text.length());
    server.send(200, "text/html", send_txt);

  } else {
    server.send(200, "text/html", "Sem Novas Mensagens.");
  }
}

/*
WIFI initialization:
Wifi credentials:
// */
const char *ssid = "POCO X3 Pro";
const char *password = "12345678";
static int foundNetworks = 0;
unsigned long networkTimeout = 10000;
String ssidName, psword;

TaskHandle_t ntScanTaskHandler, ntConnectTaskHandler;
std::vector<String> foundWifiList;

/*WEB SERVER FUNCTIONS*/
void handleRoot() {
  server.send(200, "text/Html", "<h1>Comunicador de Miguel</h1>");
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

static lv_style_t style_but;
static lv_style_t style_pr;
static lv_style_t border_style;
static lv_style_t popupBox_style;
static lv_obj_t *screenMain;
static lv_obj_t *timeLabel;
static lv_obj_t *settingBtn;
static lv_obj_t *settings;
static lv_obj_t *settingCloseBtn;
static lv_obj_t *settingWiFiSwitch;
static lv_obj_t *wfList;
static lv_obj_t *settinglabel;
static lv_obj_t *mboxConnect;
static lv_obj_t *mboxTitle;
static lv_obj_t *mboxPassword;
static lv_obj_t *mboxConnectBtn;
static lv_obj_t *mboxCloseBtn;
static lv_obj_t *keyboard;
static lv_obj_t *popupBox;
static lv_obj_t *popupBoxCloseBtn;
static lv_timer_t *timer;

/*Carregando imagens*/
LV_IMG_DECLARE(eu);
LV_IMG_DECLARE(mae);
LV_IMG_DECLARE(voce);
LV_IMG_DECLARE(vovo);
LV_IMG_DECLARE(ajuda);
LV_IMG_DECLARE(assistir);
LV_IMG_DECLARE(beber);
LV_IMG_DECLARE(comer);
LV_IMG_DECLARE(nao);
LV_IMG_DECLARE(passear);
LV_IMG_DECLARE(pegar);
LV_IMG_DECLARE(querer);
LV_IMG_DECLARE(entrar);
LV_IMG_DECLARE(sair);
LV_IMG_DECLARE(sim);
LV_IMG_DECLARE(brincar);

LV_IMG_DECLARE(youtube);
LV_IMG_DECLARE(agua);
LV_IMG_DECLARE(alexa);
LV_IMG_DECLARE(biscoito);
LV_IMG_DECLARE(bita);
LV_IMG_DECLARE(brinquedo);
LV_IMG_DECLARE(fusca);
LV_IMG_DECLARE(maca);
LV_IMG_DECLARE(netflix);
LV_IMG_DECLARE(pipoca);
LV_IMG_DECLARE(pizza);
LV_IMG_DECLARE(uva);

typedef enum {
  NONE,
  NETWORK_SEARCHING,
  NETWORK_CONNECTED_POPUP,
  NETWORK_CONNECTED,
  NETWORK_CONNECT_FAILED
} Network_Status_t;
Network_Status_t networkStatus = NONE;

void setup() {
  Serial.begin(115200);
  touch_init();
  gfx->begin();
  gfx->fillScreen(BLACK);

#ifdef TFT_BL
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
#endif

  lv_init();

#ifdef ESP32
  disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * screenWidth * 10, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#else
  disp_draw_buf = (lv_color_t *)malloc(sizeof(lv_color_t) * screenWidth * 10);
#endif
  if (!disp_draw_buf) {
    //Serial.println("LVGL draw_buf allocate failed!");
  } else {
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, screenWidth * 10);

    /*Initialize Display*/
    lv_disp_drv_init(&disp_drv);

    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /*Initialize the (dummy) input device driver*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touch_read;
    lv_indev_drv_register(&indev_drv);

    // /*WiFi initialization*/
    WiFi.begin(ssid, password);

    /*CREATE SCREEN OBJECTS*/
    screenMain = lv_scr_act();  //lv_obj_create(NULL);
    // lv_obj_t *label = lv_label_create(screenMain);

    makeKeyboard(screenMain);
    setStyle();
    buildStatusBar(screenMain);
    buildPWMsgBox();
    buildBody();
    buildSettings();
    // tryPreviousNetwork();

    if (MDNS.begin("miguelaac")) {
      Serial.println("MDNS responder started");
    }

    server.on("/", handleRoot);
    server.on("/notificacao", enviarNotificacao);
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("HTTP server started");
  }
}

void loop() {
  lv_timer_handler();
  delay(5);
  server.handleClient();
}

lv_obj_t *lv_pictogram_create(lv_obj_t *screenMain, const void *img_src, const char *texto, lv_obj_t *label) {
  lv_obj_t *imgbtn = lv_btn_create(screenMain);
  lv_obj_t *img = lv_img_create(imgbtn);
  lv_img_set_src(img, img_src);
  lv_obj_align(img, LV_ALIGN_BOTTOM_MID, 0, 10);
  lv_obj_add_event_cb(imgbtn, event_handler, LV_EVENT_ALL, label);
  lv_obj_set_size(imgbtn, 100, 100);

  lv_obj_t *labelbtn = lv_label_create(imgbtn);
  lv_label_set_text(labelbtn, texto);
  lv_obj_align(labelbtn, LV_ALIGN_TOP_MID, 0, -10);
  return imgbtn;
}

static void setStyle() {
  lv_style_init(&border_style);
  lv_style_set_border_width(&border_style, 2);
  lv_style_set_border_color(&border_style, lv_color_black());

  lv_style_init(&popupBox_style);
  lv_style_set_radius(&popupBox_style, 10);
  lv_style_set_bg_opa(&popupBox_style, LV_OPA_COVER);
  lv_style_set_border_color(&popupBox_style, lv_palette_main(LV_PALETTE_BLUE));
  lv_style_set_border_width(&popupBox_style, 5);

  /*Darken the button when pressed and make it wider*/
  lv_style_init(&style_pr);
  lv_style_set_img_recolor_opa(&style_pr, LV_OPA_30);
  lv_style_set_img_recolor(&style_pr, lv_color_black());
  lv_style_set_transform_width(&style_pr, 5);

  lv_style_init(&style_but);
  lv_style_set_bg_color(&style_but, lv_color_white());
  lv_style_set_border_color(&style_but, lv_palette_darken(LV_PALETTE_LIGHT_BLUE, 3));
  lv_style_set_border_width(&style_but, 3);
  lv_style_set_border_opa(&style_but, LV_OPA_50);
  lv_style_set_shadow_width(&style_but, 10);
  lv_style_set_text_color(&style_but, lv_color_black());
}

static void makeKeyboard(lv_obj_t *screenMain) {
  keyboard = lv_keyboard_create(screenMain);
  lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
}

static void buildStatusBar(lv_obj_t *screenMain) {
  static lv_style_t style_btn;
  lv_style_init(&style_btn);
  lv_style_set_bg_color(&style_btn, lv_color_hex(0xc5c5c5));
  lv_style_set_bg_opa(&style_btn, LV_OPA_50);

  lv_obj_t *statusbar = lv_obj_create(screenMain);
  lv_obj_set_size(statusbar, screenWidth, 35);
  lv_obj_align(statusbar, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_remove_style(statusbar, NULL, LV_PART_SCROLLBAR | LV_STATE_ANY);

  timeLabel = lv_label_create(statusbar);
  lv_obj_set_size(timeLabel, screenWidth - 50, 30);

  lv_label_set_text(timeLabel, "WiFi nao conectado!    " LV_SYMBOL_CLOSE);
  lv_obj_align(timeLabel, LV_ALIGN_LEFT_MID, 8, 4);

  settingBtn = lv_btn_create(statusbar);
  lv_obj_set_size(settingBtn, 30, 30);
  lv_obj_align(settingBtn, LV_ALIGN_RIGHT_MID, 0, 0);

  lv_obj_add_event_cb(settingBtn, btn_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_t *label = lv_label_create(settingBtn);
  lv_label_set_text(label, LV_SYMBOL_SETTINGS);
  lv_obj_center(label);
}

static void btn_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *btn = lv_event_get_target(e);
  // lv_obj_t *screenMain = lv_scr_act();

  if (code == LV_EVENT_CLICKED) {
    if (btn == settingBtn) {
      lv_obj_clear_flag(settings, LV_OBJ_FLAG_HIDDEN);
    } else if (btn == settingCloseBtn) {
      lv_obj_add_flag(settings, LV_OBJ_FLAG_HIDDEN);
    } else if (btn == mboxConnectBtn) {
      password = lv_textarea_get_text(mboxPassword);  //String(lv_textarea_get_text(mboxPassword));

      networkConnector();
      lv_obj_move_background(mboxConnect);

      popupMsgBox("Connecting!", "Attempting to connect to the selected network.");
      Serial.println("Passou deu aqui");
    } else if (btn == mboxCloseBtn) {
      lv_obj_move_background(mboxConnect);
    } else if (btn == popupBoxCloseBtn) {
      lv_obj_move_background(popupBox);
    }

  } else if (code == LV_EVENT_VALUE_CHANGED) {
    if (btn == settingWiFiSwitch) {

      if (lv_obj_has_state(btn, LV_STATE_CHECKED)) {

        if (ntScanTaskHandler == NULL) {
          networkStatus = NETWORK_SEARCHING;
          networkScanner();
          timer = lv_timer_create(timerForNetwork, 1000, wfList);
          lv_list_add_text(wfList, "WiFi: Looking for Networks...");
        }

      } else {

        if (ntScanTaskHandler != NULL) {
          networkStatus = NONE;
          vTaskDelete(ntScanTaskHandler);
          ntScanTaskHandler = NULL;
          lv_timer_del(timer);
          lv_obj_clean(wfList);
        }

        if (WiFi.status() == WL_CONNECTED) {
          WiFi.disconnect(true);
          lv_label_set_text(timeLabel, "WiFi Not Connected!    " LV_SYMBOL_CLOSE);
        }
      }
    }
  }
}

static void buildPWMsgBox() {

  mboxConnect = lv_obj_create(lv_scr_act());
  // lv_obj_add_style(mboxConnect, &border_style, 0);
  lv_obj_set_size(mboxConnect, gfx->width() * 2 / 3, gfx->height() / 2);
  lv_obj_center(mboxConnect);

  mboxTitle = lv_label_create(mboxConnect);
  lv_label_set_text(mboxTitle, "Selected WiFi SSID: ThatProject");
  lv_obj_align(mboxTitle, LV_ALIGN_TOP_LEFT, 0, 0);

  mboxPassword = lv_textarea_create(mboxConnect);
  lv_obj_set_size(mboxPassword, gfx->width() / 2, 40);
  lv_obj_align_to(mboxPassword, mboxTitle, LV_ALIGN_TOP_LEFT, 0, 30);
  lv_textarea_set_placeholder_text(mboxPassword, "Password?");
  lv_obj_add_event_cb(mboxPassword, text_input_event_cb, LV_EVENT_ALL, keyboard);

  mboxConnectBtn = lv_btn_create(mboxConnect);
  lv_obj_add_event_cb(mboxConnectBtn, btn_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_align(mboxConnectBtn, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  lv_obj_t *btnLabel = lv_label_create(mboxConnectBtn);
  lv_label_set_text(btnLabel, "Connect");
  lv_obj_center(btnLabel);

  mboxCloseBtn = lv_btn_create(mboxConnect);
  lv_obj_add_event_cb(mboxCloseBtn, btn_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_align(mboxCloseBtn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  lv_obj_t *btnLabel2 = lv_label_create(mboxCloseBtn);
  lv_label_set_text(btnLabel2, "Cancel");
  lv_obj_center(btnLabel2);
}

static void text_input_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *ta = lv_event_get_target(e);

  if (code == LV_EVENT_FOCUSED) {
    lv_obj_move_foreground(keyboard);
    lv_keyboard_set_textarea(keyboard, ta);
    lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
  }

  if (code == LV_EVENT_DEFOCUSED) {
    lv_keyboard_set_textarea(keyboard, NULL);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
  }
}

static void buildSettings() {
  settings = lv_obj_create(lv_scr_act());
  lv_obj_add_style(settings, &border_style, 0);
  lv_obj_set_size(settings, gfx->width() - 100, gfx->height() - 40);
  lv_obj_align(settings, LV_ALIGN_TOP_RIGHT, -20, 20);

  settinglabel = lv_label_create(settings);
  lv_label_set_text(settinglabel, "Settings " LV_SYMBOL_SETTINGS);
  lv_obj_align(settinglabel, LV_ALIGN_TOP_LEFT, 0, 0);

  settingCloseBtn = lv_btn_create(settings);
  lv_obj_set_size(settingCloseBtn, 30, 30);
  lv_obj_align(settingCloseBtn, LV_ALIGN_TOP_RIGHT, 0, -10);
  lv_obj_add_event_cb(settingCloseBtn, btn_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_t *btnSymbol = lv_label_create(settingCloseBtn);
  lv_label_set_text(btnSymbol, LV_SYMBOL_CLOSE);
  lv_obj_center(btnSymbol);

  settingWiFiSwitch = lv_switch_create(settings);
  lv_obj_add_event_cb(settingWiFiSwitch, btn_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_align_to(settingWiFiSwitch, settinglabel, LV_ALIGN_TOP_RIGHT, 60, -10);
  lv_obj_add_flag(settings, LV_OBJ_FLAG_HIDDEN);

  wfList = lv_list_create(settings);
  lv_obj_set_size(wfList, gfx->width() - 140, 210);
  lv_obj_align_to(wfList, settinglabel, LV_ALIGN_TOP_LEFT, 0, 30);
}

// void tryPreviousNetwork() {
//   if (!EEPROM.begin(EEPROM_SIZE)) {
//     delay(1000);
//     ESP.restart();
//   }
//   loadWIFICredentialEEPROM();
// }

// void saveWIFICredentialEEPROM(int flag, String ssidpw) {
//   EEPROM.writeInt(EEPROM_ADDR_WIFI_FLAG, flag);
//   EEPROM.writeString(EEPROM_ADDR_WIFI_CREDENTIAL, flag == 1 ? ssidpw : "");
//   EEPROM.commit();
// }

// void loadWIFICredentialEEPROM() {
//   int wifiFlag = EEPROM.readInt(EEPROM_ADDR_WIFI_FLAG);
//   String wifiCredential = EEPROM.readString(EEPROM_ADDR_WIFI_CREDENTIAL);

//   if (wifiFlag == 1 && wifiCredential.length() != 0 && wifiCredential.indexOf(" ") != -1) {
//     char preSSIDName[30], preSSIDPw[30];
//     if (sscanf(wifiCredential.c_str(), "%s %s", preSSIDName, preSSIDPw) == 2) {

//       lv_obj_add_state(settingWiFiSwitch, LV_STATE_CHECKED);
//       lv_event_send(settingWiFiSwitch, LV_EVENT_VALUE_CHANGED, NULL);

//       popupMsgBox("Welcome Back!", "Attempts to reconnect to the previously connected network.");
//       ssid = String(preSSIDName);
//       psword = String(preSSIDPw);
//       networkConnector();
//     } else {
//       saveWIFICredentialEEPROM(0, "");
//     }
//   }
// }

static void timerForNetwork(lv_timer_t *timer) {
  LV_UNUSED(timer);

  switch (networkStatus) {

    case NETWORK_SEARCHING:
      showingFoundWiFiList();
      break;

    case NETWORK_CONNECTED_POPUP:
      popupMsgBox("WiFi Connected!", "Now you'll get the current time soon.");
      networkStatus = NETWORK_CONNECTED;
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      break;

    case NETWORK_CONNECTED:

      showingFoundWiFiList();
      updateLocalTime();
      break;

    case NETWORK_CONNECT_FAILED:
      networkStatus = NETWORK_SEARCHING;
      popupMsgBox("Oops!", "Please check your wifi password and try again.");
      break;

    default:
      break;
  }
}

static void showingFoundWiFiList() {
  if (foundWifiList.size() == 0 || foundNetworks == foundWifiList.size())
    return;

  lv_obj_clean(wfList);
  lv_list_add_text(wfList, foundWifiList.size() > 1 ? "WiFi: Found Networks" : "WiFi: Not Found!");

  for (std::vector<String>::iterator item = foundWifiList.begin(); item != foundWifiList.end(); ++item) {
    lv_obj_t *btn = lv_list_add_btn(wfList, LV_SYMBOL_WIFI, (*item).c_str());
    lv_obj_add_event_cb(btn, list_event_handler, LV_EVENT_CLICKED, NULL);
    delay(1);
  }

  foundNetworks = foundWifiList.size();
}

static void buildBody() {
  lv_obj_t *bodyScreen = lv_obj_create(lv_scr_act());
  lv_obj_add_style(bodyScreen, &border_style, 0);
  lv_obj_set_size(bodyScreen, gfx->width(), gfx->height() - 34);
  lv_obj_align(bodyScreen, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_t *label = lv_label_create(bodyScreen);
  lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
  lv_label_set_text(label, "");
  lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_size(label, 800, 60);
  lv_obj_set_pos(label, 0, 0);

  /*CREATE AN IMGBTN00*/
  lv_obj_t *imgbtn00 = lv_pictogram_create(bodyScreen, &eu, "Eu ", label);
  lv_obj_add_style(imgbtn00, &style_but, 0);
  lv_obj_add_style(imgbtn00, &style_pr, LV_STATE_PRESSED);
  lv_obj_align(imgbtn00, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_pos(imgbtn00, 10, 50);

  /*CREATE AN IMGBTN01*/
  lv_obj_t *imgbtn01 = lv_pictogram_create(bodyScreen, &querer, "Quero ", label);
  lv_obj_add_style(imgbtn01, &style_but, 0);
  lv_obj_add_style(imgbtn01, &style_pr, LV_STATE_PRESSED);
  lv_obj_set_pos(imgbtn01, 120, 50);

  /*CREATE AN IMGBTN02*/
  lv_obj_t *imgbtn02 = lv_pictogram_create(bodyScreen, &assistir, "Assistir ", label);
  lv_obj_add_style(imgbtn02, &style_but, 0);
  lv_obj_add_style(imgbtn02, &style_pr, LV_STATE_PRESSED);
  lv_obj_set_pos(imgbtn02, 230, 50);

  /*CREATE AN IMGBTN03*/
  lv_obj_t *imgbtn03 = lv_pictogram_create(bodyScreen, &beber, "Beber ", label);
  lv_obj_add_style(imgbtn03, &style_but, 0);
  lv_obj_add_style(imgbtn03, &style_pr, LV_STATE_PRESSED);
  lv_obj_set_pos(imgbtn03, 340, 50);

  // /*CREATE AN IMGBTN04*/
  lv_obj_t *imgbtn04 = lv_pictogram_create(bodyScreen, &entrar, "Entrar ", label);
  lv_obj_add_style(imgbtn04, &style_but, 0);
  lv_obj_add_style(imgbtn04, &style_pr, LV_STATE_PRESSED);
  lv_obj_set_pos(imgbtn04, 450, 50);

  /*CREATE AN IMGBTN10*/
  lv_obj_t *imgbtn10 = lv_pictogram_create(bodyScreen, &mae, "Mae ", label);
  lv_obj_add_style(imgbtn10, &style_but, 0);
  lv_obj_add_style(imgbtn10, &style_pr, LV_STATE_PRESSED);
  lv_obj_set_pos(imgbtn10, 10, 160);

  /*CREATE AN IMGBTN11*/
  lv_obj_t *imgbtn11 = lv_pictogram_create(bodyScreen, &brincar, "Brincar ", label);
  lv_obj_add_style(imgbtn11, &style_but, 0);
  lv_obj_add_style(imgbtn11, &style_pr, LV_STATE_PRESSED);
  lv_obj_set_pos(imgbtn11, 120, 160);

  /*CREATE AN IMGBTN12*/
  lv_obj_t *imgbtn12 = lv_pictogram_create(bodyScreen, &ajuda, "Ajuda ", label);
  lv_obj_add_style(imgbtn12, &style_but, 0);
  lv_obj_add_style(imgbtn12, &style_pr, LV_STATE_PRESSED);
  lv_obj_set_pos(imgbtn12, 230, 160);

  /*CREATE AN IMGBTN13*/
  lv_obj_t *imgbtn13 = lv_pictogram_create(bodyScreen, &passear, "Passear ", label);
  lv_obj_add_style(imgbtn13, &style_but, 0);
  lv_obj_add_style(imgbtn13, &style_pr, LV_STATE_PRESSED);
  lv_obj_set_pos(imgbtn13, 340, 160);

  /*CREATE AN IMGBTN14*/
  lv_obj_t *imgbtn14 = lv_pictogram_create(bodyScreen, &mae, "BOATAO10 ", label);
  lv_obj_add_style(imgbtn14, &style_but, 0);
  lv_obj_add_style(imgbtn14, &style_pr, LV_STATE_PRESSED);
  lv_obj_set_pos(imgbtn14, 450, 160);

  /*CREATE AN IMGBTN20*/
  lv_obj_t *imgbtn20 = lv_pictogram_create(bodyScreen, &voce, "Voce ", label);
  lv_obj_add_style(imgbtn20, &style_but, 0);
  lv_obj_add_style(imgbtn20, &style_pr, LV_STATE_PRESSED);
  lv_obj_set_pos(imgbtn20, 10, 270);

  /*CREATE AN IMGBTN21*/
  lv_obj_t *imgbtn21 = lv_pictogram_create(bodyScreen, &comer, "Comer ", label);
  lv_obj_add_style(imgbtn21, &style_but, 0);
  lv_obj_add_style(imgbtn21, &style_pr, LV_STATE_PRESSED);
  lv_obj_set_pos(imgbtn21, 120, 270);

  /*CREATE AN IMGBTN22*/
  lv_obj_t *imgbtn22 = lv_pictogram_create(bodyScreen, &pegar, "Pegar ", label);
  lv_obj_add_style(imgbtn22, &style_but, 0);
  lv_obj_add_style(imgbtn22, &style_pr, LV_STATE_PRESSED);
  lv_obj_set_pos(imgbtn22, 230, 270);

  /*CREATE AN IMGBTN23*/
  lv_obj_t *imgbtn23 = lv_pictogram_create(bodyScreen, &nao, "Nao ", label);
  lv_obj_add_style(imgbtn23, &style_but, 0);
  lv_obj_add_style(imgbtn23, &style_pr, LV_STATE_PRESSED);
  lv_obj_set_pos(imgbtn23, 340, 270);

  /*CREATE AN IMGBTN24*/
  lv_obj_t *imgbtn24 = lv_pictogram_create(bodyScreen, &comer, "BOTAO15 ", label);
  lv_obj_add_style(imgbtn24, &style_but, 0);
  lv_obj_add_style(imgbtn24, &style_pr, LV_STATE_PRESSED);
  lv_obj_set_pos(imgbtn24, 450, 270);

  /*CREATE AN IMGBTN30*/
  lv_obj_t *imgbtn30 = lv_pictogram_create(bodyScreen, &vovo, "Vovo ", label);
  lv_obj_add_style(imgbtn30, &style_but, 0);
  lv_obj_add_style(imgbtn30, &style_pr, LV_STATE_PRESSED);
  lv_obj_set_pos(imgbtn30, 10, 380);

  // /*CREATE AN IMGBTN31*/
  lv_obj_t *imgbtn31 = lv_pictogram_create(bodyScreen, &sair, "Sair ", label);
  lv_obj_add_style(imgbtn31, &style_but, 0);
  lv_obj_add_style(imgbtn31, &style_pr, LV_STATE_PRESSED);
  lv_obj_set_pos(imgbtn31, 120, 380);

  /*CREATE AN IMGBTN32*/
  lv_obj_t *imgbtn32 = lv_pictogram_create(bodyScreen, &vovo, "BOTAO18 ", label);
  lv_obj_add_style(imgbtn32, &style_but, 0);
  lv_obj_add_style(imgbtn32, &style_pr, LV_STATE_PRESSED);
  lv_obj_set_pos(imgbtn32, 230, 380);

  // /*CREATE AN IMGBTN33*/
  lv_obj_t *imgbtn33 = lv_pictogram_create(bodyScreen, &sim, "Sim ", label);
  lv_obj_add_style(imgbtn33, &style_but, 0);
  lv_obj_add_style(imgbtn33, &style_pr, LV_STATE_PRESSED);
  lv_obj_set_pos(imgbtn33, 340, 380);

  /*CREATE AN IMGBTN34*/
  lv_obj_t *imgbtn34 = lv_pictogram_create(bodyScreen, &vovo, "BOTAO20 ", label);
  lv_obj_add_style(imgbtn34, &style_but, 0);
  lv_obj_add_style(imgbtn34, &style_pr, LV_STATE_PRESSED);
  lv_obj_set_pos(imgbtn34, 450, 380);

  /*  CREATE PANEL FOR TARGETS */
  lv_obj_t *panel = lv_obj_create(bodyScreen);
  lv_obj_set_size(panel, 200, 400);
  lv_obj_set_scroll_snap_y(panel, LV_SCROLL_SNAP_START);
  lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
  lv_obj_align(panel, LV_ALIGN_RIGHT_MID, 0, 0);

  uint32_t i;
  String labels[10] = { "Agua", "Youtube", "Uva", "Bita", "Maca", "Netflix", "Fusca", "Pipoca", "Biscoito", "Pizza" };
  for (i = 0; i < 10; i++) {
    char lbl_text[10];
    lv_obj_t *btn = lv_btn_create(panel);
    lv_obj_add_event_cb(btn, target_handler, LV_EVENT_ALL, label);
    lv_obj_set_size(btn, 150, 135);
    lv_obj_set_pos(btn, 0, 30);
    lv_obj_add_style(btn, &style_but, 0);
    lv_obj_add_style(btn, &style_pr, LV_STATE_PRESSED);
    lv_obj_t *label = lv_label_create(btn);
    labels[i].toCharArray(lbl_text, 10);
    lv_label_set_text(label, lbl_text);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);
  }

  lv_obj_t *tgt_btn = lv_obj_get_child(panel, 0);
  put_image(tgt_btn, &agua);
  tgt_btn = lv_obj_get_child(panel, 1);
  put_image(tgt_btn, &youtube);
  tgt_btn = lv_obj_get_child(panel, 2);
  put_image(tgt_btn, &uva);
  tgt_btn = lv_obj_get_child(panel, 3);
  put_image(tgt_btn, &bita);
  tgt_btn = lv_obj_get_child(panel, 4);
  put_image(tgt_btn, &maca);
  tgt_btn = lv_obj_get_child(panel, 5);
  put_image(tgt_btn, &netflix);
  tgt_btn = lv_obj_get_child(panel, 6);
  put_image(tgt_btn, &fusca);
  tgt_btn = lv_obj_get_child(panel, 7);
  put_image(tgt_btn, &pipoca);
  tgt_btn = lv_obj_get_child(panel, 8);
  put_image(tgt_btn, &biscoito);
  tgt_btn = lv_obj_get_child(panel, 9);
  put_image(tgt_btn, &pizza);

  lv_obj_update_snap(panel, LV_ANIM_ON);
  lv_obj_set_pos(panel, 0, 10);
}

void put_image(lv_obj_t *tgt_btn, const void *img_src) {
  lv_obj_t *img = lv_img_create(tgt_btn);
  lv_img_set_src(img, img_src);
  lv_obj_align(img, LV_ALIGN_BOTTOM_MID, 0, 10);
}

static void target_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);

  lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
  lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);

  lv_obj_t *content = lv_obj_get_child(btn, 0);
  char *texto = lv_label_get_text(label);
  char final_texto[100];
  if (code == LV_EVENT_CLICKED) {
    long_text += lv_label_get_text(content);
    Serial.println(long_text);
    long_text.toCharArray(final_texto, 100);
    lv_label_set_text(timeLabel, final_texto);
    enviarNotificacao();
  }
  // long_text = "";
}

static void event_handler(lv_event_t *e) {

  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
  lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
  lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
  lv_obj_t *content = lv_obj_get_child(btn, 1);
  char *texto = lv_label_get_text(label);
  char final_texto[100];

  if (code == LV_EVENT_CLICKED) {
    long_text += lv_label_get_text(content);
    long_text.toCharArray(final_texto, 100);
    lv_label_set_text(timeLabel, final_texto);
    Serial.print(long_text);
  } else if (code == LV_EVENT_RELEASED) {
    LV_LOG_USER("Pressione um botão");
  }
}

static void list_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);

  if (code == LV_EVENT_CLICKED) {

    String selectedItem = String(lv_list_get_btn_text(wfList, obj));
    for (int i = 0; i < selectedItem.length() - 1; i++) {
      if (selectedItem.substring(i, i + 2) == " (") {
        ssidName = selectedItem.substring(0, i);
        lv_label_set_text_fmt(mboxTitle, "Selected WiFi SSID: %s", ssidName);
        lv_obj_move_foreground(mboxConnect);
        break;
      }
    }
  }
}

/*
 * NETWORK TASKS
 */

static void networkScanner() {
  xTaskCreate(scanWIFITask,
              "ScanWIFITask",
              4096,
              NULL,
              1,
              &ntScanTaskHandler);
}

static void networkConnector() {
  xTaskCreate(beginWIFITask,
              "beginWIFITask",
              2048,
              NULL,
              1,
              &ntConnectTaskHandler);
}

static void scanWIFITask(void *pvParameters) {
  while (1) {
    foundWifiList.clear();
    int n = WiFi.scanNetworks();
    vTaskDelay(10);
    for (int i = 0; i < n; ++i) {
      String item = WiFi.SSID(i) + " (" + WiFi.RSSI(i) + ") " + ((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
      foundWifiList.push_back(item);
      vTaskDelay(10);
    }
    vTaskDelay(5000);
  }
}

void beginWIFITask(void *pvParameters) {

  unsigned long startingTime = millis();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  vTaskDelay(100);

  WiFi.begin(ssidName.c_str(), psword.c_str());
  while (WiFi.status() != WL_CONNECTED && (millis() - startingTime) < networkTimeout) {
    vTaskDelay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    networkStatus = NETWORK_CONNECTED_POPUP;
    // saveWIFICredentialEEPROM(1, ssid + " " + psword);
  } else {
    networkStatus = NETWORK_CONNECT_FAILED;
    // saveWIFICredentialEEPROM(0, "");
  }

  vTaskDelete(NULL);
}

static void popupMsgBox(String title, String msg) {

  if (popupBox != NULL) {
    lv_obj_del(popupBox);
  }

  popupBox = lv_obj_create(screenMain);
  lv_obj_add_style(popupBox, &popupBox_style, 0);
  lv_obj_set_size(popupBox, gfx->width() * 2 / 3, gfx->height() / 2);
  lv_obj_center(popupBox);

  lv_obj_t *popupTitle = lv_label_create(popupBox);
  lv_label_set_text(popupTitle, title.c_str());
  lv_obj_set_width(popupTitle, gfx->width() * 2 / 3 - 50);
  lv_obj_align(popupTitle, LV_ALIGN_TOP_LEFT, 0, 0);

  lv_obj_t *popupMSG = lv_label_create(popupBox);
  lv_obj_set_width(popupMSG, gfx->width() * 2 / 3 - 50);
  lv_label_set_text(popupMSG, msg.c_str());
  lv_obj_align(popupMSG, LV_ALIGN_TOP_LEFT, 0, 40);

  popupBoxCloseBtn = lv_btn_create(popupBox);
  lv_obj_add_event_cb(popupBoxCloseBtn, btn_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_align(popupBoxCloseBtn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  lv_obj_t *btnLabel = lv_label_create(popupBoxCloseBtn);
  lv_label_set_text(btnLabel, "Ok");
  lv_obj_center(btnLabel);
}

void updateLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return;
  }

  char hourMin[6];
  strftime(hourMin, 6, "%H:%M", &timeinfo);
  String hourMinWithSymbol = String(hourMin);
  hourMinWithSymbol += "   ";
  hourMinWithSymbol += LV_SYMBOL_WIFI;
  lv_label_set_text(timeLabel, hourMinWithSymbol.c_str());
}
