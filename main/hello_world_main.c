#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "lvgl.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "lvgl_helpers.h"
#include "driver/i2c.h"
#include "i2cStm32.h"
/*********************
 *      DEFINES
 *********************/
#define LV_TICK_PERIOD_MS 1
/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_tick_task(void *arg);
static void guiTask(void *pvParameter);
static void create_demo_application(void);
static void butter_application(void);
static void butter_live(void *pvParameter);
/**********************
 *   APPLICATION MAIN
 **********************/

lv_obj_t *label1;
lv_obj_t *label2;
lv_obj_t *label3;

void app_main(void)
{
    i2c_master_init();
    // xTaskCreate(guiTask, "gui", 4096*2, NULL, 1, NULL);
    // xTaskCreatePinnedToCore(guiTask, "gui", 4096*2, NULL, 1, NULL, 1);
    /* NOTE: When not using Wi-Fi nor Bluetooth you can pin the guiTask to core 0 */
    xTaskCreatePinnedToCore(guiTask, "gui", 4096 * 2, NULL, 0, NULL, 1);
}
/* Creates a semaphore to handle concurrent call to lvgl stuff
 * If you wish to call *any* lvgl function from other threads/tasks
 * you should lock on the very same semaphore! */
SemaphoreHandle_t xGuiSemaphore;
static void guiTask(void *pvParameter)
{
    (void)pvParameter;
    xGuiSemaphore = xSemaphoreCreateMutex();
    lv_init();
    /* Initialize SPI or I2C bus used by the drivers */
    lvgl_driver_init();
    lv_color_t *buf1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1 != NULL);
    /* Use double buffered when not working with monochrome displays */
    lv_color_t *buf2 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2 != NULL);
    static lv_disp_draw_buf_t disp_buf;
    uint32_t size_in_px = DISP_BUF_SIZE;
    /* Initialize the working buffer depending on the selected display. */
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, size_in_px);
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LV_VER_RES_MAX;
    disp_drv.ver_res = LV_HOR_RES_MAX;
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);
    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "periodic_gui"};
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));


    /* Create the demo application */
    // create_demo_application();
    butter_application();
    xTaskCreatePinnedToCore(butter_live, "butter_live", 4096 * 2, NULL, 0, NULL, 1);
    while (1)
    {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS(10));
        lv_task_handler();
        /* Try to take the semaphore, call lvgl related function on success */
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY))
        {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
        }
    }
}
static void create_demo_application(void)
{
    /* Get the current screen  */
    lv_obj_t *scr = lv_disp_get_scr_act(NULL);
    /*Create a Label on the currently active screen*/
    lv_obj_t *label1 = lv_label_create(scr);
    /*Modify the Label's text*/
    lv_label_set_text(label1, "Hello\nworld");
    /* Align the Label to the center
     * NULL means align on parent (which is the screen now)
     * 0, 0 at the end means an x, y offset after alignment*/
    lv_obj_align(label1, LV_ALIGN_CENTER, 0, 0);
    // lv_obj_set_pos(label1,50,50);
}

static void butter_application(void)
{
    /* Get the current screen  */
    lv_obj_t *scr = lv_disp_get_scr_act(NULL);
    /*Create a Label on the currently active screen*/
    label1 = lv_label_create(scr);
    label2 = lv_label_create(scr);
    label3 = lv_label_create(scr);
    /*Modify the Label's text*/
    lv_label_set_text_fmt(label1, "Ser=%dA",0);
    lv_label_set_text_fmt(label2, "Mot=%dA",0);
    lv_label_set_text_fmt(label3, "BAT=%dV",0);
    /* Align the Label to the center
     * NULL means align on parent (which is the screen now)
     * 0, 0 at the end means an x, y offset after alignment*/
    lv_obj_align(label1, LV_ALIGN_CENTER, 0, -15);
    lv_obj_align(label2, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(label3, LV_ALIGN_CENTER, 0, 15);
}
static void butter_live(void *pvParameter)
{
    while(1)
    {
        uint16_t butter_date[3]={0};
        char string_show[20]={0};
        stm32_register_read(0x00,(uint8_t *)butter_date,6);

        for(int i=0;i<3;i++)
        {
            snprintf(string_show,20,"Ser=%0.2fA\n",((butter_date[0]*3.3/4096.0)/200.0)/0.01);
            lv_label_set_text(label1, string_show);
            snprintf(string_show,20,"Mot=%0.2fA\n",((butter_date[1]*3.3/4096.0)/200.0)/0.01);
            lv_label_set_text(label2, string_show);
            snprintf(string_show,20,"BAT=%0.2fV\n",(butter_date[2]*3.3/4096.0)*10.3448);
            lv_label_set_text(label3, string_show);
        }
        // lv_label_set_text_fmt(label1,"Ser=%0.2fA\n",((butter_date[0]*3.3/4096.0)/200.0)/0.01);
        // lv_label_set_text_fmt(label2,"Mot=%0.2fA\n",((butter_date[1]*3.3/4096.0)/200.0)/0.01);
        // lv_label_set_text_fmt(label3,"BAT=%0.2fV\n",(butter_date[2]*3.3/4096.0)*10.3448);
        vTaskDelay(  500 / portTICK_RATE_MS );
        // lv_label_set_text_fmt(label1, "label1=%0.2f",((butter_date[0]*3.3/4096.0)/200)/0.01);
        // lv_label_set_text_fmt(label2, "label2=%0.2f",((butter_date[0]*3.3/4096.0)/200)/0.01);
        // lv_label_set_text_fmt(label3, "label3=%0.2f",((butter_date[0]*3.3/4096.0)/200)/0.01);
    }

}
static void lv_tick_task(void *arg)
{
    (void)arg;
    lv_tick_inc(LV_TICK_PERIOD_MS);
}