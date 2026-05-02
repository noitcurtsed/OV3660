/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Pin map matches Espressif ESP32-S3-EYE (main + LCD sub-board), same as ESP-IDF
 * examples/peripherals/camera/dvp_spi_lcd and the ESP32-S3-EYE BSP defaults
 * (`BSP_I2C_SDA` / `BSP_I2C_SCL` = 4 / 5 for camera SCCB). SCCB pins can be overridden in menuconfig.
 */
//----------CAM Config------------//
#define EXAMPLE_RGB565_BITS_PER_PIXEL      16
#define EXAMPLE_RGB565_BYTES_PER_PIXEL     (EXAMPLE_RGB565_BITS_PER_PIXEL / 8)

#define EXAMPLE_DVP_CAM_SCCB_SDA_IO        (CONFIG_EXAMPLE_CAM_SCCB_SDA_GPIO)
#define EXAMPLE_DVP_CAM_SCCB_SCL_IO        (CONFIG_EXAMPLE_CAM_SCCB_SCL_GPIO)

#if CONFIG_EXAMPLE_CAMERA_SENSOR_OV5640
#define EXAMPLE_DVP_CAM_XCLK_FREQ_HZ       (24000000)
#elif CONFIG_EXAMPLE_CAMERA_SENSOR_OV3660
#define EXAMPLE_DVP_CAM_XCLK_FREQ_HZ       (20000000)
#else
#error "Select OV3660 or OV5640 under Example Configuration > Camera sensor model."
#endif

#define EXAMPLE_DVP_CAM_DATA_WIDTH         (8)

#define EXAMPLE_DVP_CAM_D0_IO              (11)
#define EXAMPLE_DVP_CAM_D1_IO              (9)
#define EXAMPLE_DVP_CAM_D2_IO              (8)
#define EXAMPLE_DVP_CAM_D3_IO              (10)
#define EXAMPLE_DVP_CAM_D4_IO              (12)
#define EXAMPLE_DVP_CAM_D5_IO              (18)
#define EXAMPLE_DVP_CAM_D6_IO              (17)
#define EXAMPLE_DVP_CAM_D7_IO              (16)

#define EXAMPLE_DVP_CAM_XCLK_IO            (15)
#define EXAMPLE_DVP_CAM_PCLK_IO            (13)
#define EXAMPLE_DVP_CAM_DE_IO              (7)
#define EXAMPLE_DVP_CAM_VSYNC_IO           (6)
#define EXAMPLE_DVP_CAM_HSYNC_IO           (-1)

#if CONFIG_SPIRAM
#define EXAMPLE_DVP_CAM_BUF_ALLOC_CAPS     (MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA)
#else
#define EXAMPLE_DVP_CAM_BUF_ALLOC_CAPS     (MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA)
#endif

#if CONFIG_EXAMPLE_CAMERA_SENSOR_OV5640
#define EXAMPLE_CAM_HRES_RUNTIME 320
#define EXAMPLE_CAM_VRES_RUNTIME 240
#else
#define EXAMPLE_CAM_HRES_RUNTIME CONFIG_EXAMPLE_CAM_HRES
#define EXAMPLE_CAM_VRES_RUNTIME CONFIG_EXAMPLE_CAM_VRES
#endif

/* Format .name strings must match esp_cam_sensor ov3660.c / ov5640.c. */
#if CONFIG_EXAMPLE_CAMERA_SENSOR_OV3660
#if CONFIG_EXAMPLE_CAM_HRES == 240 && CONFIG_EXAMPLE_CAM_VRES == 240
#if CONFIG_EXAMPLE_CAM_INPUT_FORMAT_YUV422
#define EXAMPLE_CAM_FORMAT                  "DVP_8bit_20Minput_YUV422_240x240_24fps"
#elif CONFIG_EXAMPLE_CAM_INPUT_FORMAT_RGB565
#define EXAMPLE_CAM_FORMAT                  "DVP_8bit_20Minput_RGB565_240x240_24fps"
#endif
#elif CONFIG_EXAMPLE_CAM_HRES == 640 && CONFIG_EXAMPLE_CAM_VRES == 480
#if CONFIG_EXAMPLE_CAM_INPUT_FORMAT_YUV422
#define EXAMPLE_CAM_FORMAT                  "DVP_8bit_20Minput_YUV422_640x480_10fps"
#elif CONFIG_EXAMPLE_CAM_INPUT_FORMAT_RGB565
#define EXAMPLE_CAM_FORMAT                  "DVP_8bit_20Minput_RGB565_640x480_10fps"
#endif
#endif
#endif

#if CONFIG_EXAMPLE_CAMERA_SENSOR_OV5640
#if CONFIG_EXAMPLE_CAM_INPUT_FORMAT_YUV422
#define EXAMPLE_CAM_FORMAT                  "DVP_8bit_24Minput_YUV422_320x240_10fps"
#elif CONFIG_EXAMPLE_CAM_INPUT_FORMAT_RGB565
#define EXAMPLE_CAM_FORMAT                  "DVP_8bit_24Minput_RGB565_320x240_10fps"
#endif
#endif

#ifndef EXAMPLE_CAM_FORMAT
#error "Unsupported camera format: OV3660 uses 240x240 or 640x480; OV5640 uses QVGA 320x240; pick RGB565 or YUV422 in menuconfig."
#endif

//----------LCD Config------------//
#define EXAMPLE_LEDC_DVP_XCLK       (LEDC_TIMER_0)
#define EXAMPLE_LEDC_LCD_BACKLIGHT  (LEDC_TIMER_1)
#define EXAMPLE_LCD_SPI_NUM         (SPI3_HOST)
#define EXAMPLE_LCD_CMD_BITS        (8)
#define EXAMPLE_LCD_PARAM_BITS      (8)

/* LCD Display */
#define EXAMPLE_LCD_SPI_MOSI        (GPIO_NUM_47)
#define EXAMPLE_LCD_SPI_CLK         (GPIO_NUM_21)
#define EXAMPLE_LCD_SPI_CS          (GPIO_NUM_44)
#define EXAMPLE_LCD_DC              (GPIO_NUM_43)
#define EXAMPLE_LCD_RST             (GPIO_NUM_NC)
#define EXAMPLE_LCD_BACKLIGHT       (GPIO_NUM_48)

#define EXAMPLE_LCD_PIXEL_CLOCK_HZ  (80 * 1000 * 1000)

#ifdef __cplusplus
}
#endif
