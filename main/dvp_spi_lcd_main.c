/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sdkconfig.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_dev.h"
#include "esp_cache.h"
#include "driver/i2c_master.h"
#include "esp_cam_ctlr.h"
#include "esp_cam_ctlr_dvp.h"
#include "example_config.h"
#include "driver/ledc.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "example_sensor_init.h"

static const char *TAG = "dvp_spi_lcd";

#define BUFFER_SIZE         (EXAMPLE_CAM_HRES_RUNTIME * EXAMPLE_CAM_VRES_RUNTIME * EXAMPLE_RGB565_BYTES_PER_PIXEL)

typedef struct {
    esp_lcd_panel_handle_t panel_hdl;
    esp_cam_ctlr_trans_t cam_trans;
#if CONFIG_EXAMPLE_ENABLE_LCD
    void *bufs[2];
    uint8_t next_buf;
#endif
} example_cam_context_t;

#if CONFIG_EXAMPLE_ENABLE_LCD
typedef struct {
    void *pix;
    esp_lcd_panel_handle_t panel;
} example_lcd_frame_t;

static QueueHandle_t s_lcd_frame_q;

static void lcd_draw_task(void *arg)
{
    example_lcd_frame_t frame;
    while (xQueueReceive(s_lcd_frame_q, &frame, portMAX_DELAY) == pdTRUE) {
        if (frame.panel && frame.pix) {
            esp_lcd_panel_draw_bitmap(frame.panel, 0, 0, EXAMPLE_CAM_HRES_RUNTIME, EXAMPLE_CAM_VRES_RUNTIME, frame.pix);
        }
    }
}
#endif

static bool s_camera_get_new_vb(esp_cam_ctlr_handle_t handle, esp_cam_ctlr_trans_t *trans, void *user_data);
static bool s_camera_get_finished_trans(esp_cam_ctlr_handle_t handle, esp_cam_ctlr_trans_t *trans, void *user_data);

#if CONFIG_EXAMPLE_ENABLE_LCD
static void lcd_display_init(esp_lcd_panel_handle_t *lcd_panel_hdl, esp_lcd_panel_io_handle_t lcd_io_hdl)
{
    esp_lcd_panel_handle_t panel_handle = NULL;
    //----------LEDC initialization------------//
    const ledc_timer_config_t lcd_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_10_BIT,
        .timer_num        = EXAMPLE_LEDC_LCD_BACKLIGHT,
        .freq_hz          = 5000,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&lcd_timer));
    const ledc_channel_config_t lcd_channel = {
        .gpio_num       = EXAMPLE_LCD_BACKLIGHT,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = EXAMPLE_LEDC_LCD_BACKLIGHT,
        .intr_type      = LEDC_INTR_DISABLE,
        .duty           = 0,
        .hpoint         = 0,
        .flags.output_invert = true,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&lcd_channel));

    //----------SPI initialization------------//
    ESP_LOGI(TAG, "Init SPI bus");
    const spi_bus_config_t bus_cfg = {
        .sclk_io_num = EXAMPLE_LCD_SPI_CLK,
        .mosi_io_num = EXAMPLE_LCD_SPI_MOSI,
        .miso_io_num = GPIO_NUM_NC,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = BUFFER_SIZE,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(EXAMPLE_LCD_SPI_NUM, &bus_cfg, SPI_DMA_CH_AUTO));

    //----------Panel IO initialization------------//
    ESP_LOGI(TAG, "New panel IO SPI");
    const esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num = EXAMPLE_LCD_DC,
        .cs_gpio_num = EXAMPLE_LCD_SPI_CS,
        .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
        .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
        .spi_mode         = 2,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
                        (esp_lcd_spi_bus_handle_t)EXAMPLE_LCD_SPI_NUM,
                        &io_cfg,
                        &lcd_io_hdl
                    ));

    //----------ST7789 Panel initialization------------//
#if CONFIG_EXAMPLE_CAMERA_SENSOR_OV5640 && CONFIG_EXAMPLE_OV5640_LCD_BGR565
    const lcd_rgb_element_order_t lcd_rgb_order = LCD_RGB_ELEMENT_ORDER_BGR;
#else
    const lcd_rgb_element_order_t lcd_rgb_order = LCD_RGB_ELEMENT_ORDER_RGB;
#endif
    ESP_LOGI(TAG, "New ST7789 panel (RGB element order %s)",
             lcd_rgb_order == LCD_RGB_ELEMENT_ORDER_BGR ? "BGR565" : "RGB565");

    const esp_lcd_panel_dev_config_t panel_dev_cfg = {
        .reset_gpio_num   = EXAMPLE_LCD_RST,
        .rgb_ele_order    = lcd_rgb_order,
        .bits_per_pixel   = EXAMPLE_RGB565_BITS_PER_PIXEL,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(lcd_io_hdl, &panel_dev_cfg, &panel_handle));

    ESP_LOGI(TAG, "Reset and init panel");
    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
#if CONFIG_EXAMPLE_OV5640_LCD_SWAP_XY
    ESP_LOGI(TAG, "LCD swap_xy enabled (OV5640 QVGA vs portrait ST7789)");
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
#endif
    esp_lcd_panel_invert_color(panel_handle, true);

    ESP_LOGI(TAG, "Turn on display");
    esp_lcd_panel_disp_on_off(panel_handle, true);

    const int brightness = 100;
    uint32_t duty = (1023 * brightness) / 100;
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));

    *lcd_panel_hdl = panel_handle;
}
#endif

void app_main(void)
{
    esp_err_t ret = ESP_FAIL;

    esp_lcd_panel_handle_t lcd_panel_hdl = NULL;
#if CONFIG_EXAMPLE_ENABLE_LCD
    esp_lcd_panel_io_handle_t lcd_io_hdl = NULL;
    lcd_display_init(&lcd_panel_hdl, lcd_io_hdl);
#else
    ESP_LOGI(TAG, "LCD disabled (Example Configuration): camera-only mode");
#endif

    //----------CAM Controller Init------------//
    esp_cam_ctlr_handle_t cam_handle = NULL;
    esp_cam_ctlr_dvp_pin_config_t pin_cfg = {
        .data_width = EXAMPLE_DVP_CAM_DATA_WIDTH,
        .data_io = {
            EXAMPLE_DVP_CAM_D0_IO,
            EXAMPLE_DVP_CAM_D1_IO,
            EXAMPLE_DVP_CAM_D2_IO,
            EXAMPLE_DVP_CAM_D3_IO,
            EXAMPLE_DVP_CAM_D4_IO,
            EXAMPLE_DVP_CAM_D5_IO,
            EXAMPLE_DVP_CAM_D6_IO,
            EXAMPLE_DVP_CAM_D7_IO,
        },
        .vsync_io = EXAMPLE_DVP_CAM_VSYNC_IO,
        .de_io = EXAMPLE_DVP_CAM_DE_IO,
        .pclk_io = EXAMPLE_DVP_CAM_PCLK_IO,
        .xclk_io = EXAMPLE_DVP_CAM_XCLK_IO,
    };

    esp_cam_ctlr_dvp_config_t dvp_config = {
        .ctlr_id = 0,
        .clk_src = CAM_CLK_SRC_DEFAULT,
        .h_res = EXAMPLE_CAM_HRES_RUNTIME,
        .v_res = EXAMPLE_CAM_VRES_RUNTIME,
#if CONFIG_EXAMPLE_CAM_INPUT_FORMAT_YUV422
        .input_data_color_type = CAM_CTLR_COLOR_YUV422,
#else
        .input_data_color_type = CAM_CTLR_COLOR_RGB565,
#endif
        .dma_burst_size = 64,
        .pin = &pin_cfg,
        .bk_buffer_dis = 1,
        .xclk_freq = EXAMPLE_DVP_CAM_XCLK_FREQ_HZ,
    };

    ret = esp_cam_new_dvp_ctlr(&dvp_config, &cam_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "dvp init fail[%d]", ret);
        return;
    }

    /* DVP driver starts XCLK here; allow SCCB settle before sensor ID probe (OV3660/OV5640). */
    vTaskDelay(pdMS_TO_TICKS(CONFIG_EXAMPLE_CAM_XCLK_SETTLE_MS));

    //--------Allocate Camera Buffer----------//
    size_t cam_buffer_size = EXAMPLE_CAM_HRES_RUNTIME * EXAMPLE_CAM_VRES_RUNTIME * EXAMPLE_RGB565_BYTES_PER_PIXEL;
#if CONFIG_EXAMPLE_ENABLE_LCD
    void *cam_buf0 = esp_cam_ctlr_alloc_buffer(cam_handle, cam_buffer_size, EXAMPLE_DVP_CAM_BUF_ALLOC_CAPS);
    void *cam_buf1 = esp_cam_ctlr_alloc_buffer(cam_handle, cam_buffer_size, EXAMPLE_DVP_CAM_BUF_ALLOC_CAPS);
    if (cam_buf0 == NULL || cam_buf1 == NULL) {
        ESP_LOGE(TAG, "camera buffer alloc fail");
        return;
    }
#else
    void *cam_buffer = esp_cam_ctlr_alloc_buffer(cam_handle, cam_buffer_size, EXAMPLE_DVP_CAM_BUF_ALLOC_CAPS);
#endif

    //--------Camera Sensor and SCCB Init-----------//
    example_sensor_config_t cam_sensor_config = {
        .i2c_port_num = I2C_NUM_0,
        .i2c_sda_io_num = EXAMPLE_DVP_CAM_SCCB_SDA_IO,
        .i2c_scl_io_num = EXAMPLE_DVP_CAM_SCCB_SCL_IO,
        .port = ESP_CAM_SENSOR_DVP,
        .format_name = EXAMPLE_CAM_FORMAT,
    };
    example_sensor_handle_t sensor_handle = {
        .sccb_handle = NULL,
        .i2c_bus_handle = NULL,
    };
    example_sensor_init(&cam_sensor_config, &sensor_handle);

    //--------Register Camera Callbacks----------//
    example_cam_context_t cam_ctx = {
        .panel_hdl = lcd_panel_hdl,
#if !CONFIG_EXAMPLE_ENABLE_LCD
        .cam_trans = {
            .buffer = cam_buffer,
            .buflen = cam_buffer_size,
        },
#endif
    };
#if CONFIG_EXAMPLE_ENABLE_LCD
    cam_ctx.bufs[0] = cam_buf0;
    cam_ctx.bufs[1] = cam_buf1;
    cam_ctx.next_buf = 0;
    cam_ctx.cam_trans.buflen = cam_buffer_size;

    s_lcd_frame_q = xQueueCreate(2, sizeof(example_lcd_frame_t));
    if (s_lcd_frame_q == NULL) {
        ESP_LOGE(TAG, "lcd frame queue alloc fail");
        return;
    }
    xTaskCreate(lcd_draw_task, "lcd_draw", 4096, NULL, 5, NULL);
#endif

    esp_cam_ctlr_evt_cbs_t cbs = {
        .on_get_new_trans = s_camera_get_new_vb,
        .on_trans_finished = s_camera_get_finished_trans,
    };
    if (esp_cam_ctlr_register_event_callbacks(cam_handle, &cbs, &cam_ctx) != ESP_OK) {
        ESP_LOGE(TAG, "ops register fail");
        return;
    }

    //--------Enable and start Camera Controller----------//
    ESP_ERROR_CHECK(esp_cam_ctlr_enable(cam_handle));

#if CONFIG_EXAMPLE_CAM_INPUT_FORMAT_YUV422
    ESP_LOGI(TAG, "Configure format conversion: YUV422 -> RGB565");
    // Configure format conversion
    const cam_ctlr_format_conv_config_t conv_cfg = {
        .src_format = CAM_CTLR_COLOR_YUV422,      // Source format: YUV422
        .dst_format = CAM_CTLR_COLOR_RGB565,      // Destination format: RGB565
        .conv_std = COLOR_CONV_STD_RGB_YUV_BT601,
        .data_width = 8,
        .input_range = COLOR_RANGE_LIMIT,
        .output_range = COLOR_RANGE_LIMIT,
    };
    ESP_ERROR_CHECK(esp_cam_ctlr_format_conversion(cam_handle, &conv_cfg));
#endif

    if (esp_cam_ctlr_start(cam_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Driver start fail");
        return;
    }

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

static bool s_camera_get_new_vb(esp_cam_ctlr_handle_t handle, esp_cam_ctlr_trans_t *trans, void *user_data)
{
    example_cam_context_t *ctx = (example_cam_context_t *)user_data;
#if CONFIG_EXAMPLE_ENABLE_LCD
    ctx->cam_trans.buffer = ctx->bufs[ctx->next_buf];
    ctx->cam_trans.buflen = BUFFER_SIZE;
    ctx->next_buf ^= 1;
    *trans = ctx->cam_trans;
#else
    *trans = ctx->cam_trans;
#endif
    (void)handle;
    return false;
}

static bool s_camera_get_finished_trans(esp_cam_ctlr_handle_t handle, esp_cam_ctlr_trans_t *trans, void *user_data)
{
    example_cam_context_t *ctx = (example_cam_context_t *)user_data;
#if CONFIG_EXAMPLE_ENABLE_LCD
    BaseType_t hpw = pdFALSE;
    if (ctx->panel_hdl && s_lcd_frame_q) {
        example_lcd_frame_t frame = {
            .pix = trans->buffer,
            .panel = ctx->panel_hdl,
        };
        if (xQueueSendFromISR(s_lcd_frame_q, &frame, &hpw) != pdTRUE) {
            /* LCD slower than sensor: drop frame */
        }
    }
    (void)handle;
    return hpw == pdTRUE;
#else
    (void)handle;
    (void)trans;
    (void)ctx;
    return false;
#endif
}
