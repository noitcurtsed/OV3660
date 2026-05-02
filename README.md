| Supported Targets | ESP32-P4 | ESP32-S3 |
| ----------------- | -------- | -------- |


# DVP Camera display via LCD example

## Overview

This example demonstrates how to use the esp_driver_cam component to capture DVP camera sensor signals and display it via LCD interface. This example will auto-detect camera sensors via [ESP camera sensor driver](https://components.espressif.com/components/espressif/esp_cam_sensor) and capture camera sensor signals via DVP interface and display it via LCD interface.

## Usage

The subsections below give only absolutely necessary information. For full steps to configure ESP-IDF and use it to build and run projects, see [ESP-IDF Getting Started](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html#get-started).


### Hardware Required

- **Espressif ESP32-S3-EYE** (recommended for this repo): onboard DVP camera header + ST7789 LCD sub-board. This project targets **OV3660** on the same wiring as the stock ESP-IDF `dvp_spi_lcd` example (`example_config.h` pin numbers match that board).
- Other ESP32-S3 kits with parallel DVP camera + SPI LCD are supported if you align GPIOs in `menuconfig` (**Example Configuration** → SCCB pins) and in [`main/example_config.h`](main/example_config.h).

You can also use sensors or LCDs from other vendors if drivers exist in the [ESP Component Registry](https://components.espressif.com).


                                   GND                                                                   GND
                ┌────────────────────────────────────────────────┐             ┌─────────────────────────────────────────────────────────┐
                │                                                │             │                                                         │
                │                                                │             │                                                         │
                │                                                │             │                                                         │
                │                                                │             │                                                         │
                │                                                │             │                                                         │
                │                                                │             │                                                         │
                │                                                │             │                                                         │
                │                                                │             │                                                         │
                │                                ┌───────────────┴─────────────┴──────────────────┐                                      │
                │                                │                                                │                           ┌──────────┴───────────┐
                │                                │                                                │      LCD MOSI             │                      │
                │                                │                                                ├───────────────────────────┤                      │
    ┌───────────┴─────────┐                      │                                                │                           │                      │
    │                     │                      │                                                │      LCD CLK              │                      │
    │                     │                      │                                                ├───────────────────────────┤                      │
    │                     │       XCLK           │                  ESP_CHIP                      │                           │                      │
    │     DVP Camera      ├──────────────────────┤                                                │      LCD CS               │      LCD Screen      │
    │                     │                      │                                                ├───────────────────────────┤                      │
    │                     │       D0~7           │                                                │                           │                      │
    │                     ├──────────────────────┤                                                │      LCD DC               │                      │
    │                     │                      │                                                ├───────────────────────────┤                      │
    │                     │       PCLK           │                                                │                           │                      │
    │                     ├──────────────────────┤                                                │      LCD BACKLIGHT        │                      │
    │                     │                      │                                                ├───────────────────────────┤                      │
    │                     │       VSYNC          │                                                │                           │                      │
    │                     ├──────────────────────┤                                                │                           │                      │
    │                     │                      │                                                │                           │                      │
    │                     │      DE (HREF)       │                                                │                           │                      │
    │                     ├──────────────────────┤                                                │                           └──────────────────────┘
    │                     │                      │                                                │
    └───────┬──┬──────────┘                      │                                                │
            │  │           I2C SCL               │                                                │
            │  └─────────────────────────────────┤                                                │
            │              I2C SDA               │                                                │
            └────────────────────────────────────┤                                                │
                                                 └────────────────────────────────────────────────┘


### Set Chip Target

First of all, your target must be supported by both:

- **By your ESP-IDF version**: For the full list of supported targets, run:
  ```
  idf.py --list-targets
  ```
- **By this example**: For the full list of supported targets,  refer to the supported targets table at the top of this README.

After you make sure that your target is supported, go to your example project directory and [set the chip target](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/tools/idf-py.html#select-the-target-chip-set-target):

```
idf.py set-target <target>
```

For example, to set esp32-S3 as the chip target, run:

```
idf.py set-target esp32s3
```


### Configure the Project

For information about Kconfig options, see [Project Configuration](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/kconfig.html) > _Name of relevant section(s)_.

To conveniently check or modify Kconfig options for this example in a project configuration menu, run:

```
idf.py menuconfig
```

Enable **OV3660** under **Espressif Camera Sensors Configurations** (default in `sdkconfig.defaults`). Leave **Example Configuration → Enable SPI LCD** off unless your panel matches the ST7789 wiring in `example_config.h`.

Available options for resolution and SCCB pins can be seen in ``menuconfig`` > ``Example Configuration`` (ESP32-S3-EYE defaults: SCCB SDA = GPIO 4, SCL = GPIO 5).

If boot logs show ``Get sensor ID failed`` / SCCB transmit errors: enable **Example Configuration → Log I2C probe** (also defaulted on via `sdkconfig.defaults`). An ACK at **0x30** usually means an OV2640-class sensor, not OV3660 (**0x3c**, or **0x3d** if SID is strapped). No ACK at **0x3c** / **0x3d** points to wiring, power, or missing XCLK settle—increase **Delay after DVP starts XCLK**, lower **SCCB frequency** (try 10 kHz), or toggle **internal pull-ups** if your board already has external resistors.


### Build and Flash

Execute the following command to build the project, flash it to your development board, and run the monitor tool to view the serial output:

```
idf.py build flash monitor
```

This command can be reduced to `idf.py flash monitor`.

If the above command fails, check the log on the serial monitor which usually provides information on the possible cause of the issue.

To exit the serial monitor, use `Ctrl` + `]`.


## Example Output

If you see the following console output, your example should be running correctly:

```
I (1481) main_task: Calling app_main()
I (278) dvp_spi_lcd: Init SPI bus
I (278) dvp_spi_lcd: New panel IO SPI
I (278) dvp_spi_lcd: New ST7789 panel
I (278) dvp_spi_lcd: Reset and init panel
I (408) dvp_spi_lcd: Turn on display
I (408) dvp_spi_lcd: Screen lit up now!

```


## Reference

- Link to the ESP-IDF feature's API reference, for example [ESP-IDF: Camera Controller Driver](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/camera_driver.html)
- [ESP-IDF Getting Started](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html#get-started)
- [Project Configuration](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/kconfig.html) (Kconfig Options)
