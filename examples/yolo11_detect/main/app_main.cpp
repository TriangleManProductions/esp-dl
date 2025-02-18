#include "coco_detect.hpp"
#include "esp_log.h"
#include "esp_timer.h"

extern const uint8_t bus_jpg_start[] asm("_binary_bus_jpg_start");
extern const uint8_t bus_jpg_end[] asm("_binary_bus_jpg_end");
const char *TAG = "YOLO11_n_EXAMPLE";

extern "C" void app_main(void)
{
    dl::image::jpeg_img_t jpeg_img = {
        .data = (uint8_t *)bus_jpg_start,
        .width = 405,
        .height = 540,
        .data_size = (uint32_t)(bus_jpg_end - bus_jpg_start),
    };
    dl::image::img_t img;
    img.pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB888;
    sw_decode_jpeg(jpeg_img, img, true);

    COCODetect *detect = new COCODetect();
    vTaskDelay(pdMS_TO_TICKS(10)); // delay 10ms
    auto &detect_results = detect->run(img);
    for (const auto &res : detect_results) {
        ESP_LOGI(TAG,
                 "[category: %d, score: %f, x1: %d, y1: %d, x2: %d, y2: %d]\n",
                 res.category,
                 res.score,
                 res.box[0],
                 res.box[1],
                 res.box[2],
                 res.box[3]);
    }
    delete detect;
    heap_caps_free(img.data);
}
