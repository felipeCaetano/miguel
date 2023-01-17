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
static void target_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
  lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
  lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
  lv_obj_t *content = lv_obj_get_child(btn, 0);
  char *texto = lv_label_get_text(label);
  if (code == LV_EVENT_CLICKED) {

    long_text += lv_label_get_text(content);
    Serial.println(long_text);
    enviarNotificacao();
  }
  long_text = "";
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
    // strcpy(long_text, lv_label_get_text(content));

    long_text += lv_label_get_text(content);
    //strcpy(final_texto, long_text);
    long_text.toCharArray(final_texto, 100);
    lv_label_set_text(label, final_texto);
    Serial.println(long_text);
  } else if (code == LV_EVENT_RELEASED) {
    LV_LOG_USER("Pressione um botão");
  }
}

void enviarNotificacao() {
  Serial.println("Enviar Notificação");
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;
  char send_txt[1024];
  if (long_text.length() > 1) {
    Serial.println(long_text);
    snprintf(send_txt, 600,
             "Uma Nova Mensagem!<br>\
     <p>Em: %d: %d: %d - %s</p>",
             hr, min, sec, long_text);
    server.send(200, "text/html", send_txt);
  } else {
    server.send(200, "text/html", "Sem Novas Mensagens.");
  }
}

/*
WIFI initialization:
Wifi credentials:
*/
const char *ssid = "POCO X3 Pro";
const char *password = "12345678";

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

    /*WiFi initialization*/
    WiFi.begin(ssid, password);

    if (MDNS.begin("miguelaac")) {
      Serial.println("MDNS responder started");
    }

    server.on("/", handleRoot);
    server.on("/notificacao", enviarNotificacao);
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("HTTP server started");

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

    /*Darken the button when pressed and make it wider*/
    static lv_style_t style_pr;
    lv_style_init(&style_pr);
    lv_style_set_img_recolor_opa(&style_pr, LV_OPA_30);
    lv_style_set_img_recolor(&style_pr, lv_color_black());
    lv_style_set_transform_width(&style_pr, 5);

    /*CREATE SCREEN OBJECTS*/
    lv_obj_t *screenMain = lv_obj_create(NULL);
    lv_obj_t *label = lv_label_create(screenMain);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
    lv_label_set_text(label, "");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);
    //lv_obj_set_size(label, 800, 60);
    lv_obj_set_pos(label, 0, 0);

    static lv_style_t style_but;
    lv_style_init(&style_but);
    lv_style_set_bg_color(&style_but, lv_color_white());
    lv_style_set_border_color(&style_but, lv_palette_darken(LV_PALETTE_LIGHT_BLUE, 3));
    lv_style_set_border_width(&style_but, 3);
    lv_style_set_border_opa(&style_but, LV_OPA_50);
    lv_style_set_shadow_width(&style_but, 10);
    lv_style_set_text_color(&style_but, lv_color_black());

    /*CREATE AN IMGBTN00*/
    lv_obj_t *imgbtn00 = lv_pictogram_create(screenMain, &eu, "Eu ", label);
    lv_obj_add_style(imgbtn00, &style_but, 0);
    lv_obj_add_style(imgbtn00, &style_pr, LV_STATE_PRESSED);
    lv_obj_align(imgbtn00, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(imgbtn00, 10, 50);

    /*CREATE AN IMGBTN01*/
    lv_obj_t *imgbtn01 = lv_pictogram_create(screenMain, &querer, "Quero ", label);
    lv_obj_add_style(imgbtn01, &style_but, 0);
    lv_obj_add_style(imgbtn01, &style_pr, LV_STATE_PRESSED);
    lv_obj_set_pos(imgbtn01, 120, 50);

    /*CREATE AN IMGBTN02*/
    lv_obj_t *imgbtn02 = lv_pictogram_create(screenMain, &assistir, "Assistir ", label);
    lv_obj_add_style(imgbtn02, &style_but, 0);
    lv_obj_add_style(imgbtn02, &style_pr, LV_STATE_PRESSED);
    lv_obj_set_pos(imgbtn02, 230, 50);

    /*CREATE AN IMGBTN03*/
    lv_obj_t *imgbtn03 = lv_pictogram_create(screenMain, &beber, "Beber ", label);
    lv_obj_add_style(imgbtn03, &style_but, 0);
    lv_obj_add_style(imgbtn03, &style_pr, LV_STATE_PRESSED);
    lv_obj_set_pos(imgbtn03, 340, 50);

    // /*CREATE AN IMGBTN04*/
    lv_obj_t *imgbtn04 = lv_pictogram_create(screenMain, &entrar, "Entrar ", label);
    lv_obj_add_style(imgbtn04, &style_but, 0);
    lv_obj_add_style(imgbtn04, &style_pr, LV_STATE_PRESSED);
    lv_obj_set_pos(imgbtn04, 450, 50);

    /*CREATE AN IMGBTN10*/
    lv_obj_t *imgbtn10 = lv_pictogram_create(screenMain, &mae, "Mae ", label);
    lv_obj_add_style(imgbtn10, &style_but, 0);
    lv_obj_add_style(imgbtn10, &style_pr, LV_STATE_PRESSED);
    lv_obj_set_pos(imgbtn10, 10, 160);

    /*CREATE AN IMGBTN11*/
    lv_obj_t *imgbtn11 = lv_pictogram_create(screenMain, &brincar, "Brincar ", label);
    lv_obj_add_style(imgbtn11, &style_but, 0);
    lv_obj_add_style(imgbtn11, &style_pr, LV_STATE_PRESSED);
    lv_obj_set_pos(imgbtn11, 120, 160);

    /*CREATE AN IMGBTN12*/
    lv_obj_t *imgbtn12 = lv_pictogram_create(screenMain, &ajuda, "Ajuda ", label);
    lv_obj_add_style(imgbtn12, &style_but, 0);
    lv_obj_add_style(imgbtn12, &style_pr, LV_STATE_PRESSED);
    lv_obj_set_pos(imgbtn12, 230, 160);

    /*CREATE AN IMGBTN13*/
    lv_obj_t *imgbtn13 = lv_pictogram_create(screenMain, &passear, "Passear ", label);
    lv_obj_add_style(imgbtn13, &style_but, 0);
    lv_obj_add_style(imgbtn13, &style_pr, LV_STATE_PRESSED);
    lv_obj_set_pos(imgbtn13, 340, 160);

    /*CREATE AN IMGBTN14*/
    lv_obj_t *imgbtn14 = lv_pictogram_create(screenMain, &mae, "BOATAO10 ", label);
    lv_obj_add_style(imgbtn14, &style_but, 0);
    lv_obj_add_style(imgbtn14, &style_pr, LV_STATE_PRESSED);
    lv_obj_set_pos(imgbtn14, 450, 160);

    /*CREATE AN IMGBTN20*/
    lv_obj_t *imgbtn20 = lv_pictogram_create(screenMain, &voce, "Voce ", label);
    lv_obj_add_style(imgbtn20, &style_but, 0);
    lv_obj_add_style(imgbtn20, &style_pr, LV_STATE_PRESSED);
    lv_obj_set_pos(imgbtn20, 10, 270);

    /*CREATE AN IMGBTN21*/
    lv_obj_t *imgbtn21 = lv_pictogram_create(screenMain, &comer, "Comer ", label);
    lv_obj_add_style(imgbtn21, &style_but, 0);
    lv_obj_add_style(imgbtn21, &style_pr, LV_STATE_PRESSED);
    lv_obj_set_pos(imgbtn21, 120, 270);

    /*CREATE AN IMGBTN22*/
    lv_obj_t *imgbtn22 = lv_pictogram_create(screenMain, &pegar, "Pegar ", label);
    lv_obj_add_style(imgbtn22, &style_but, 0);
    lv_obj_add_style(imgbtn22, &style_pr, LV_STATE_PRESSED);
    lv_obj_set_pos(imgbtn22, 230, 270);

    /*CREATE AN IMGBTN23*/
    lv_obj_t *imgbtn23 = lv_pictogram_create(screenMain, &nao, "Nao ", label);
    lv_obj_add_style(imgbtn23, &style_but, 0);
    lv_obj_add_style(imgbtn23, &style_pr, LV_STATE_PRESSED);
    lv_obj_set_pos(imgbtn23, 340, 270);

    /*CREATE AN IMGBTN24*/
    lv_obj_t *imgbtn24 = lv_pictogram_create(screenMain, &comer, "BOTAO15 ", label);
    lv_obj_add_style(imgbtn24, &style_but, 0);
    lv_obj_add_style(imgbtn24, &style_pr, LV_STATE_PRESSED);
    lv_obj_set_pos(imgbtn24, 450, 270);

    /*CREATE AN IMGBTN30*/
    lv_obj_t *imgbtn30 = lv_pictogram_create(screenMain, &vovo, "Vovo ", label);
    lv_obj_add_style(imgbtn30, &style_but, 0);
    lv_obj_add_style(imgbtn30, &style_pr, LV_STATE_PRESSED);
    lv_obj_set_pos(imgbtn30, 10, 380);

    // /*CREATE AN IMGBTN31*/
    lv_obj_t *imgbtn31 = lv_pictogram_create(screenMain, &sair, "Sair ", label);
    lv_obj_add_style(imgbtn31, &style_but, 0);
    lv_obj_add_style(imgbtn31, &style_pr, LV_STATE_PRESSED);
    lv_obj_set_pos(imgbtn31, 120, 380);

    /*CREATE AN IMGBTN32*/
    lv_obj_t *imgbtn32 = lv_pictogram_create(screenMain, &vovo, "BOTAO18 ", label);
    lv_obj_add_style(imgbtn32, &style_but, 0);
    lv_obj_add_style(imgbtn32, &style_pr, LV_STATE_PRESSED);
    lv_obj_set_pos(imgbtn32, 230, 380);

    // /*CREATE AN IMGBTN33*/
    lv_obj_t *imgbtn33 = lv_pictogram_create(screenMain, &sim, "Sim ", label);
    lv_obj_add_style(imgbtn33, &style_but, 0);
    lv_obj_add_style(imgbtn33, &style_pr, LV_STATE_PRESSED);
    lv_obj_set_pos(imgbtn33, 340, 380);

    /*CREATE AN IMGBTN34*/
    lv_obj_t *imgbtn34 = lv_pictogram_create(screenMain, &vovo, "BOTAO20 ", label);
    lv_obj_add_style(imgbtn34, &style_but, 0);
    lv_obj_add_style(imgbtn34, &style_pr, LV_STATE_PRESSED);
    lv_obj_set_pos(imgbtn34, 450, 380);

    /*  CREATE PANEL FOR TARGETS */
    lv_obj_t *panel = lv_obj_create(screenMain);
    lv_obj_set_size(panel, 200, 400);
    lv_obj_set_scroll_snap_y(panel, LV_SCROLL_SNAP_START);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_align(panel, LV_ALIGN_RIGHT_MID, 0, 0);

    uint32_t i;
    for (i = 0; i < 10; i++) {
      lv_obj_t *btn = lv_btn_create(panel);
      lv_obj_add_event_cb(btn, target_handler, LV_EVENT_ALL, label);
      lv_obj_set_size(btn, 150, 135);
      lv_obj_set_pos(btn, 0, 30);

      lv_obj_t *label = lv_label_create(btn);
      lv_label_set_text_fmt(label, "Panel %" LV_PRIu32, i);
      lv_obj_center(label);
    }
    lv_obj_update_snap(panel, LV_ANIM_ON);
    lv_obj_set_pos(panel, 0, 10);

    lv_scr_load(screenMain);
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