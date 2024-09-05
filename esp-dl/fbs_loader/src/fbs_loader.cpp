#include "fbs_loader.hpp"
#include "mbedtls/aes.h"

static const char *TAG = "FbsLoader";

namespace fbs {

/**
 * @brief This function is used to decrypt the AES 128-bit CTR mode encrypted data.
 * AES (Advanced Encryption Standard) is a widely-used symmetric encryption algorithm that provides strong security for
 * data protection CTR mode converts the block cipher into a stream cipher, allowing it to encrypt data of any length
 * without the need for padding
 *
 * @param ciphertext   Input Fbs data encrypted by AES 128-bit CTR mode
 * @param plaintext    Decrypted data
 * @param size         Size of input data
 * @param key          128-bit AES key
 */
void fbs_aes_crypt_ctr(const uint8_t *ciphertext, uint8_t *plaintext, size_t size, const uint8_t *key)
{
    mbedtls_aes_context aes_ctx;
    size_t offset = 0;
    uint8_t nonce[16] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};
    uint8_t stream_block[16];
    mbedtls_aes_init(&aes_ctx);
    mbedtls_aes_setkey_enc(&aes_ctx, key, 128); // 128-bit key
    mbedtls_aes_crypt_ctr(&aes_ctx, size, &offset, nonce, stream_block, ciphertext, plaintext);
    mbedtls_aes_free(&aes_ctx);
}

/**
    FBS_FILE_FORMAT_EDL1:
    {
        char[4]: "EDL1",
        uint32:  the mode of entru
        uint32:  the length of data
        uint8[]:  the data
    }

    FBS_FILE_FORMAT_PDL1:
    {
        "PDL1": char[4]
        model_num: uint32
        model1_data_offset: uint32
        model1_name_offset: uint32
        model1_name_length: uint32
        model2_data_offset: uint32
        model2_name_offset: uint32
        model2_name_length: uint32
        ...
        model1_name,
        model2_name,
        ...
        model1_data(format:FBS_FILE_FORMAT_EDL1),
        model2_data(format:FBS_FILE_FORMAT_EDL1),
        ...
    }
*/
typedef enum { FBS_FILE_FORMAT_UNK = 0, FBS_FILE_FORMAT_EDL1 = 1, FBS_FILE_FORMAT_PDL1 = 2 } fbs_file_format_t;

fbs_file_format_t get_model_format(const char *format)
{
    char str[5];
    memcpy(str, format, 4);
    str[4] = '\0';

    if (strcmp(str, "EDL1") == 0) {
        return FBS_FILE_FORMAT_EDL1;
    } else if (strcmp(str, "PDL1") == 0) {
        return FBS_FILE_FORMAT_PDL1;
    } else {
        return FBS_FILE_FORMAT_UNK;
    }

    return FBS_FILE_FORMAT_UNK;
}

esp_err_t get_model_offset_by_index(const uint8_t *fbs_buf, uint32_t index, uint32_t &offset)
{
    const uint32_t *header = (const uint32_t *)fbs_buf;
    uint32_t model_num = header[1];
    if (index >= model_num) {
        ESP_LOGE(TAG, "The model index is out of range.");
        return ESP_FAIL;
    }

    offset = header[2 + index * 3];
    return ESP_OK;
}

FbsModel *create_fbs_model(const uint8_t *model_buf, const uint8_t *key)
{
    if (model_buf == nullptr) {
        ESP_LOGE(TAG, "Model's flatbuffers is empty.");
        return nullptr;
    }

    uint32_t *header = (uint32_t *)model_buf;
    uint32_t mode = header[1]; // cryptographic mode, 0: without encryption, 1: aes encryption
    uint32_t size = header[2];
    if (mode != 0 && key == NULL) {
        ESP_LOGE(TAG, "This is a cryptographic model, please enter the secret key!");
        return nullptr;
    }

    model_buf += 12;
    if (mode == 0) { // without encryption
        return new FbsModel(model_buf, false);
    } else if (mode == 1) { // 128-bit AES encryption
        uint8_t *m_data = (uint8_t *)heap_caps_malloc(size, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
        fbs_aes_crypt_ctr(model_buf, m_data, size, key);
        return new FbsModel(m_data, true);
    } else {
        ESP_LOGE(TAG, "The model file is corrupted or is an unsupported file type");
    }

    return nullptr;
}

FbsLoader::FbsLoader(const char *name, model_location_type_t location) :
    m_mmap_handle(nullptr), m_location(location), m_fbs_buf(nullptr)
{
    if (name == nullptr) {
        return;
    }

    if (m_location == MODEL_LOCATION_IN_FLASH_RODATA) {
        m_fbs_buf = (const void *)name;
    } else if (m_location == MODEL_LOCATION_IN_FLASH_PARTITION) {
        const esp_partition_t *partition =
            esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, name);
        if (partition) {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
            int free_pages = spi_flash_mmap_get_free_pages(SPI_FLASH_MMAP_DATA);
            uint32_t storage_size = free_pages * 64 * 1024; // Byte
            ESP_LOGI(TAG, "The storage free size is %ld KB", storage_size / 1024);
            ESP_LOGI(TAG, "The partition size is %ld KB", partition->size / 1024);
            if (storage_size < partition->size) {
                ESP_LOGE(TAG,
                         "The storage free size of this board is less than %s partition required size",
                         partition->label);
            }
            this->m_mmap_handle = (esp_partition_mmap_handle_t *)malloc(sizeof(esp_partition_mmap_handle_t));
            ESP_ERROR_CHECK(esp_partition_mmap(partition,
                                               0,
                                               partition->size,
                                               ESP_PARTITION_MMAP_DATA,
                                               &this->m_fbs_buf,
                                               static_cast<esp_partition_mmap_handle_t *>(this->m_mmap_handle)));
#else
            int free_pages = spi_flash_mmap_get_free_pages(SPI_FLASH_MMAP_DATA);
            uint32_t storage_size = free_pages * 64 * 1024; // Byte
            ESP_LOGI(TAG, "The storage free size is %d KB", storage_size / 1024);
            ESP_LOGI(TAG, "The partition size is %d KB", partition->size / 1024);
            if (storage_size < partition->size) {
                ESP_LOGE(TAG, "The storage free size of board is less than %s partition size", partition->label);
            }
            this->m_mmap_handle = (spi_flash_mmap_handle_t *)malloc(sizeof(spi_flash_mmap_handle_t));
            ESP_ERROR_CHECK(esp_partition_mmap(partition,
                                               0,
                                               partition->size,
                                               SPI_FLASH_MMAP_DATA,
                                               &this->m_fbs_buf,
                                               static_cast<spi_flash_mmap_handle_t *>(this->m_mmap_handle)));
#endif
        } else {
            ESP_LOGE(TAG, "Can not find %s in partition table", name);
        }
    } else if (m_location == MODEL_LOCATION_IN_SDCARD) {
        // TODO
    }
}

FbsLoader::~FbsLoader()
{
    if (m_location == MODEL_LOCATION_IN_FLASH_PARTITION) {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        esp_partition_munmap(*static_cast<esp_partition_mmap_handle_t *>(this->m_mmap_handle)); // support esp-idf v5
#else
        spi_flash_munmap(*static_cast<spi_flash_mmap_handle_t *>(this->m_mmap_handle)); // support esp-idf v4
#endif
        if (this->m_mmap_handle) {
            free(this->m_mmap_handle);
            this->m_mmap_handle = nullptr;
        }
    } else if (m_location == MODEL_LOCATION_IN_SDCARD) {
        // TODO
    }
}

FbsModel *FbsLoader::load(const int model_index, const uint8_t *key)
{
    if (this->m_fbs_buf == nullptr) {
        ESP_LOGE(TAG, "Model's flatbuffers is empty.");
        return nullptr;
    }

    uint8_t *model_buf = (uint8_t *)m_fbs_buf;
    uint32_t offset = 0;
    fbs_file_format_t format = get_model_format((const char *)m_fbs_buf);
    if (format == FBS_FILE_FORMAT_PDL1) {
        // packed multiple espdl models
        if (get_model_offset_by_index(model_buf, model_index, offset) != ESP_OK) {
            return nullptr;
        }
    } else if (format == FBS_FILE_FORMAT_EDL1) {
        // single espdl model
        if (model_index > 0) {
            ESP_LOGW(TAG, "There are only one model in the flatbuffers, ignore the input model index!");
        }
        offset = 0;
    } else {
        ESP_LOGE(TAG, "Unsupported format, or the model file is corrupted!");
        return nullptr;
    }
    return create_fbs_model(model_buf + offset, key);
}

FbsModel *FbsLoader::load(const uint8_t *key)
{
    return this->load(0, key);
}

FbsModel *FbsLoader::load(const char *model_name, const uint8_t *key)
{
    return this->load(0, key);
}

int FbsLoader::get_model_num()
{
    if (this->m_fbs_buf == nullptr) {
        return 0;
    }

    uint8_t *model_buf = (uint8_t *)m_fbs_buf;
    fbs_file_format_t format = get_model_format((const char *)m_fbs_buf);
    if (format == FBS_FILE_FORMAT_PDL1) {
        // packed multiple espdl models
        uint32_t *header = (uint32_t *)model_buf;
        uint32_t model_num = header[1];
        return model_num;
    } else if (format == FBS_FILE_FORMAT_EDL1) {
        // single espdl model
        return 1;
    } else {
        ESP_LOGE(TAG, "Unsupported format, or the model file is corrupted!");
        return 0;
    }

    return 0;
}

void FbsLoader::list_models()
{
    if (this->m_fbs_buf == nullptr) {
        ESP_LOGE(TAG, "Model's flatbuffers is empty.");
        return;
    }

    fbs_file_format_t format = get_model_format((const char *)m_fbs_buf);
    if (format == FBS_FILE_FORMAT_PDL1) {
        // packed multiple espdl models
        uint32_t *header = (uint32_t *)m_fbs_buf;
        uint32_t model_num = header[1];
        for (int i = 0; i < model_num; i++) {
            uint32_t name_offset = header[2 + 3 * i + 1];
            uint32_t name_length = header[2 + 3 * i + 2];
            std::string name((const char *)m_fbs_buf + name_offset, name_length);
            ESP_LOGI(TAG, "model name: %s, index:%d", name.c_str(), i);
        }
    } else if (format == FBS_FILE_FORMAT_EDL1) {
        ESP_LOGI(TAG, "There are only one model in the flatbuffers without model name.");
    }
}

} // namespace fbs
