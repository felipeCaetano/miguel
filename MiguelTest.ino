#include <WiFi.h>
/*******************************************************************************
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
#include <lvgl.h>

#define GFX_BL DF_GFX_BL  // default backlight pin, you may replace DF_GFX_BL to actual backlight pin
#define TFT_BL 2
/* More dev device declaration: https://github.com/moononournation/Arduino_GFX/wiki/Dev-Device-Declaration */
#if defined(DISPLAY_DEV_KIT)
Arduino_GFX *gfx = create_default_Arduino_GFX();
#else  /* !defined(DISPLAY_DEV_KIT) */

/* More data bus class: https://github.com/moononournation/Arduino_GFX/wiki/Data-Bus-Class */
//Arduino_DataBus *bus = create_default_Arduino_DataBus();

/* More display class: https://github.com/moononournation/Arduino_GFX/wiki/Display-Class */
//Arduino_GFX *gfx = new Arduino_ILI9341(bus, DF_GFX_RST, 0 /* rotation */, false /* IPS */);

Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
  GFX_NOT_DEFINED /* CS */, GFX_NOT_DEFINED /* SCK */, GFX_NOT_DEFINED /* SDA */,
  40 /* DE */, 41 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */,
  45 /* R0 */, 48 /* R1 */, 47 /* R2 */, 21 /* R3 */, 14 /* R4 */,
  5 /* G0 */, 6 /* G1 */, 7 /* G2 */, 15 /* G3 */, 16 /* G4 */, 4 /* G5 */,
  8 /* B0 */, 3 /* B1 */, 46 /* B2 */, 9 /* B3 */, 1 /* B4 */
);
// option 1:
// ILI6485 LCD 480x2720
// Arduino_RPi_DPI_RGBPanel *gfx = new Arduino_RPi_DPI_RGBPanel(
//   bus,
//   480 /* width */, 0 /* hsync_polarity */, 8 /* hsync_front_porch */, 4 /* hsync_pulse_width */, 43 /* hsync_back_porch */,
//   272 /* height */, 0 /* vsync_polarity */, 8 /* vsync_front_porch */, 4 /* vsync_pulse_width */, 12 /* vsync_back_porch */,
//   1 /* pclk_active_neg */, 9000000 /* prefer_speed */, true /* auto_flush */);

// option 2:
// ST7262 IPS LCD 800x480
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

// int numbtn = 20;
// lv_obj_t* imgbtn;
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


static void event_handler(lv_event_t *e) {

  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
  lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
  lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
  lv_obj_t *content = lv_obj_get_child(btn, 0);
  char *texto = lv_label_get_text(label);
  char long_text[2048];

  if (code == LV_EVENT_CLICKED) {
    strcpy(long_text, lv_label_get_text(content));

    lv_label_set_text(label, long_text);
  } else if (code == LV_EVENT_RELEASED) {
    LV_LOG_USER("Pressione um botão");
  }
}

const char* ssid     = "chesf-movel";
const char* password = "Helena_2";

void setup(void) {
  Serial.begin(921600);
  // Serial.println("LVGL Hello World");

  touch_init();
  gfx->begin();

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
    Serial.println("LVGL draw_buf allocate failed!");
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

    // /*Create a Simple Label*/
    //   lv_obj_t *label = lv_label_create(lv_scr_act());
    //   lv_label_set_text(label, "Hello ESP32");
    //   lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    /*WiFi initialization*/
    // Serial.println("Connecting");
    WiFi.begin(ssid, password);

    // while (WiFi.status() != WL_CONNECTED) {
    //   delay(500);
    //   Serial.print(".");
    // }
    // Serial.println("");
    // Serial.print("Connected to WiFi network with IP Address: ");
    // Serial.println(WiFi.localIP());

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
    // LV_IMG_DECLARE(passear);
    // LV_IMG_DECLARE(pegar);
    LV_IMG_DECLARE(querer);
    LV_IMG_DECLARE(entrar);
    LV_IMG_DECLARE(sair);
    LV_IMG_DECLARE(sim);

    /*Create a transition animation on width transformation and recolor.*/
    // static lv_style_prop_t tr_prop[] = {LV_STYLE_TRANSFORM_WIDTH, LV_STYLE_IMG_RECOLOR_OPA, 0};
    // static lv_style_transition_dsc_t tr;
    // lv_style_transition_dsc_init(&tr, tr_prop, lv_anim_path_linear, 200, 0, NULL);

    static lv_style_t style_def;
    lv_style_init(&style_def);
    lv_style_set_text_color(&style_def, lv_color_hex(0x001010));
    //lv_style_set_text_opa(&style_def, LV_OPA_0);
    //lv_style_set_transition(&style_def, &tr);

    /*Darken the button when pressed and make it wider*/
    static lv_style_t style_pr;
    lv_style_init(&style_pr);
    lv_style_set_img_recolor_opa(&style_pr, LV_OPA_30);
    lv_style_set_img_recolor(&style_pr, lv_color_black());
    lv_style_set_transform_width(&style_pr, 5);

    /*Add border to the bottom+right*/
    static lv_style_t style;
    lv_style_set_border_color(&style, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_border_width(&style, 5);
    lv_style_set_border_opa(&style, LV_OPA_50);
    lv_style_set_border_side(&style, LV_BORDER_SIDE_BOTTOM | LV_BORDER_SIDE_RIGHT | LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_TOP);

    /*CREATE SCREEN OBJECTS*/
    lv_obj_t *screenMain = lv_obj_create(NULL);
    lv_obj_t *label = lv_label_create(screenMain);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
    lv_label_set_text(label, "");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);
    //lv_obj_set_size(label, 800, 60);
    lv_obj_set_pos(label, 0, 0);

    // Define
    //const char *str = WiFi.localIP().toString().c_str();
    // Length (with one extra character for the null terminator)
    //int str_len = str.length() + 1;
    // Prepare the character array (the buffer)
    //char char_array[str_len];
    // Copy it over
    //str.toCharArray(char_array, str_len);

    // lv_obj_t *label = lv_label_create(lv_scr_act());
    // lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    // if (str != NULL) {
    //   lv_label_set_text(label, "Conectado!");
    // }
    // lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);
    //       lv_obj_set_size(label, 250, 60);
    //       lv_obj_set_pos(label, 0, 0);

    // //       /*CREATE AN IMGBTN00*/
    lv_obj_t *imgbtn00 = lv_imgbtn_create(screenMain);
    lv_imgbtn_set_src(imgbtn00, LV_IMGBTN_STATE_RELEASED, NULL, &eu, NULL);
    lv_obj_add_event_cb(imgbtn00, event_handler, LV_EVENT_ALL, label);
    lv_obj_add_style(imgbtn00, &style_def, 0);
    lv_obj_add_style(imgbtn00, &style_pr, LV_STATE_PRESSED);
    static lv_style_t style_but;
    lv_style_init(&style_but);
    lv_style_set_text_color(&style_but, lv_color_black());
    lv_obj_add_style(imgbtn00, &style, 0);

    lv_obj_set_size(imgbtn00, 100, 100);
    lv_obj_align(imgbtn00, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_pos(imgbtn00, 10, 50);

    lv_obj_t *labelbtn00 = lv_label_create(imgbtn00);
    lv_label_set_text(labelbtn00, "Eu ");
    lv_obj_align(labelbtn00, LV_ALIGN_TOP_MID, 0, -4);

    /*CREATE AN IMGBTN01*/
    lv_obj_t *imgbtn01 = lv_imgbtn_create(screenMain);
    lv_imgbtn_set_src(imgbtn01, LV_IMGBTN_STATE_RELEASED, NULL, &querer, NULL);
    lv_obj_add_event_cb(imgbtn01, event_handler, LV_EVENT_ALL, label);
    lv_obj_add_style(imgbtn01, &style_def, 0);
    lv_obj_add_style(imgbtn01, &style_pr, LV_STATE_PRESSED);
  
    lv_style_set_text_color(&style_but, lv_color_black());
    lv_obj_add_style(imgbtn01, &style, 0);
    lv_obj_set_size(imgbtn01, 100, 100);
    lv_obj_set_pos(imgbtn01, 120, 50);

    lv_obj_t *labelbtn01 = lv_label_create(imgbtn01);
    lv_label_set_text(labelbtn01, "Querer");
    lv_obj_align(labelbtn01, LV_ALIGN_TOP_MID, 0, -4);

    /*CREATE AN IMGBTN02*/
    lv_obj_t *imgbtn02 = lv_imgbtn_create(screenMain);
    lv_imgbtn_set_src(imgbtn02, LV_IMGBTN_STATE_RELEASED, NULL, &assistir, NULL);

    lv_obj_add_event_cb(imgbtn02, event_handler, LV_EVENT_ALL, label);
    lv_obj_add_style(imgbtn02, &style_def, 0);
    lv_obj_add_style(imgbtn02, &style_pr, LV_STATE_PRESSED);

    lv_style_set_text_color(&style_but, lv_color_black());
    lv_obj_add_style(imgbtn02, &style, 0);        

    lv_obj_set_size(imgbtn02, 100, 100);
    lv_obj_set_pos(imgbtn02, 230, 50);

    lv_obj_t *labelbtn02 = lv_label_create(imgbtn02);
    lv_label_set_text(labelbtn02, "Assistir");
    lv_obj_align(labelbtn02, LV_ALIGN_TOP_MID, 0, -4);

    /*CREATE AN IMGBTN03*/
    lv_obj_t *imgbtn03 = lv_imgbtn_create(screenMain);
    lv_imgbtn_set_src(imgbtn03, LV_IMGBTN_STATE_RELEASED, NULL, &beber, NULL);
    lv_obj_add_event_cb(imgbtn03, event_handler, LV_EVENT_ALL, label);
    lv_obj_add_style(imgbtn03, &style_def, 0);
    lv_obj_add_style(imgbtn03, &style_pr, LV_STATE_PRESSED);

    lv_style_set_text_color(&style_but, lv_color_black());
    lv_obj_add_style(imgbtn03, &style, 0);
    lv_obj_set_size(imgbtn03, 100, 100);
    lv_obj_set_pos(imgbtn03, 340, 50);

    lv_obj_t *labelbtn03 = lv_label_create(imgbtn03);
    lv_label_set_text(labelbtn03, "Beber");
    lv_obj_align(labelbtn03, LV_ALIGN_TOP_MID, 0, -4);

    /*CREATE AN IMGBTN04*/
    lv_obj_t *imgbtn04 = lv_imgbtn_create(screenMain);
    lv_imgbtn_set_src(imgbtn04, LV_IMGBTN_STATE_RELEASED, NULL, &entrar, NULL);
    lv_obj_add_event_cb(imgbtn04, event_handler, LV_EVENT_ALL, label);
    lv_obj_add_style(imgbtn04, &style_def, 0);
    lv_obj_add_style(imgbtn04, &style_pr, LV_STATE_PRESSED);
    lv_style_set_text_color(&style_but, lv_color_black());
    lv_obj_add_style(imgbtn04, &style, 0);
    lv_obj_set_size(imgbtn04, 100, 100);
    lv_obj_set_pos(imgbtn04, 450, 50);

    lv_obj_t *labelbtn04 = lv_label_create(imgbtn04);
    lv_label_set_text(labelbtn04, "Entrar");
    lv_obj_align(labelbtn04, LV_ALIGN_TOP_MID, 0, -4);

    /*CREATE AN IMGBTN10*/
    lv_obj_t *imgbtn10 = lv_imgbtn_create(screenMain);
    lv_imgbtn_set_src(imgbtn10, LV_IMGBTN_STATE_RELEASED, NULL, &mae, NULL);
    lv_obj_add_event_cb(imgbtn10, event_handler, LV_EVENT_ALL, label);
    lv_obj_add_style(imgbtn10, &style_def, 0);
    lv_obj_add_style(imgbtn10, &style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(imgbtn10, &style, 0);
    lv_obj_set_size(imgbtn10, 100, 100);
    lv_obj_set_pos(imgbtn10, 10, 160);

    lv_obj_t *labelbtn10 = lv_label_create(imgbtn10);
    lv_label_set_text(labelbtn10, "Mae");
    lv_obj_align(labelbtn10, LV_ALIGN_TOP_MID, 0, -4);

    /*CREATE AN IMGBTN11*/
    lv_obj_t *imgbtn11 = lv_btn_create(screenMain);
    //lv_imgbtn_set_src(imgbtn11, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
    lv_obj_set_size(imgbtn11, 100, 100);
    lv_obj_set_pos(imgbtn11, 120, 160);

    lv_obj_t *labelbtn11 = lv_label_create(imgbtn11);
    lv_label_set_text(labelbtn11, "BOTAO7");
    lv_obj_align(labelbtn11, LV_ALIGN_TOP_MID, 0, -4);

    /*CREATE AN IMGBTN12*/
    lv_obj_t *imgbtn12 = lv_btn_create(screenMain);
    //lv_imgbtn_set_src(imgbtn12, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
    lv_obj_set_size(imgbtn12, 100, 100);
    lv_obj_set_pos(imgbtn12, 230, 160);

    lv_obj_t *labelbtn12 = lv_label_create(imgbtn12);
    lv_label_set_text(labelbtn12, "BOTAO8");
    lv_obj_align(labelbtn12, LV_ALIGN_TOP_MID, 0, -4);

    /*CREATE AN IMGBTN13*/
    lv_obj_t *imgbtn13 = lv_btn_create(screenMain);
    //lv_imgbtn_set_src(imgbtn11, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
    lv_obj_set_size(imgbtn13, 100, 100);
    lv_obj_set_pos(imgbtn13, 340, 160);

    lv_obj_t *labelbtn13 = lv_label_create(imgbtn13);
    lv_label_set_text(labelbtn13, "BOTAO9");
    lv_obj_align(labelbtn13, LV_ALIGN_TOP_MID, 0, -4);

    /*CREATE AN IMGBTN14*/
    lv_obj_t *imgbtn14 = lv_btn_create(screenMain);
    //lv_imgbtn_set_src(imgbtn11, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
    lv_obj_set_size(imgbtn14, 100, 100);
    lv_obj_set_pos(imgbtn14, 450, 160);

    lv_obj_t *labelbtn14 = lv_label_create(imgbtn14);
    lv_label_set_text(labelbtn14, "BOTAO10");
    lv_obj_align(labelbtn14, LV_ALIGN_TOP_MID, 0, -4);

    /*CREATE AN IMGBTN20*/
    lv_obj_t *imgbtn20 = lv_imgbtn_create(screenMain);
    lv_imgbtn_set_src(imgbtn20, LV_IMGBTN_STATE_RELEASED, NULL, &voce, NULL);
    lv_obj_add_event_cb(imgbtn20, event_handler, LV_EVENT_ALL, label);
    lv_obj_add_style(imgbtn20, &style_def, 0);
    lv_obj_add_style(imgbtn20, &style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(imgbtn20, &style, 0);
    lv_obj_set_size(imgbtn20, 100, 100);
    lv_obj_set_pos(imgbtn20, 10, 270);

    lv_obj_t *labelbtn20 = lv_label_create(imgbtn20);
    lv_label_set_text(labelbtn20, "Voce");
    lv_obj_align(labelbtn20, LV_ALIGN_TOP_MID, 0, -4);

    /*CREATE AN IMGBTN21*/
    lv_obj_t *imgbtn21 = lv_imgbtn_create(screenMain);
    lv_imgbtn_set_src(imgbtn21, LV_IMGBTN_STATE_RELEASED, NULL, &comer, NULL);
    lv_obj_add_event_cb(imgbtn21, event_handler, LV_EVENT_ALL, label);
    lv_obj_add_style(imgbtn21, &style_def, 0);
    lv_obj_add_style(imgbtn21, &style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(imgbtn21, &style, 0);
    lv_obj_set_size(imgbtn21, 100, 100);
    lv_obj_set_pos(imgbtn21, 120, 270);

    lv_obj_t *labelbtn21 = lv_label_create(imgbtn21);
    lv_label_set_text(labelbtn21, "Comer");
    lv_obj_align(labelbtn21, LV_ALIGN_TOP_MID, 0, -4);

    /*CREATE AN IMGBTN22*/
    lv_obj_t *imgbtn22 = lv_btn_create(screenMain);
    //lv_imgbtn_set_src(imgbtn22, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
    lv_obj_set_size(imgbtn22, 100, 100);
    lv_obj_set_pos(imgbtn22, 230, 270);

    lv_obj_t *labelbtn22 = lv_label_create(imgbtn22);
    lv_label_set_text(labelbtn22, "BOTAO13");
    lv_obj_align(labelbtn22, LV_ALIGN_TOP_MID, 0, -4);

    /*CREATE AN IMGBTN23*/
    lv_obj_t *imgbtn23 = lv_imgbtn_create(screenMain);
    lv_imgbtn_set_src(imgbtn23, LV_IMGBTN_STATE_RELEASED, NULL, &nao, NULL);
    lv_obj_add_event_cb(imgbtn23, event_handler, LV_EVENT_ALL, label);
    lv_obj_add_style(imgbtn23, &style_def, 0);
    lv_obj_add_style(imgbtn23, &style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(imgbtn23, &style, 0);
    lv_obj_set_size(imgbtn23, 100, 100);
    lv_obj_set_pos(imgbtn23, 340, 270);

    lv_obj_t *labelbtn23 = lv_label_create(imgbtn23);
    lv_label_set_text(labelbtn23, "Nao");
    lv_obj_align(labelbtn23, LV_ALIGN_TOP_MID, 0, -4);

    /*CREATE AN IMGBTN24*/
    lv_obj_t *imgbtn24 = lv_btn_create(screenMain);
    //lv_imgbtn_set_src(imgbtn24, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
    lv_obj_set_size(imgbtn24, 100, 100);
    lv_obj_set_pos(imgbtn24, 450, 270);

    lv_obj_t *labelbtn24 = lv_label_create(imgbtn24);
    lv_label_set_text(labelbtn24, "BOTAO15");
    lv_obj_align(labelbtn24, LV_ALIGN_TOP_MID, 0, -4);

    /*CREATE AN IMGBTN30*/
    lv_obj_t *imgbtn30 = lv_imgbtn_create(screenMain);
    lv_imgbtn_set_src(imgbtn30, LV_IMGBTN_STATE_RELEASED, NULL, &vovo, NULL);
    lv_obj_add_event_cb(imgbtn30, event_handler, LV_EVENT_ALL, label);
    lv_obj_add_style(imgbtn30, &style_def, 0);
    lv_obj_add_style(imgbtn30, &style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(imgbtn30, &style, 0);

    lv_obj_set_size(imgbtn30, 100, 100);
    lv_obj_set_pos(imgbtn30, 10, 380);

    lv_obj_t *labelbtn30 = lv_label_create(imgbtn30);
    lv_label_set_text(labelbtn30, " Vovo");
    lv_obj_align(labelbtn30, LV_ALIGN_TOP_MID, 0, -4);

    /*CREATE AN IMGBTN31*/
    lv_obj_t *imgbtn31 = lv_btn_create(screenMain);
    lv_imgbtn_set_src(imgbtn31, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
    lv_obj_add_event_cb(imgbtn31, event_handler, LV_EVENT_ALL, label);
    lv_obj_add_style(imgbtn31, &style_def, 0);
    lv_obj_add_style(imgbtn31, &style_pr, LV_STATE_PRESSED);
    lv_style_set_text_color(&style_but, lv_color_black());
    lv_obj_add_style(imgbtn31, &style, 0);
    lv_obj_set_size(imgbtn31, 100, 100);
    lv_obj_set_pos(imgbtn31, 450, 50);

    lv_obj_t *labelbtn04 = lv_label_create(imgbtn31);
    lv_label_set_text(labelbtn04, "Sair");
    lv_obj_set_size(imgbtn31, 100, 100);
    lv_obj_set_pos(imgbtn31, 120, 380);

    lv_obj_t *labelbtn31 = lv_label_create(imgbtn31);
    lv_label_set_text(labelbtn31, "BOTAO17");
    lv_obj_align(labelbtn31, LV_ALIGN_TOP_MID, 0, -4);

    /*CREATE AN IMGBTN32*/
    lv_obj_t *imgbtn32 = lv_btn_create(screenMain);
    //lv_imgbtn_set_src(imgbtn32, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
    lv_obj_set_size(imgbtn32, 100, 100);
    lv_obj_set_pos(imgbtn32, 230, 380);

    lv_obj_t *labelbtn32 = lv_label_create(imgbtn32);
    lv_label_set_text(labelbtn32, "BOTAO18");
    lv_obj_align(labelbtn32, LV_ALIGN_TOP_MID, 0, -4);

    /*CREATE AN IMGBTN33*/
    lv_obj_t *imgbtn33 = lv_imgbtn_create(screenMain);
    lv_imgbtn_set_src(imgbtn33, LV_IMGBTN_STATE_RELEASED, NULL, &sim, NULL);
    lv_obj_add_event_cb(imgbtn33, event_handler, LV_EVENT_ALL, label);
    lv_obj_add_style(imgbtn33, &style_def, 0);
    lv_obj_add_style(imgbtn33, &style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(imgbtn33, &style, 0);
    lv_obj_set_size(imgbtn33, 100, 100);
    lv_obj_set_pos(imgbtn33, 340, 380);

    lv_obj_t *labelbtn33 = lv_label_create(imgbtn33);
    lv_label_set_text(labelbtn33, "Sim");
    lv_obj_align(labelbtn33, LV_ALIGN_TOP_MID, 0, -4);

    /*CREATE AN IMGBTN34*/
    lv_obj_t *imgbtn34 = lv_btn_create(screenMain);
    //lv_imgbtn_set_src(imgbtn24, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
    lv_obj_set_size(imgbtn34, 100, 100);
    lv_obj_set_pos(imgbtn34, 450, 380);

    lv_obj_t *labelbtn34 = lv_label_create(imgbtn34);
    lv_label_set_text(labelbtn34, "BOTAO20");
    lv_obj_align(labelbtn34, LV_ALIGN_TOP_MID, 0, -4);

    /*  CREATE PANEL FOR TARGETS */
    lv_obj_t *panel = lv_obj_create(screenMain);
    lv_obj_set_size(panel, 200, 400);
    lv_obj_set_scroll_snap_y(panel, LV_SCROLL_SNAP_START);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_align(panel, LV_ALIGN_RIGHT_MID, 0, 0);

    uint32_t i;
    for (i = 0; i < 10; i++) {
      lv_obj_t *btn = lv_btn_create(panel);
      lv_obj_set_size(btn, 150, 135);
      lv_obj_set_pos(btn, 0, 30);

      lv_obj_t *label = lv_label_create(btn);
      lv_label_set_text_fmt(label, "Panel %" LV_PRIu32, i);

      lv_obj_center(label);
    }
    lv_obj_update_snap(panel, LV_ANIM_ON);

    lv_obj_set_pos(panel, 0, 10);

    lv_scr_load(screenMain);

    // Serial.println("Setup Done");
  }
}

void loop() {

  lv_timer_handler();
  delay(15);
  //  if(touch_has_signal())
  //  {
  //     if(touch_touched())
  //     {
  //       int x = touch_last_x;
  //       int y = touch_last_y;
  //       gfx->setCursor(x, y + 10);
  //       gfx->setTextColor(WHITE);
  //       gfx->setTextSize(1);
  //       gfx->printf("TOQUE EM: %d, %d;\n", x, y);
  //       //gfx->printf("TOQUE EM: %d;\n", y);
  //     }
  //  }
}