# TLC59731 (EasySet) driver for ESP32 (ESP-IDF)

Texas Instruments [TLC59731](http://www.ti.com/product/TLC59731) PWM LED controller driver for the ESP32 SoC.
The chip utilizes TI's single-wire serial interface called <i>EasySet</i>.

<ul>
<li><i>EasySet</i> implementation using the RMT peripheral</li>
<li><i>To be used with Espressif's ESP-IDF (IoT Development Framework), <b>not an Arduino library</b></i></li>
</ul>

## Driver API

The driver itself is implemented as an ESP-IDF component with a simple API consisting of only three functions:

- **esp_err_t tlc59731_init(tlc59731_handle_t \**h, uint8_t pin, uint8_t rmt_ch)**: initializes the resources
  for the driver's operation such as GPIO direction, level and RMT peripheral configuration. Returns ESP_OK on success,
  *esp_err_t* error code on failure. The handle argument will be initialized and populated on success and needs to
  be passed as argument to all future API calls. When passing the handle argument to this function, the handle itself
  needs to be NULL and if successful, it will contain non-NULL value after the function completes. This is to prevent
  bugs caused by calling it twice on the same configuration, in which case it will return ESP_ERR_INVALID_STATE.

- **esp_err_t tlc59731_set_grayscale(tlc59731_handle_t \*h, led_rgb_t \*gs, uint8_t count_leds)**: takes the
  pointer to an array of desired LED RGB settings for *count_leds* number of LEDs, encodes them into the *EasySet*
  protocol  and writes onto the configured pin using the RMT peripheral. The function returns ESP_OK on success and
  *esp_err_t* error code on failure.

- **esp_err_t tlc59731_release(tlc59731_handle_t \**h)**: this function performs a cleanup of all the
  resources used by the driver, uninstalls the RMT peripheral and frees the handle. After calling this
  function on a handle, the handle is no longer valid and needs to be reinitialized by calling *tlc59731_init()*
  again if the driver is to be used again at a later time. The GPIO state will be left as-is after the call to this
  function and direction will remain set as OUTPUT. The function returns ESP_OK on success and *esp_err_t* error code
  on failure.

## Note

The driver in its current revision relies on the legacy RMT peripheral driver as the [newer version of RMT peripheral
driver was introduced by Espressif in ESP-IDF v5.0](https://docs.espressif.com/projects/esp-idf/en/release-v5.0/esp32s2/migration-guides/release-5.x/peripherals.html#rmt-driver).
The TLC59731 driver will be switched to using the new RMT peripheral driver in the next update, however, it should
work fine as of now since the latest ESP-IDF version still fully supports the legacy driver.
