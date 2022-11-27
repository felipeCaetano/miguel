/*******************************************************************************
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
#include <lvgl.h>

#define GFX_BL DF_GFX_BL // default backlight pin, you may replace DF_GFX_BL to actual backlight pin
#define TFT_BL 2
/* More dev device declaration: https://github.com/moononournation/Arduino_GFX/wiki/Dev-Device-Declaration */
#if defined(DISPLAY_DEV_KIT)
Arduino_GFX *gfx = create_default_Arduino_GFX();
#else /* !defined(DISPLAY_DEV_KIT) */

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
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 +1);
  uint32_t h = (area->y2 - area->y1 +1);
  #if (LV_COLOR_SWAP != 0)
    gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
  #else
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
  #endif

  lv_disp_flush_ready(disp);
}

void my_touch_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
  if (touch_has_signal())
  {
    if (touch_touched())
    {
      data->state = LV_INDEV_STATE_PR;

      /*Set the coordinates*/
      data->point.x = touch_last_x;
      data->point.y = touch_last_y;
    }
    else if (touch_released())
    {
      data->state = LV_INDEV_STATE_REL;
    }
  }
  else
  {
    data->state = LV_INDEV_STATE_REL;
  }
}


void setup(void)
{
    Serial.begin(115200);
    Serial.println("LVGL Hello World");
      
    touch_init();
    gfx->begin();
    gfx->fillScreen(BLACK);
    delay(2000);
    gfx->fillScreen(WHITE);
    delay(2000);
    gfx->fillScreen(BLACK);   

    #ifdef TFT_BL
      pinMode(TFT_BL, OUTPUT);
      digitalWrite(TFT_BL, HIGH);
    #endif    
     
    lv_init();

    #ifdef ESP32
      disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t)*screenWidth * 10, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    #else
      disp_draw_buf = (lv_color_t *)malloc(sizeof(lv_color_t)*screenWidth * 10);
    #endif
      if (!disp_draw_buf)     
      {
        Serial.println("LVGL draw_buf allocate failed!");
      }
      else
      {
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

      /*CREATE SCREEN OBJECTS*/
      lv_obj_t *screenMain = lv_obj_create(NULL);
      lv_obj_t *label = lv_label_create(screenMain);
      lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
      lv_label_set_text(label, "Press a Button");
      lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);
      lv_obj_set_size(label, 250, 60);
      lv_obj_set_pos(label, 0, 0);

//       /*CREATE AN IMGBTN00*/
      lv_obj_t *imgbtn00 = lv_btn_create(screenMain);
      //lv_imgbtn_set_src(imgbtn00, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
      lv_obj_set_size(imgbtn00, 100, 100);
      lv_obj_align(imgbtn00, LV_ALIGN_TOP_LEFT, 0, 0);
      lv_obj_set_pos(imgbtn00, 10, 50);

      lv_obj_t *labelbtn00 = lv_label_create(imgbtn00);
      lv_label_set_text(labelbtn00, "BOTAO1");
      lv_obj_align(labelbtn00, LV_ALIGN_CENTER, 0, -4);

       /*CREATE AN IMGBTN01*/
      lv_obj_t *imgbtn01 = lv_btn_create(screenMain);
      //lv_imgbtn_set_src(imgbtn01, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
      lv_obj_set_size(imgbtn01, 100, 100);
      lv_obj_set_pos(imgbtn01, 120, 50);

      lv_obj_t *labelbtn01 = lv_label_create(imgbtn01);
      lv_label_set_text(labelbtn01, "BOTAO2");
      lv_obj_align(labelbtn01, LV_ALIGN_CENTER, 0, -4);

             /*CREATE AN IMGBTN02*/
      lv_obj_t *imgbtn02 = lv_btn_create(screenMain);
      //lv_imgbtn_set_src(imgbtn02, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
      lv_obj_set_size(imgbtn02, 100, 100);
      lv_obj_set_pos(imgbtn02, 230, 50);

      lv_obj_t *labelbtn02 = lv_label_create(imgbtn02);
      lv_label_set_text(labelbtn02, "BOTAO3");
      lv_obj_align(labelbtn02, LV_ALIGN_CENTER, 0, -4);

             /*CREATE AN IMGBTN03*/
      lv_obj_t *imgbtn03 = lv_btn_create(screenMain);
      //lv_imgbtn_set_src(imgbtn03, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
      lv_obj_set_size(imgbtn03, 100, 100);
      lv_obj_set_pos(imgbtn03, 340, 50);

      lv_obj_t *labelbtn03 = lv_label_create(imgbtn03);
      lv_label_set_text(labelbtn03, "BOTAO4");
      lv_obj_align(labelbtn03, LV_ALIGN_CENTER, 0, -4);

      /*CREATE AN IMGBTN04*/
      lv_obj_t *imgbtn04 = lv_btn_create(screenMain);
      //lv_imgbtn_set_src(imgbtn04, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
      lv_obj_set_size(imgbtn04, 100, 100);
      lv_obj_set_pos(imgbtn04, 450, 50);

      lv_obj_t *labelbtn04 = lv_label_create(imgbtn04);
      lv_label_set_text(labelbtn04, "BOTAO5");
      lv_obj_align(labelbtn04, LV_ALIGN_CENTER, 0, -4);

             /*CREATE AN IMGBTN10*/
      lv_obj_t *imgbtn10 = lv_btn_create(screenMain);
      //lv_imgbtn_set_src(imgbtn10, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
      lv_obj_set_size(imgbtn10, 100, 100);
      lv_obj_set_pos(imgbtn10, 10, 160);

      lv_obj_t *labelbtn10 = lv_label_create(imgbtn10);
      lv_label_set_text(labelbtn10, "BOTAO6");
      lv_obj_align(labelbtn10, LV_ALIGN_CENTER, 0, -4);

             /*CREATE AN IMGBTN11*/
      lv_obj_t *imgbtn11 = lv_btn_create(screenMain);
      //lv_imgbtn_set_src(imgbtn11, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
      lv_obj_set_size(imgbtn11, 100, 100);
      lv_obj_set_pos(imgbtn11, 120, 160);

      lv_obj_t *labelbtn11 = lv_label_create(imgbtn11);
      lv_label_set_text(labelbtn11, "BOTAO7");
      lv_obj_align(labelbtn11, LV_ALIGN_CENTER, 0, -4);

              /*CREATE AN IMGBTN12*/
      lv_obj_t *imgbtn12 = lv_btn_create(screenMain);
      //lv_imgbtn_set_src(imgbtn12, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
      lv_obj_set_size(imgbtn12, 100, 100);
      lv_obj_set_pos(imgbtn12, 230, 160);

      lv_obj_t *labelbtn12 = lv_label_create(imgbtn12);
      lv_label_set_text(labelbtn12, "BOTAO8");
      lv_obj_align(labelbtn12, LV_ALIGN_CENTER, 0, -4);

              /*CREATE AN IMGBTN13*/
      lv_obj_t *imgbtn13 = lv_btn_create(screenMain);
      //lv_imgbtn_set_src(imgbtn11, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
      lv_obj_set_size(imgbtn13, 100, 100);
      lv_obj_set_pos(imgbtn13, 340, 160);

      lv_obj_t *labelbtn13 = lv_label_create(imgbtn13);
      lv_label_set_text(labelbtn13, "BOTAO9");
      lv_obj_align(labelbtn13, LV_ALIGN_CENTER, 0, -4);

              /*CREATE AN IMGBTN14*/
      lv_obj_t *imgbtn14 = lv_btn_create(screenMain);
      //lv_imgbtn_set_src(imgbtn11, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
      lv_obj_set_size(imgbtn14, 100, 100);
      lv_obj_set_pos(imgbtn14, 450, 160);

      lv_obj_t *labelbtn14 = lv_label_create(imgbtn14);
      lv_label_set_text(labelbtn14, "BOTAO10");
      lv_obj_align(labelbtn14, LV_ALIGN_CENTER, 0, -4);

      /*CREATE AN IMGBTN20*/
      lv_obj_t *imgbtn20 = lv_btn_create(screenMain);
      //lv_imgbtn_set_src(imgbtn20, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
      lv_obj_set_size(imgbtn20, 100, 100);
      lv_obj_set_pos(imgbtn20, 10, 270);

      lv_obj_t *labelbtn20 = lv_label_create(imgbtn20);
      lv_label_set_text(labelbtn20, "BOTAO11");
      lv_obj_align(labelbtn20, LV_ALIGN_CENTER, 0, -4);

      /*CREATE AN IMGBTN21*/
      lv_obj_t *imgbtn21 = lv_btn_create(screenMain);
      //lv_imgbtn_set_src(imgbtn21, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
      lv_obj_set_size(imgbtn21, 100, 100);
      lv_obj_set_pos(imgbtn21, 120, 270);

      lv_obj_t *labelbtn21 = lv_label_create(imgbtn21);
      lv_label_set_text(labelbtn21, "BOTAO12");
      lv_obj_align(labelbtn21, LV_ALIGN_CENTER, 0, -4);

      /*CREATE AN IMGBTN22*/
      lv_obj_t *imgbtn22 = lv_btn_create(screenMain);
      //lv_imgbtn_set_src(imgbtn22, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
      lv_obj_set_size(imgbtn22, 100, 100);
      lv_obj_set_pos(imgbtn22, 230, 270);

      lv_obj_t *labelbtn22 = lv_label_create(imgbtn22);
      lv_label_set_text(labelbtn22, "BOTAO13");
      lv_obj_align(labelbtn22, LV_ALIGN_CENTER, 0, -4);

      /*CREATE AN IMGBTN23*/
      lv_obj_t *imgbtn23 = lv_btn_create(screenMain);
      //lv_imgbtn_set_src(imgbtn23, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
      lv_obj_set_size(imgbtn23, 100, 100);
      lv_obj_set_pos(imgbtn23, 340, 270);

      lv_obj_t *labelbtn23 = lv_label_create(imgbtn23);
      lv_label_set_text(labelbtn23, "BOTA14");
      lv_obj_align(labelbtn23, LV_ALIGN_CENTER, 0, -4);

      /*CREATE AN IMGBTN24*/
      lv_obj_t *imgbtn24 = lv_btn_create(screenMain);
      //lv_imgbtn_set_src(imgbtn24, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
      lv_obj_set_size(imgbtn24, 100, 100);
      lv_obj_set_pos(imgbtn24, 450, 270);

      lv_obj_t *labelbtn24 = lv_label_create(imgbtn24);
      lv_label_set_text(labelbtn24, "BOTAO15");
      lv_obj_align(labelbtn24, LV_ALIGN_CENTER, 0, -4);

/*CREATE AN IMGBTN30*/
      lv_obj_t *imgbtn30 = lv_btn_create(screenMain);
      //lv_imgbtn_set_src(imgbtn20, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
      lv_obj_set_size(imgbtn30, 100, 100);
      lv_obj_set_pos(imgbtn30, 10, 380);

      lv_obj_t *labelbtn30 = lv_label_create(imgbtn30);
      lv_label_set_text(labelbtn30, "BOTAO16");
      lv_obj_align(labelbtn30, LV_ALIGN_CENTER, 0, -4);

      /*CREATE AN IMGBTN31*/
      lv_obj_t *imgbtn31 = lv_btn_create(screenMain);
      //lv_imgbtn_set_src(imgbtn21, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
      lv_obj_set_size(imgbtn31, 100, 100);
      lv_obj_set_pos(imgbtn31, 120, 380);

      lv_obj_t *labelbtn31 = lv_label_create(imgbtn31);
      lv_label_set_text(labelbtn31, "BOTAO17");
      lv_obj_align(labelbtn31, LV_ALIGN_CENTER, 0, -4);

      /*CREATE AN IMGBTN32*/
      lv_obj_t *imgbtn32 = lv_btn_create(screenMain);
      //lv_imgbtn_set_src(imgbtn32, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
      lv_obj_set_size(imgbtn32, 100, 100);
      lv_obj_set_pos(imgbtn32, 230, 380);

      lv_obj_t *labelbtn32 = lv_label_create(imgbtn32);
      lv_label_set_text(labelbtn32, "BOTAO18");
      lv_obj_align(labelbtn32, LV_ALIGN_CENTER, 0, -4);

      /*CREATE AN IMGBTN33*/
      lv_obj_t *imgbtn33 = lv_btn_create(screenMain);
      //lv_imgbtn_set_src(imgbtn23, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
      lv_obj_set_size(imgbtn33, 100, 100);
      lv_obj_set_pos(imgbtn33, 340, 380);

      lv_obj_t *labelbtn33 = lv_label_create(imgbtn33);
      lv_label_set_text(labelbtn33, "BOTA19");
      lv_obj_align(labelbtn33, LV_ALIGN_CENTER, 0, -4);

      /*CREATE AN IMGBTN34*/
      lv_obj_t *imgbtn34 = lv_btn_create(screenMain);
      //lv_imgbtn_set_src(imgbtn24, LV_IMGBTN_STATE_RELEASED, NULL, NULL, NULL);
      lv_obj_set_size(imgbtn34, 100, 100);
      lv_obj_set_pos(imgbtn34, 450, 380);

      lv_obj_t *labelbtn34 = lv_label_create(imgbtn34);
      lv_label_set_text(labelbtn34, "BOTAO20");
      lv_obj_align(labelbtn34, LV_ALIGN_CENTER, 0, -4);
      
/*  CREATE PANEL FOR TARGETS */
      lv_obj_t *panel = lv_obj_create(screenMain);
      lv_obj_set_size(panel, 200, 400);
      lv_obj_set_scroll_snap_y(panel, LV_SCROLL_SNAP_START);
      lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
      lv_obj_align(panel, LV_ALIGN_RIGHT_MID, 0, 0);

      uint32_t i;
      for(i = 0; i < 10; i++) {
          lv_obj_t * btn = lv_btn_create(panel);
          lv_obj_set_size(btn, 150, 135);
          lv_obj_set_pos(btn, 0, 30);

          lv_obj_t * label = lv_label_create(btn);
          lv_label_set_text_fmt(label, "Panel %"LV_PRIu32, i);
          
          lv_obj_center(label);
      }
      lv_obj_update_snap(panel, LV_ANIM_ON);

      lv_obj_set_pos(panel, 0, 10);

      lv_scr_load(screenMain);

      Serial.println("Setup Done");
  }    
}

void loop()
{
  lv_timer_handler();
  delay(5);
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
