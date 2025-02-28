/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ST7735.h"
#include "GFX_FUNCTIONS.h"
#include "fonts.h"
#include "ov2640.h"
#include "Bitcoin/utility/crypto/bip39.h"
#include "Bitcoin/utility/crypto/bip39_english.h"
#include "Bitcoin/utility/crypto/sha2.h"
#include "Bitcoin/Bitcoin.h"
#include "Bitcoin/PSBT.h"
#include "Bitcoin/Conversion.h"
#include "Bitcoin/Hash.h"
#include "optiga/optiga_util.h"
#include "optiga/pal/pal_i2c.h"
#include "optiga/pal/pal_gpio.h"
#include "optiga/pal/pal_os_event.h"
#include "optiga/pal/pal_ifx_i2c_config.h"
#include "aes.h"
#include "quirc/quirc.h"
#include "qrcode.h"
#include "decode_dma.h"
#include "logo_img.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <cstring>
#include <map>
#include <iostream>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

DCMI_HandleTypeDef hdcmi;
DMA_HandleTypeDef hdma_dcmi;

DMA2D_HandleTypeDef hdma2d;

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

JPEG_HandleTypeDef hjpeg;
MDMA_HandleTypeDef hmdma_jpeg_infifo_th;
MDMA_HandleTypeDef hmdma_jpeg_outfifo_th;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim1;

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_tx;

DMA_HandleTypeDef hdma_memtomem_dma1_stream0;
/* USER CODE BEGIN PV */
#define MAX_STRINGS 100
#define MAX_STRING_LENGTH 65  // Maximum tring length + 1 for null terminator
#define BROADCAST_CHUNK 28
#define MAX_BROADCAST_LENGTH BROADCAST_CHUNK+1
#define TOTAL_MAX_LENGTH (MAX_STRINGS * (MAX_STRING_LENGTH - 1) + 1)
#define IMAGE_WIDTH  320
#define IMAGE_HEIGHT 240
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 160
static volatile optiga_lib_status_t optiga_lib_status;
static const std::string base64_chars ="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static JPEG_ConfTypeDef JPEG_Info;
static struct quirc *qr = NULL;
static uint8_t image_data[76800];
static uint8_t stream_entropy[2048];
static uint8_t dice_entropy[100] = {0};
static uint8_t *jpg_image_data = NULL;
static const char * found_matches[100];
enum imageResolution img_res=RES_320X240;
extern __IO uint32_t Jpeg_HWDecodingEnd;
char psbt_strings[MAX_STRINGS][MAX_STRING_LENGTH];  // Array to store individual strings
char broadcast_strings[MAX_STRINGS][MAX_BROADCAST_LENGTH];
int total_collected = 0;
int total_broadcast_qr =0;
int global_tick =0;
bool unlock = false;
bool restore = false;
bool pinlock = false;
std::string mnem;
std::string mnem_options[5];
std::string check_mnem;
aes_256_context_t aes_context;
optiga_util_t* optiga_util_instance;
uint8_t frame_buffer[RES_320X240] = { 0 };
uint8_t mutex = 0;
uint8_t header_found = 0;
uint8_t menu_state =0;
uint8_t old_state = 255;
uint8_t second_ms =0;
uint8_t old_sMS =0;
uint8_t lock_mult = 1;
uint8_t tx_outputs = 0;
uint8_t output_pos =0;
uint8_t max_dice =64;
uint8_t dice_pos =0;
uint8_t entropy_trk = 0;
uint8_t old_et =255;
uint8_t* jpeg_data;       // Pointer to the JPEG data
uint8_t hash_hold[32];
uint8_t seed_phase = 0;
uint8_t option_state =0;
uint8_t seed_pos = 0;
uint8_t pincode[8];
uint8_t checkcode[8];
uint8_t pinpos =0;
uint8_t letter1_pos=0;
uint8_t letter2_pos=0;
uint8_t restore_word_pos =0;
uint8_t restore_word_state =0;
uint8_t found_max = 0;
uint8_t found_word_pos=0;
uint16_t buffer_pointer = 0;
uint16_t jpeg_start_index=0;
uint16_t tick_count = 0;
uint16_t qr_state = 0;
uint32_t current_time_ms = 0;
int32_t jpeg_data_size;   // Size of the JPEG data
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_MDMA_Init(void);
static void MX_DCMI_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM1_Init(void);
static void MX_JPEG_Init(void);
static void MX_DMA2D_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void save_info();
uint16_t read_uint16_le(const std::vector<uint8_t>& data, size_t offset) {
    return (static_cast<uint16_t>(data[offset])) |
           (static_cast<uint16_t>(data[offset + 1]) << 8);
}
uint64_t read_uint64_le(const std::vector<uint8_t>& data, size_t offset) {
    return (static_cast<uint64_t>(data[offset])) |
           (static_cast<uint64_t>(data[offset + 1]) << 8) |
           (static_cast<uint64_t>(data[offset + 2]) << 16) |
           (static_cast<uint64_t>(data[offset + 3]) << 24) |
           (static_cast<uint64_t>(data[offset + 4]) << 32) |
           (static_cast<uint64_t>(data[offset + 5]) << 40) |
           (static_cast<uint64_t>(data[offset + 6]) << 48) |
           (static_cast<uint64_t>(data[offset + 7]) << 56);
}
bool is_base_64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}
std::vector<uint8_t> base64_decode(const std::string& encoded_string) {
  int in_len = encoded_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  uint8_t char_array_4[4], char_array_3[3];
  std::vector<uint8_t> ret;

  while (in_len-- && (encoded_string[in_] != '=') && is_base_64(encoded_string[in_])) {
    char_array_4[i++] = base64_chars.find(encoded_string[in_]); in_++;
    if (i == 4) {
      char_array_3[0] =  (char_array_4[0] << 2)        + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) +   char_array_4[3];
      for (i = 0; (i < 3); i++)
        ret.push_back(char_array_3[i]);
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 4; j++)
      char_array_4[j] = 0;

    char_array_3[0] =  (char_array_4[0] << 2)        + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) +   char_array_4[3];

    for (j = 0; (j < i - 1); j++)
      ret.push_back(char_array_3[j]);
  }

  return ret;
}
std::string base64_encode_from_hex(const std::string& hex_string) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex_string.size(); i += 2) {
        uint8_t byte = std::stoi(hex_string.substr(i, 2), nullptr, 16);
        bytes.push_back(byte);
    }
    std::string base64_result;
    int i = 0, j = 0;
    uint8_t char_array_3[3], char_array_4[4];
    size_t input_len = bytes.size();
    size_t input_pos = 0;

    while (input_len--) {
        char_array_3[i++] = bytes[input_pos++];
        if (i == 3) {
            char_array_4[0] =  (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] =   char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++)
                base64_result += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++)
            char_array_3[j] = 0;

        char_array_4[0] =  (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] =   char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            base64_result += base64_chars[char_array_4[j]];

        while ((i++ < 3))
            base64_result += '=';
    }

    return base64_result;
}
uint16_t convert_rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t color = ((r & 0xF8) << 8) | // Red: 5 bits, shifted to bits 11-15
                     ((g & 0xFC) << 3) | // Green: 6 bits, shifted to bits 5-10
                     ((b & 0xF8) >> 3);  // Blue: 5 bits, shifted to bits 0-4
    return color;
}
void hash_entropy(){
	for (size_t i = 0; i < 32; i++) {
		hash_hold[i] = 0;
	}
	size_t data_size = sizeof(image_data) / sizeof(image_data[0]);
	SHA256_CTX ctx;
	sha256_Init(&ctx);
	sha256_Update(&ctx, image_data, data_size);
	sha256_Final(&ctx, hash_hold);
	memcpy(&stream_entropy[entropy_trk * 32], hash_hold, 32);
	entropy_trk++;
	if(entropy_trk == 64){
		SHA256_CTX ctx_2;
		sha256_Init(&ctx_2);
		sha256_Update(&ctx_2, stream_entropy, 2048);
		sha256_Final(&ctx_2, hash_hold);
		menu_state = 13;
	}
}
void hash_dice_entropy(){
	for (size_t i = 0; i < 32; i++) {
		hash_hold[i]=0;
	}
	SHA256_CTX ctx;
	sha256_Init(&ctx);
	sha256_Update(&ctx, dice_entropy, max_dice);
	sha256_Final(&ctx, hash_hold);
	menu_state =13;
}
void clear_memory(){
	mnem = "";
	check_mnem = "";
	for(int i = 0; i < 2048; i++){
		if(i < 5){
			mnem_options[i] = "";
		}
		if(i < 8){
			pincode[i] = 0;
			checkcode[i] = 0;
		}
		if(i < 32){
			hash_hold[i] = 0;
		}
		if(i < 100){
			dice_entropy[i] = 0;
		}
		stream_entropy[i] = 0;
	}
}
void draw_logo(){
	for(int y = 0; y < 64; y++){
		for(int x = 0; x < 64; x++){
			int index = 64*y + x;
			if(logo_img[index] == 0x01){
				ST7735_DrawPixel(x + 33, y + 25, ORANGE);
			}
		}
	}
}
void draw_photo_gray(){
	for (int y = 0; y < IMAGE_HEIGHT; y++) {
		for (int x = 0; x < IMAGE_WIDTH; x++) {
			if(x%6 == 0 && x/2 < SCREEN_WIDTH && y%6 == 0 && y/2<SCREEN_HEIGHT ){
				int index = ((IMAGE_HEIGHT-y-1) * IMAGE_WIDTH + (IMAGE_WIDTH-x-1));
				uint8_t gray = image_data[index];
				uint16_t color = convert_rgb888_to_rgb565(gray, gray, gray);
				int half_x = x/2;
				int half_y = y/2 + 23;
				ST7735_FillRectangle(half_x, half_y, 3, 3, color);
			}
		}
	}
}
void extract_image_data(bool entropy){
    uint8_t *pData = jpg_image_data;
    // Determine MCU parameters based on ChromaSubsampling
    int MCU_width_pixels, MCU_height_pixels, num_Y_blocks;
    int blocks_in_mcu_h, blocks_in_mcu_v;
    int chroma_subsample_h, chroma_subsample_v;

    if (JPEG_Info.ChromaSubsampling == JPEG_420_SUBSAMPLING) {
        // 4:2:0 subsampling
        chroma_subsample_h = 2;
        chroma_subsample_v = 2;
        MCU_width_pixels = 16;
        MCU_height_pixels = 16;
        num_Y_blocks = 4;
        blocks_in_mcu_h = 2;
        blocks_in_mcu_v = 2;
    } else if (JPEG_Info.ChromaSubsampling == JPEG_422_SUBSAMPLING) {
        // 4:2:2 subsampling
        chroma_subsample_h = 2;
        chroma_subsample_v = 1;
        MCU_width_pixels = 16;
        MCU_height_pixels = 8;
        num_Y_blocks = 2;
        blocks_in_mcu_h = 2;
        blocks_in_mcu_v = 1;
    } else if (JPEG_Info.ChromaSubsampling == JPEG_444_SUBSAMPLING) {
        // 4:4:4 subsampling
        chroma_subsample_h = 1;
        chroma_subsample_v = 1;
        MCU_width_pixels = 8;
        MCU_height_pixels = 8;
        num_Y_blocks = 1;
        blocks_in_mcu_h = 1;
        blocks_in_mcu_v = 1;
    } else {
        // Default to 4:2:0
        chroma_subsample_h = 2;
        chroma_subsample_v = 2;
        MCU_width_pixels = 16;
        MCU_height_pixels = 16;
        num_Y_blocks = 4;
        blocks_in_mcu_h = 2;
        blocks_in_mcu_v = 2;
    }

    int MCU_width = (IMAGE_WIDTH + MCU_width_pixels - 1) / MCU_width_pixels;
    int MCU_height = (IMAGE_HEIGHT + MCU_height_pixels - 1) / MCU_height_pixels;
    int chroma_block_width = (blocks_in_mcu_h * 8) / chroma_subsample_h;

    for (int mcu_y = 0; mcu_y < MCU_height; mcu_y++) {
        for (int mcu_x = 0; mcu_x < MCU_width; mcu_x++) {
            uint8_t *Y_blocks[num_Y_blocks];
            for (int i = 0; i < num_Y_blocks; i++) {
                if (i == 0){
                    Y_blocks[i] = pData;
                }else{
                    Y_blocks[i] = Y_blocks[i - 1] + 64;
                }
            }
            uint8_t *Cb_block = Y_blocks[num_Y_blocks - 1] + 64;
            uint8_t *Cr_block = Cb_block + 64;
            pData = Cr_block + 64;
            for (int block_y = 0; block_y < blocks_in_mcu_v; block_y++) {
                for (int block_x = 0; block_x < blocks_in_mcu_h; block_x++) {
                    int block_index = block_y * blocks_in_mcu_h + block_x;
                    uint8_t *Y_block = Y_blocks[block_index];
                    for (int y = 0; y < 8; y++) {
                        for (int x = 0; x < 8; x++) {
                            int pixel_x = mcu_x * MCU_width_pixels + block_x * 8 + x;
                            int pixel_y = mcu_y * MCU_height_pixels + block_y * 8 + y;
							uint8_t Y_val = Y_block[y * 8 + x];
							int chroma_x = (block_x * 8 + x) / chroma_subsample_h;
							int chroma_y = (block_y * 8 + y) / chroma_subsample_v;
							int chroma_index = chroma_y * chroma_block_width + chroma_x;
							uint8_t Cb = Cb_block[chroma_index];
							uint8_t Cr = Cr_block[chroma_index];
							int16_t r = Y_val + ((179 * (Cr - 128)) >> 7);
							int16_t g = Y_val - ((44 * (Cb - 128) + 91 * (Cr - 128)) >> 7);
							int16_t b = Y_val + ((227 * (Cb - 128)) >> 7);
							r = (r < 0) ? 0 : (r > 255) ? 255 : r;
							g = (g < 0) ? 0 : (g > 255) ? 255 : g;
							b = (b < 0) ? 0 : (b > 255) ? 255 : b;
							uint8_t gray_pixel = (r * 38 + g * 75 + b * 15) >> 8;
							if(pixel_x < IMAGE_WIDTH && pixel_y < IMAGE_HEIGHT){
								int dst_index = pixel_x + IMAGE_WIDTH * pixel_y;
								image_data[dst_index] = gray_pixel;
							}else{
								//menu_state = 22;
							}
							if(entropy){
								if (pixel_x % 2 == 0 && pixel_x / 2 < SCREEN_WIDTH &&  pixel_y % 2 == 0 && pixel_y / 2 < SCREEN_HEIGHT) {
									uint16_t color = convert_rgb888_to_rgb565(r, g, b);
									int half_x = pixel_x / 2;
									int half_y = pixel_y / 2 + 23;
									half_y = 165 - half_y;
									half_x = 128 - half_x;
									ST7735_DrawPixel(half_x, half_y, color);
								}
							}
                        }
                    }
                }
            }
        }
    }
}
void scan_qr_code_end() {
    quirc_end(qr);
    int num_codes = quirc_count(qr);
    current_time_ms = HAL_GetTick();
    if (num_codes == 0) {
        return;
    } else {
        struct quirc_code code;
        struct quirc_data data;
        bool decoded = false;
        for (int i = 0; i < num_codes; i++) {
            quirc_extract(qr, i, &code);
            if (!quirc_decode(&code, &data)) {
                char payload[QUIRC_MAX_PAYLOAD+1];
                memcpy(payload, data.payload, data.payload_len);
                payload[data.payload_len] = '\0';
                char* separator = strchr(payload, ')');
                std::string collected_string = "";
                int x =0;
                int n =0;
                if (separator) {
					*separator = '\0';  // Null-terminate the first part
					std::string first_part(payload);
					std::string second_part(separator + 1);
					size_t slash_pos = first_part.find('/');
					x = std::stoi(first_part.substr(1, slash_pos - 1));
					n = std::stoi(first_part.substr(slash_pos + 1));
					if (x < 0 || x >= 25) {
						return;
					}
					if (n <= 0 || n > 25) {
						return;
					}
					if(psbt_strings[x][0] == '\0'){
						strncpy(psbt_strings[x], second_part.c_str(), MAX_STRING_LENGTH - 1);
						psbt_strings[x][MAX_STRING_LENGTH - 1] = '\0';
						total_collected++;
					}
                }
                if(total_collected == n){
                	menu_state = 4;
                }
                collected_string = std::to_string(total_collected)+"/"+std::to_string(n);
                int total_w = (total_collected*128)/n;
                ST7735_FillRectangle(0, 144, total_w, 16, ORANGE);
                ST7735_WriteString(53, 144,collected_string.c_str() , Font_7x10, BLACK, CREAM);
                decoded = true;
                break;
            }
        }
        if (!decoded) {
            return;
        }
    }
}
void fast_decode_jpeg(uint8_t* jpeg_buffer, int buffer_size,bool entropy,bool warmup){
	jpeg_data = jpeg_buffer;
	jpeg_data_size = buffer_size;
	int y_plane_size = IMAGE_WIDTH * IMAGE_HEIGHT; // One Y sample per pixel
	int chroma_width = IMAGE_WIDTH / 2;
	int chroma_height = IMAGE_HEIGHT / 2;
	int chroma_plane_size = chroma_width * chroma_height; // One Cb/Cr sample per 2x2 block
	int total_size = y_plane_size + 2 * chroma_plane_size;
	jpg_image_data = (uint8_t*)malloc(total_size);
	if (jpg_image_data == NULL) {
	    return;
	}
	uint32_t result = JPEG_Decode_DMA(&hjpeg, (uint32_t)jpeg_data, jpeg_data_size , (uint32_t)jpg_image_data);
	if(result != 0){ return;}
	while(Jpeg_HWDecodingEnd == 0){}
	HAL_JPEG_GetInfo(&hjpeg, &JPEG_Info);

	int imgw = JPEG_Info.ImageWidth;
	int imgh = JPEG_Info.ImageHeight;
	extract_image_data(entropy);
	free(jpg_image_data);

	if (!qr) {
		qr = quirc_new();
		if (!qr) {
			if (qr != NULL) {
				quirc_destroy(qr);
				qr = NULL;
			}
			return;
		}
		if (quirc_resize(qr, imgw, imgh) < 0) {
			if (qr != NULL) {
				quirc_destroy(qr);
				qr = NULL;
			}
			return;
		}
	}

	uint8_t *image_buffer = quirc_begin(qr, &imgw, &imgh);
	if(!image_buffer){
		return;
	}
	memcpy(image_buffer, image_data, imgw*imgh);

	scan_qr_code_end();
	if (qr != NULL) {
		quirc_destroy(qr);
		qr = NULL;
	}
	if(entropy){
		hash_entropy();
	}else{
		if(!warmup){
			draw_photo_gray();
		}
	}
}
static void optiga_util_callback(void *context, optiga_lib_status_t return_status){
    optiga_lib_status = return_status;
}
void vprint(const char *fmt, va_list argp) {
	char string[200];
	if (0 < vsprintf(string, fmt, argp)) // build string
			{
		HAL_UART_Transmit(&huart1, (uint8_t*) string, strlen(string), 0xffffff); // send message via UART
	}
}
void my_printf(const char *fmt, ...) {
	va_list argp;
	va_start(argp, fmt);
	vprint(fmt, argp);
	va_end(argp);
}
void draw_qr_code(QRCode qrcode){
  uint16_t blocksize = 4;
  for (uint8_t y = 0; y < qrcode.size; y++) {
      for (uint8_t x = 0; x < qrcode.size; x++) {
          if(qrcode_getModule(&qrcode, x, y)){
        	  ST7735_FillRectangle(uint16_t(x*blocksize+6), uint16_t(y*blocksize+8), blocksize, blocksize, BLACK);
          }else{
        	  ST7735_FillRectangle(uint16_t(x*blocksize+6), uint16_t(y*blocksize+8), blocksize, blocksize, CREAM);
          }
      }
  }
}
void scan_for_qr(bool entropy_picture, bool warmup){
	memset(frame_buffer, 0, sizeof frame_buffer);
	OV2640_CaptureSnapshot((uint32_t) frame_buffer, img_res);
	buffer_pointer=0;
	bool decode = false;
	while (1) {
		if (header_found == 0 && frame_buffer[buffer_pointer] == 0xFF
				&& frame_buffer[buffer_pointer + 1] == 0xD8) {
			header_found = 1;
			jpeg_start_index = buffer_pointer;
		}
		if (header_found == 1 && frame_buffer[buffer_pointer] == 0xFF
				&& frame_buffer[buffer_pointer + 1] == 0xD9) {
			buffer_pointer = buffer_pointer + 2;
			decode = true;
			header_found = 0;
			break;
		}
		if (buffer_pointer >= 15533) {
			break;
		}
		buffer_pointer++;
	}
	buffer_pointer--;
	if(decode){
		jpeg_data = &frame_buffer[jpeg_start_index];
		jpeg_data_size = buffer_pointer - jpeg_start_index;
		fast_decode_jpeg(jpeg_data, jpeg_data_size, entropy_picture, warmup);
	}
	buffer_pointer = 0;
	mutex = 0;
	header_found =0;
}
void reset_chunks(){
	for(int i = 0; i < MAX_STRINGS; i++){
		broadcast_strings[i][0] = '\0';
	}
}
void reset_psbt_strings() {
    for (int i = 0; i < MAX_STRINGS; i++) {
        psbt_strings[i][0] = '\0';  // Reset each string to empty
    }
}
int init_optiga(){
	optiga_util_instance = optiga_util_create(0, optiga_util_callback, NULL);
    if(optiga_util_instance == NULL){ return 0; }
    optiga_lib_status = OPTIGA_LIB_BUSY;
    optiga_lib_status_t status_code = optiga_util_open_application(optiga_util_instance, 0);
    if (status_code != OPTIGA_LIB_SUCCESS){ return 0; }
    while (optiga_lib_status == OPTIGA_LIB_BUSY){}
    if (optiga_lib_status != OPTIGA_LIB_SUCCESS){ return 0; }
    return 1;
}
int write_to_se(optiga_util_t* optiga_util_instance,uint8_t* write_data,uint16_t data_len,uint16_t sector){
	optiga_lib_status = OPTIGA_LIB_BUSY;
	optiga_lib_status_t status_code =  optiga_util_write_data(optiga_util_instance, sector, OPTIGA_UTIL_ERASE_AND_WRITE, 0, write_data, data_len);
	if (status_code != OPTIGA_LIB_SUCCESS){ return 0; }
	__HAL_TIM_SET_AUTORELOAD(&htim1, 1000);
	__HAL_TIM_SET_COUNTER(&htim1, 0);
	HAL_TIM_Base_Start_IT(&htim1);
	while (optiga_lib_status == OPTIGA_LIB_BUSY){}
	if (optiga_lib_status != OPTIGA_LIB_SUCCESS){ return 0; }
	return 1;
}
int read_from_se(optiga_util_t* optiga_util_instance,uint8_t* read_data,uint16_t read_len, uint16_t sector){
	optiga_lib_status = OPTIGA_LIB_BUSY;
	optiga_lib_status_t status_code = optiga_util_read_data(optiga_util_instance, sector,0, read_data, &read_len);
	if (status_code != OPTIGA_LIB_SUCCESS){ return 0; }
	__HAL_TIM_SET_AUTORELOAD(&htim1, 1000);
	__HAL_TIM_SET_COUNTER(&htim1, 0);
	HAL_TIM_Base_Start_IT(&htim1);
	while (optiga_lib_status == OPTIGA_LIB_BUSY){}
	if (optiga_lib_status != OPTIGA_LIB_SUCCESS){ return 0; }
	return 1;
}
void combine_psbt() {
	bool multi_sig = false;
	std::string psbt_str = "";
	for (int i = 0; i < total_collected; i++) {
		std::string psbt_str_i(psbt_strings[i]);
		psbt_str = psbt_str + psbt_str_i;
	}
	std::string tx_half = "";
	std::string extrad_half = "";
	std::string mpub_keys = "";
	std::vector<uint8_t> mpubData;
	const char* pos = strchr(psbt_str.c_str(), ':');
	if (pos != nullptr) {
		size_t posIndex = pos - psbt_str.c_str();
		tx_half = psbt_str.substr(0, posIndex);
		extrad_half = psbt_str.substr(posIndex + 1);
	}

	//Multi sig
	const char* second_pos = strchr(extrad_half.c_str(),':');
	if (second_pos != nullptr){
		multi_sig = true;
		std::string temp = "";
		size_t posIndex = second_pos - extrad_half.c_str();
		temp = extrad_half.substr(0, posIndex);
		mpub_keys = extrad_half.substr(posIndex + 1);
		extrad_half = temp;
	}
	//Multi sig



	std::vector<uint8_t> txData = base64_decode(tx_half);
	std::vector<uint8_t> extraData = base64_decode(extrad_half);
	uint8_t mThresh=0;
	uint8_t mPubLen=0;
	std::vector<PublicKey> pubkeys;
	if(multi_sig){
		mpubData = base64_decode(mpub_keys);
		mThresh = mpubData[0];
		mPubLen = mpubData[1];
		pubkeys.reserve(mPubLen);
		size_t offset = 2;
		for (uint8_t i = 0; i < mPubLen; i++) {
			// Ensure we have 33 bytes available for this public key.
			if (offset + 33 > mpubData.size()) {
				menu_state = 22;
				return;
			}
			const uint8_t* pubkeyBytes = mpubData.data() + offset;
			PublicKey pubkey(pubkeyBytes);
			pubkeys.push_back(pubkey);
			offset += 33;
		}
	}

	size_t numInputs = extraData.size() / 12;
	std::string inputDerivationPaths[numInputs];
	uint64_t txAmts[numInputs];
	for (size_t i = 0; i < numInputs; i++) {
		size_t offset = i * 12;
		uint16_t a = read_uint16_le(extraData, offset);
		uint16_t b = read_uint16_le(extraData, offset + 2);
		std::string deri = std::to_string(a) +"/"+std::to_string(b);
		inputDerivationPaths[i] = deri;
		txAmts[i] = read_uint64_le(extraData, offset + 4);
	}
	if (txData.empty()) {
		menu_state = 22;
		return;
	}
	uint8_t test_key[32];
	memcpy(test_key,        pincode, 8);
	memcpy(test_key + 8,    pincode, 8);
	memcpy(test_key + 16,   pincode, 8);
	memcpy(test_key + 24,   pincode, 8);
	uint8_t arr[32] ={0};
	read_from_se(optiga_util_instance, arr, sizeof(arr), 0xF1D0);
	uint8_t half_key1[16];
	uint8_t half_key2[16];
	memcpy(half_key1, arr, 16);
	memcpy(half_key2, arr + 16, 16);
	aes_256_init(&aes_context, test_key);
	aes_256_decrypt(&aes_context, half_key1);
	aes_256_decrypt(&aes_context, half_key2);
	uint8_t combined_hash[32];
	memcpy(combined_hash, half_key1, 16);
	memcpy(combined_hash + 16, half_key2, 16);
	std::string mn = mnemonicFromEntropy(combined_hash, sizeof(combined_hash));
	HDPrivateKey hd(mn, "", &Mainnet);
	HDPrivateKey account = hd.derive("m/84'/0'/0'/");

	//HDPublicKey xpub = account.xpub();
	Tx tx;

	size_t parsedBytes = tx.parse(txData.data(), txData.size());
	if (parsedBytes == 0) {
		menu_state = 22;
		return;
	}
	for (size_t i = 0; i < tx.inputsNumber; i++) {
		std::string derivationPath = inputDerivationPaths[i];
		uint64_t amount = txAmts[i];
		if(multi_sig){
			Script multisigScript = multi(mThresh, pubkeys.data(), mPubLen);
			HDPrivateKey child_xprv1 = account.derive("m/0/0");
			tx.signSegwitInput(i, child_xprv1, multisigScript, amount, P2WSH, SigHashType::SIGHASH_ALL);
		}else{
			HDPrivateKey key = account.derive(derivationPath.c_str());
			Script scriptPubKey(key.publicKey(), P2WPKH);
			tx.signSegwitInput(i, key, scriptPubKey, amount);
		}
	}

	std::string signedTxString = tx.serialize();
	std::string signed_base64 = base64_encode_from_hex(signedTxString);
	if (signedTxString.empty()) {
		menu_state = 23;
		return;
	}

	int chunkSize = BROADCAST_CHUNK;
	int fixedPrefixLength = 6;
	int effectiveChunkSize = chunkSize - fixedPrefixLength;
	int numChunks = (signed_base64.length() + effectiveChunkSize - 1) / effectiveChunkSize;
	total_broadcast_qr = numChunks;
	reset_chunks();
	for (int i = 0; i < numChunks; i++) {
		char prefix[16];
		std::snprintf(prefix, sizeof(prefix), "%02d/%02d:", i + 1, numChunks);
		int availableSpace = chunkSize - fixedPrefixLength;
		int startIndex = i * effectiveChunkSize;
		std::string chunkContent = signed_base64.substr(startIndex, availableSpace);
		std::strncpy(broadcast_strings[i], prefix, sizeof(broadcast_strings[i])-1);
		std::strncpy(broadcast_strings[i] + fixedPrefixLength, chunkContent.c_str(), availableSpace);
		broadcast_strings[i][chunkSize] = '\0';
	}
	clear_memory();
}
void gen_seed_phrase(){
	std::string mn = mnemonicFromEntropy(hash_hold, sizeof(hash_hold));
	mnem = mn;
}
void draw_seed_phrase(int section){
	std::istringstream iss(mnem);
	std::vector<std::string> words;
	std::string word;
	while (iss >> word) {
		words.push_back(word);
	}
	for (int i = 0;i < 8; i++) {
		std::string indexedWord = std::to_string(i+8*section+1) + ") " + words[i+8*section];
		ST7735_WriteString(25, 30+12*i, indexedWord.c_str(), Font_7x10, BLACK,CREAM);
	}
}
uint16_t get_psuedo_random(uint16_t min, uint16_t max) {
    return (uint16_t)(min + (rand() % (max - min + 1)));
}
void draw_option_state(){
	ST7735_FillRectangle(0,23,128,137,CREAM);
	std::string select_str = "Select word #"+std::to_string(seed_pos+1);
	ST7735_WriteString(17,27,select_str.c_str(),Font_7x10,BLACK,CREAM);
	ST7735_FillRectangle(0,39,128,1,ORANGE);
	for(int i =0; i < 5;i++){
		if(i==option_state){
			ST7735_FillRectangle(0,39+12*i,128,12,ORANGE);
			ST7735_WriteString(40,41+12*i,mnem_options[i].c_str(),Font_7x10,CREAM,ORANGE);
		}else{
			ST7735_WriteString(40,41+12*i,mnem_options[i].c_str(),Font_7x10,BLACK,CREAM);
		}
	}
	ST7735_FillRectangle(0,99,128,1,ORANGE);
}
void gen_options(){
	std::istringstream iss(mnem);
	std::vector<std::string> words;
	std::string word;
	while (iss >> word) {
		words.push_back(word);
	}
	uint16_t chosen = get_psuedo_random(0, 4);
	for(int i = 0; i < 5; i++){
		mnem_options[i]="";
	}
	for(int i = 0; i < 5; i++){
		if(i == chosen){
			mnem_options[i] = words[seed_pos];
		}else{
			uint16_t word_selc = 0;
			bool not_correct = true;
			while(not_correct){
				not_correct = false;
				word_selc = get_psuedo_random(0, 2047);
				if(wordlist[word_selc] == words[seed_pos]){
					not_correct = true;
				}
				for(int x = 0; x < 5; x++){
					if(mnem_options[x] == wordlist[word_selc]){
						not_correct = true;
					}
				}
			}
			mnem_options[i] = wordlist[word_selc];
		}
	}
}
void reset_pin_arr(){
	for(int i = 0; i < 8; i++){
		pincode[i] = 10;
	}
}
void reset_check_pin(){
	for(int i = 0; i < 8; i++){
		checkcode[i] = 10;
	}
}
bool pin_length_check(){
	int x_count = 0;
	for(int i = 0; i < 8; i++){
		if(pincode[i] == 10){
			x_count++;
		}
	}
	if(x_count > 4){
		return false;
	}else{
		return true;
	}
}
void draw_pinselect(){
	for(int i = 0; i < 8; i++){
		if(pincode[i] != 10){
			ST7735_WriteString(10+14*i,60,std::to_string(pincode[i]).c_str(),Font_11x18,BLACK,CREAM);
		}else{
			ST7735_WriteString(10+14*i,60,"X",Font_11x18,BLACK,CREAM);
		}
	}
	ST7735_FillRectangle(0,78,128,2,CREAM);
	ST7735_FillRectangle(10+14*pinpos,78,11,2,BLACK);
}
void draw_diceselect(){
	ST7735_FillRectangle(0,39,128,121,CREAM);
	if(dice_entropy[dice_pos] > 8){
		ST7735_WriteString(45,75,std::to_string(dice_entropy[dice_pos]+1).c_str(),Font_16x26,BLACK,CREAM);
	}else{
		ST7735_WriteString(55,75,std::to_string(dice_entropy[dice_pos]+1).c_str(),Font_16x26,BLACK,CREAM);
	}
	int total_w = (dice_pos*128)/max_dice;
	ST7735_FillRectangle(0,144,total_w,16,GREEN);
}
void draw_checkpin(){
	for(int i = 0; i < 8; i++){
		if(checkcode[i]!= 10){
			ST7735_WriteString(10+14*i,60,std::to_string(checkcode[i]).c_str(),Font_11x18,BLACK,CREAM);
		}else{
			ST7735_WriteString(10+14*i,60,"X",Font_11x18,BLACK,CREAM);
		}
	}
	ST7735_FillRectangle(0,78,128,2,CREAM);
	ST7735_FillRectangle(10+14*pinpos,78,11,2,BLACK);
}
void save_info(){
	uint8_t test_key[32];
	memcpy(test_key,        pincode, 8);
	memcpy(test_key + 8,    pincode, 8);
	memcpy(test_key + 16,   pincode, 8);
	memcpy(test_key + 24,   pincode, 8);
	uint8_t half_key1[16];
	uint8_t half_key2[16];
	memcpy(half_key1, hash_hold, 16);
	memcpy(half_key2, hash_hold + 16, 16);
	aes_256_init(&aes_context, test_key);
	aes_256_encrypt(&aes_context,half_key1);
	aes_256_encrypt(&aes_context,half_key2);
	uint8_t combined_hash[32];
	memcpy(combined_hash, half_key1, 16);
	memcpy(combined_hash + 16, half_key2, 16);
	write_to_se(optiga_util_instance,combined_hash,sizeof(combined_hash),0xF1D0);
	uint8_t pin_hash[32] = {0};
	SHA256_CTX ctx;
	sha256_Init(&ctx);
	sha256_Update(&ctx, pincode, 8);
	sha256_Final(&ctx, pin_hash);
	write_to_se(optiga_util_instance,pin_hash,sizeof(pin_hash),0xF1D6);
	std::string mn = mnemonicFromEntropy(hash_hold, sizeof(hash_hold));
	mnem = mn;
	HDPrivateKey hd(mn, "",&Mainnet);
	HDPublicKey hdxpub = hd.xpub();
	HDPrivateKey account = hd.derive("m/84'/0'/0'/");
	HDPublicKey xpub = account.xpub();
	std::string xpub_str = xpub.toString();
	std::string fingerprint = hd.fingerprint();
	std::string address_str = xpub.derive("m/0/0").address();
	std::string final_str = xpub_str+":"+fingerprint;
	uint16_t write_len = final_str.size();
	uint16_t address_write_len = address_str.size();
	uint8_t write_data[write_len];
	uint8_t address_write_data[address_write_len];
	memcpy(write_data, final_str.data(), write_len);
	memcpy(address_write_data, address_str.data(), address_write_len);
	write_to_se(optiga_util_instance,write_data,sizeof(write_data),0xF1D2);
	write_to_se(optiga_util_instance,address_write_data,sizeof(address_write_data),0xF1D4);
	uint8_t write_len_bytes[2];
	uint8_t a_write_len_bytes[2];
	write_len_bytes[0] = (write_len >> 8) & 0xFF;  // Most significant byte
	write_len_bytes[1] = write_len & 0xFF;
	a_write_len_bytes[0] = (address_write_len >> 8) & 0xFF;  // Most significant byte
	a_write_len_bytes[1] = address_write_len & 0xFF;
	write_to_se(optiga_util_instance,write_len_bytes,sizeof(write_len_bytes),0xF1D3);
	write_to_se(optiga_util_instance,a_write_len_bytes,sizeof(a_write_len_bytes),0xF1D5);
	uint8_t read_len_bytes[2];
	read_from_se(optiga_util_instance,read_len_bytes,sizeof(read_len_bytes),0xF1D3);
	uint16_t read_len = (static_cast<uint16_t>(read_len_bytes[0]) << 8) | read_len_bytes[1];
	uint8_t read_data[read_len];
	read_from_se(optiga_util_instance,read_data,sizeof(read_data),0xF1D2);
	std::string result(reinterpret_cast<char*>(read_data), read_len);
	uint8_t i2c_write_d2[1] ={7};
	write_to_se(optiga_util_instance,i2c_write_d2,sizeof(i2c_write_d2),0xF1D1);
	uint8_t settings_data[2] ={0};
	settings_data[0] = 0;
	settings_data[1] = 1;
	lock_mult = 1;
	write_to_se(optiga_util_instance,settings_data,sizeof(settings_data),0xF1D8);
	uint8_t i2c_write_pincounts[1] ={5};
	write_to_se(optiga_util_instance,i2c_write_pincounts,sizeof(i2c_write_pincounts),0xF1D9);
	uint8_t i2c_read_data[32] ={0};
	read_from_se(optiga_util_instance,i2c_read_data,sizeof(i2c_read_data),0xF1D0);
	uint8_t half_key3[16];
	uint8_t half_key4[16];
	memcpy(half_key3, i2c_read_data, 16);
	memcpy(half_key4, i2c_read_data + 16, 16);
	aes_256_init(&aes_context, test_key);
	aes_256_decrypt(&aes_context,half_key3);
	aes_256_decrypt(&aes_context,half_key4);
	uint8_t combined_hash_2[32];
	memcpy(combined_hash_2, half_key3, 16);
	memcpy(combined_hash_2 + 16, half_key4, 16);
	std::string mn2 = mnemonicFromEntropy(combined_hash_2, sizeof(combined_hash_2));
	HDPrivateKey hd2(mn2, "",&Mainnet);
	HDPublicKey hdxpub2 = hd2.xpub();
	HDPrivateKey account2 = hd2.derive("m/84'/0'/0'/");
	HDPublicKey xpub2 = account2.xpub();
	std::string xpub_str2 = xpub2.toString();
	std::string fingerprint2 = hd2.fingerprint();
	clear_memory();
}
void checkpin(){
	bool check = true;
	for(int i = 0; i < 8; i++){
		if(pincode[i] != checkcode[i]){
			check = false;
		}
	}
	if(check){
		if(restore){
			save_info();
			menu_state = 21;
		}else{
			menu_state = 9;
		}
	}else{
		menu_state = 15;
	}
}
void draw_pairing_qr(){
	uint8_t qrcodeData[qrcode_getBufferSize(3)];
	QRCode qrcode;
	uint8_t read_len_bytes[2];
	read_from_se(optiga_util_instance,read_len_bytes,sizeof(read_len_bytes),0xF1D3);
	uint16_t read_len = (static_cast<uint16_t>(read_len_bytes[0]) << 8) | read_len_bytes[1];
	uint8_t read_data[read_len];
	read_from_se(optiga_util_instance,read_data,sizeof(read_data),0xF1D2);
	std::string final_string(reinterpret_cast<char*>(read_data), read_len);
	int f_length = final_string.length();
	int mid = f_length / 3;
	int remainder = f_length % 3;
	if (remainder != 0) {
	  f_length = f_length - remainder;
	  mid = (int)(f_length / 3);
	}
	std::string first_part = "1,"+final_string.substr(0, mid);
	std::string second_part = "2,"+final_string.substr(mid,mid);
	std::string third_part = "3,"+final_string.substr(mid*2);
	switch(qr_state){
	case 0:
		qrcode_initText(&qrcode, qrcodeData, 3, 0, (char*)first_part.c_str());
		draw_qr_code(qrcode);
		break;
	case 1:
		qrcode_initText(&qrcode, qrcodeData, 3, 0, (char*)second_part.c_str());
		draw_qr_code(qrcode);
		break;
	case 2:
		qrcode_initText(&qrcode, qrcodeData, 3, 0, (char*)third_part.c_str());
		draw_qr_code(qrcode);
		break;
	}
}
bool pin_hash_check(){
	uint8_t pin_h[32] = {0};
	read_from_se(optiga_util_instance,pin_h,sizeof(pin_h),0xF1D6);
	uint8_t pin_hash[32] = {0};
	SHA256_CTX ctx;
	sha256_Init(&ctx);
	sha256_Update(&ctx, pincode, 8);
	sha256_Final(&ctx, pin_hash);
	for(int i = 0; i < 32; i++){
		if(pin_h[i] != pin_hash[i]){
			return false;
		}
	}
	return true;
}
void update_pin_on_start(){
	uint8_t i2c_read_data[2] ={0};
	read_from_se(optiga_util_instance,i2c_read_data,sizeof(i2c_read_data),0xF1D8);
	if(i2c_read_data[0] < 1){
		i2c_read_data[0]++;
	}else{
		i2c_read_data[0] = 0;
	}
	write_to_se(optiga_util_instance,i2c_read_data,sizeof(i2c_read_data),0xF1D8);
}
void update_lock_time(){
	uint8_t i2c_read_data[2] = {0};
	read_from_se(optiga_util_instance,i2c_read_data,sizeof(i2c_read_data),0xF1D8);
	if(i2c_read_data[1] < 2){
		i2c_read_data[1]++;
	}else{
		i2c_read_data[1] = 0;
	}
	write_to_se(optiga_util_instance,i2c_read_data,sizeof(i2c_read_data),0xF1D8);
	lock_mult = i2c_read_data[1];
}
void erase_memory(){
	uint8_t i2c_write_data[32] ={0};
	write_to_se(optiga_util_instance,i2c_write_data,sizeof(i2c_write_data),0xF1D0);
	write_to_se(optiga_util_instance,i2c_write_data,sizeof(i2c_write_data),0xF1D1);
	write_to_se(optiga_util_instance,i2c_write_data,sizeof(i2c_write_data),0xF1D2);
	write_to_se(optiga_util_instance,i2c_write_data,sizeof(i2c_write_data),0xF1D3);
	write_to_se(optiga_util_instance,i2c_write_data,sizeof(i2c_write_data),0xF1D4);
	write_to_se(optiga_util_instance,i2c_write_data,sizeof(i2c_write_data),0xF1D5);
	write_to_se(optiga_util_instance,i2c_write_data,sizeof(i2c_write_data),0xF1D6);
	write_to_se(optiga_util_instance,i2c_write_data,sizeof(i2c_write_data),0xF1D7);
	write_to_se(optiga_util_instance,i2c_write_data,sizeof(i2c_write_data),0xF1D8);
	write_to_se(optiga_util_instance,i2c_write_data,sizeof(i2c_write_data),0xF1D9);
	menu_state = 6;
}
void init_cardware(){
	unlock = true;
	restore = false;
	uint8_t i2c_read_data[1] = {0};
	read_from_se(optiga_util_instance,i2c_read_data,sizeof(i2c_read_data),0xF1D1);
	uint8_t settings[2] = {0};
	read_from_se(optiga_util_instance,settings,sizeof(settings),0xF1D8);
	if(i2c_read_data[0] != 7){
		menu_state = 6;
	}else{
		if(settings[0] == 1){
			menu_state = 0;
		}else{
			menu_state = 17;
		}
	}
	lock_mult = settings[1];
	scan_for_qr(false,true);
}
std::string satoshis_to_btc_string(uint64_t sats){
    const uint64_t SATS_PER_BTC = 100000000;
    uint64_t integerPart = sats / SATS_PER_BTC;
    uint64_t fractionalPart = sats % SATS_PER_BTC;
    std::string btcStr = std::to_string(integerPart);
    btcStr += ".";
    std::string fracStr = std::to_string(fractionalPart);
    size_t fracLen = fracStr.size();
    if (fracLen < 8){
        btcStr.append(8 - fracLen, '0'); // Append leading zeros
    }
    btcStr += fracStr;
    return btcStr;
}
int display_output(uint8_t n){
	std::string psbt_str = "";
	for (int i = 0; i < total_collected; i++) {
		std::string psbt_str_i(psbt_strings[i]);
		psbt_str = psbt_str + psbt_str_i;
	}
	std::string tx_half = "";
	std::string extrad_half = "";
	const char* pos = strchr(psbt_str.c_str(), ':');
	if (pos != nullptr) {
		size_t posIndex = pos - psbt_str.c_str();
		tx_half = psbt_str.substr(0, posIndex);
		extrad_half = psbt_str.substr(posIndex + 1);
	}
	std::vector<uint8_t> txData = base64_decode(tx_half);
	std::vector<uint8_t> extraData = base64_decode(extrad_half);
	size_t numInputs = extraData.size() / 12;
	uint64_t total_inpt_amt = 0;
	for (size_t i = 0; i < numInputs; ++i) {
		size_t offset = i * 12;
		uint16_t a = read_uint16_le(extraData, offset);
		uint16_t b = read_uint16_le(extraData, offset + 2);
		std::string deri = std::to_string(a) +"/"+std::to_string(b);
		total_inpt_amt += read_uint64_le(extraData, offset + 4);
	}

	Tx tx;
	size_t parsedBytes = tx.parse(txData.data(), txData.size());
	if (parsedBytes == 0) {
		return 0;
	}

	uint64_t total_amount = 0;
	bool draw_fees = false;
	for (uint8_t i = 0; i < tx.outputsNumber; i++){
		if((n == tx.outputsNumber-1 && tx.outputsNumber > 1) || (tx.outputsNumber == 1 && n == 1)){
			const TxOut &out = tx.txOuts[i];
			total_amount += out.amount;
			draw_fees = true;
		}else if(n == i){
			const TxOut &out = tx.txOuts[i];
			char addressBuf[80] = {0};
			size_t result = out.scriptPubkey.address(addressBuf, sizeof(addressBuf), &Mainnet);
			if (result > 0){
				std::string output_amt = satoshis_to_btc_string(out.amount)+" BTC";
				ST7735_FillRectangle(0,23,128,137,CREAM);
				ST7735_WriteString(48,30,"SEND",Font_7x10,BLACK,CREAM);
				ST7735_WriteString(15,50,output_amt.c_str(),Font_7x10,BLACK,CREAM);
				ST7735_WriteString(55,70,"TO",Font_7x10,BLACK,CREAM);
				ST7735_WriteString(0,90,addressBuf,Font_7x10,BLACK,CREAM);
				ST7735_WriteString(95,147,"NEXT",Font_7x10,BLACK,CREAM);
			} else {
				// If address() returned 0, it indicates unrecognized or non-standard script
			}
		}
	}
	if(draw_fees){
		uint64_t fees = total_inpt_amt - total_amount;
		ST7735_FillRectangle(0,23,128,137,CREAM);
		ST7735_WriteString(48,30,"FEES",Font_7x10,BLACK,CREAM);
		std::string output_amt = satoshis_to_btc_string(fees)+" BTC";
		ST7735_WriteString(15,50,output_amt.c_str(),Font_7x10,BLACK,CREAM);
		ST7735_WriteString(74,147,"CONFIRM",Font_7x10,BLACK,CREAM);
	}
	return tx.outputsNumber;
}
std::string remove_last_word(const std::string& input) {
    std::size_t pos = input.find_last_of(' ');
    if (pos == std::string::npos) {
        return input;
    }
    return input.substr(0, pos);
}
void draw_restore_word(){
	ST7735_FillRectangle(0,23,128,137,CREAM);
	std::string select_str = "Enter word #"+std::to_string(restore_word_pos+1);
	ST7735_WriteString(17,27,select_str.c_str(),Font_7x10,BLACK,CREAM);
	ST7735_FillRectangle(0,39,128,1,ORANGE);
	ST7735_FillRectangle(0,140,128*restore_word_pos/23,20,GREEN);
	char letter1[2]= { base64_chars[letter1_pos+26], '\0' };
	char letter2[2]= { base64_chars[letter2_pos+26], '\0' };
	char nextLetter1[2] = {base64_chars[letter1_pos+27],'\0'};
	char prevLetter1[2] = {base64_chars[letter1_pos+25],'\0'};
	switch(restore_word_state){
	case 0:
		if(letter1_pos == 0){
			prevLetter1[0] = base64_chars[letter1_pos+51];
		}
		if(letter1_pos == 25){
			nextLetter1[0] = base64_chars[letter1_pos+1];
		}
		ST7735_WriteString(22,52,prevLetter1,Font_7x10,GREY,CREAM);
		ST7735_WriteString(20,65,letter1,Font_11x18,BLACK,CREAM);
		ST7735_WriteString(22,85,nextLetter1,Font_7x10,GREY,CREAM);
		break;
	case 1:
		if(letter2_pos == 0){
			prevLetter1[0] = base64_chars[letter2_pos+51];
		}else{
			prevLetter1[0] = base64_chars[letter2_pos+25];
		}
		if(letter2_pos == 25){
			nextLetter1[0] = base64_chars[letter2_pos+1];
		}else{
			nextLetter1[0] = base64_chars[letter2_pos+27];
		}
		ST7735_WriteString(20,65,letter1,Font_11x18,BLACK,CREAM);
		ST7735_WriteString(34,52,prevLetter1,Font_7x10,GREY,CREAM);
		ST7735_WriteString(32,65,letter2,Font_11x18,BLACK,CREAM);
		ST7735_WriteString(34,85,nextLetter1,Font_7x10,GREY,CREAM);
		break;
	case 2:
		if(found_max == 0){
			ST7735_WriteString(5,52,"Not in wordlist",Font_7x10,BLACK,CREAM);
		}else{
			if(found_word_pos == 0){
				ST7735_WriteString(22,52,found_matches[found_max-1],Font_7x10,GREY,CREAM);
			}else{
				ST7735_WriteString(22,52,found_matches[found_word_pos-1],Font_7x10,GREY,CREAM);
			}
			ST7735_WriteString(20,65,found_matches[found_word_pos],Font_11x18,BLACK,CREAM);
			if(found_word_pos == found_max-1){
				ST7735_WriteString(22,85,found_matches[0],Font_7x10,GREY,CREAM);
			}else{
				ST7735_WriteString(22,85,found_matches[found_word_pos+1],Font_7x10,GREY,CREAM);
			}
		}
		break;
	}
}
void fetch_matching_words(){
	int matchCount = 0;
	char prefix[3] = { base64_chars[letter1_pos+26], base64_chars[letter2_pos+26], '\0' };
	for (int i = 0; wordlist[i] != NULL; i++) {
		if (strncmp(wordlist[i], prefix, 2) == 0) {
			if (matchCount < (int)(sizeof(found_matches) / sizeof(found_matches[0]) - 1)) {
				found_matches[matchCount++] = wordlist[i];
			}
		}
	}
	found_max = matchCount;
	found_matches[matchCount] = NULL;
}
void save_seed_hash(){
	int result = mnemonic_to_entropy(check_mnem.c_str(),hash_hold);
	if(result != 0){
		menu_state =13;
	}
}
//UI Screens
void onboarding_screen(){
	if(menu_state != old_state){
		fillScreen(CREAM);
		ST7735_WriteString(20,2,"CARDWARE",Font_11x18,ORANGE,CREAM);
		ST7735_FillRectangle(0,22,128,1,ORANGE);
		second_ms = 0;
		old_sMS = 99;
		old_state = menu_state;
		entropy_trk =0 ;
		restore = false;
		HAL_Delay(100);
		ST7735_WriteString(5,32,"Please select",Font_7x10,BLACK,CREAM);
		ST7735_WriteString(5,42,"entropy creation",Font_7x10,BLACK,CREAM);
		ST7735_WriteString(5,52,"method or restore.",Font_7x10,BLACK,CREAM);
		ST7735_FillRectangle(0,71,128,1,ORANGE);
		ST7735_FillRectangle(0,93,128,1,ORANGE);
		ST7735_FillRectangle(0,115,128,1,ORANGE);
		ST7735_FillRectangle(0,137,128,1,ORANGE);
	}
	if(second_ms != old_sMS){
		old_sMS = second_ms;
		ST7735_FillRectangle(0,61,128,99,CREAM);
		ST7735_FillRectangle(0,71,128,1,ORANGE);
		ST7735_FillRectangle(0,93,128,1,ORANGE);
		ST7735_FillRectangle(0,115,128,1,ORANGE);
		ST7735_FillRectangle(0,137,128,1,ORANGE);
		ST7735_WriteString(30,74,"Video",Font_11x18,ORANGE,CREAM);
		ST7735_WriteString(20,96,"Dice D20",Font_11x18,ORANGE,CREAM);
		ST7735_WriteString(25,118,"Dice D6",Font_11x18,ORANGE,CREAM);
		ST7735_WriteString(25,140,"Restore",Font_11x18,ORANGE,CREAM);
		switch(second_ms){
			case 0:
			  ST7735_FillRectangle(0,72,128,22,ORANGE);
			  ST7735_WriteString(30,74,"Video",Font_11x18,CREAM,ORANGE);
			  break;
			case 1:
			  ST7735_FillRectangle(0,94,128,22,ORANGE);
			  ST7735_WriteString(20,96,"Dice D20",Font_11x18,CREAM,ORANGE);
			  break;
			case 2:
			  ST7735_FillRectangle(0,116,128,22,ORANGE);
			  ST7735_WriteString(25,118,"Dice D6",Font_11x18,CREAM,ORANGE);
			  break;
			case 3:
			  ST7735_FillRectangle(0,138,128,22,ORANGE);
			  ST7735_WriteString(25,140,"Restore",Font_11x18,CREAM,ORANGE);
			  break;
		}
	}
	if(HAL_GPIO_ReadPin(BTN_3_GPIO_Port,BTN_3_Pin)){
		global_tick = 0;
		if(second_ms >0){
			second_ms--;
		}else{
			second_ms = 3;
		}
		HAL_Delay(100);
	}
	if(HAL_GPIO_ReadPin(BTN_4_GPIO_Port,BTN_4_Pin)){
		global_tick = 0;
		if(second_ms < 3){
			second_ms++;
		}else{
			second_ms = 0;
		}
		HAL_Delay(100);
	}
	if(HAL_GPIO_ReadPin(BTN_2_GPIO_Port,BTN_2_Pin)){
		global_tick = 0;
		switch(second_ms){
			case 0:
			  menu_state = 16;
			  break;
			case 1:
			  menu_state = 19;
			  max_dice = 64;
			  break;
			case 2:
			  menu_state = 19;
			  max_dice = 99;
			  break;
			case 3:
			  menu_state = 20;
			  break;
		}
	}
}
void main_menu_screen(){
	if(menu_state != old_state){
		unlock = false;
		fillScreen(CREAM);
		total_collected = 0;
		clear_memory();
		reset_psbt_strings();
		ST7735_WriteString(20,2,"CARDWARE",Font_11x18,ORANGE,CREAM);
		ST7735_FillRectangle(0,22,128,1,ORANGE);
		old_state = menu_state;
		second_ms = 0;
		old_sMS = 255;
		HAL_Delay(100);
	}
	if(second_ms != old_sMS){
		ST7735_FillRectangle(0,23,128,137,CREAM);
		switch(second_ms){
		  case 0:
			  ST7735_FillRectangle(0,22,128,22,ORANGE);
			  ST7735_WriteString(40,24,"SCAN",Font_11x18,CREAM,ORANGE);
			  ST7735_WriteString(40,46,"PAIR",Font_11x18,ORANGE,CREAM);
			  ST7735_WriteString(25,69,"ADDRESS",Font_11x18,ORANGE,CREAM);
			  ST7735_WriteString(20,92,"SETTINGS",Font_11x18,ORANGE,CREAM);
			  break;
		  case 1:
			  ST7735_FillRectangle(0,44,128,22,ORANGE);
			  ST7735_WriteString(40,24,"SCAN",Font_11x18,ORANGE,CREAM);
			  ST7735_WriteString(40,46,"PAIR",Font_11x18,CREAM,ORANGE);
			  ST7735_WriteString(25,69,"ADDRESS",Font_11x18,ORANGE,CREAM);
			  ST7735_WriteString(20,92,"SETTINGS",Font_11x18,ORANGE,CREAM);
			  break;
		  case 2:
			  ST7735_FillRectangle(0,66,128,22,ORANGE);
			  ST7735_WriteString(40,24,"SCAN",Font_11x18,ORANGE,CREAM);
			  ST7735_WriteString(40,46,"PAIR",Font_11x18,ORANGE,CREAM);
			  ST7735_WriteString(25,69,"ADDRESS",Font_11x18,CREAM,ORANGE);
			  ST7735_WriteString(20,92,"SETTINGS",Font_11x18,ORANGE,CREAM);
			  break;
		  case 3:
			  ST7735_FillRectangle(0,88,128,22,ORANGE);
			  ST7735_WriteString(40,24,"SCAN",Font_11x18,ORANGE,CREAM);
			  ST7735_WriteString(40,46,"PAIR",Font_11x18,ORANGE,CREAM);
			  ST7735_WriteString(25,69,"ADDRESS",Font_11x18,ORANGE,CREAM);
			  ST7735_WriteString(20,92,"SETTINGS",Font_11x18,CREAM,ORANGE);
			  break;
		}
		ST7735_FillRectangle(0,44,128,1,ORANGE);
		ST7735_FillRectangle(0,66,128,1,ORANGE);
		ST7735_FillRectangle(0,88,128,1,ORANGE);
		ST7735_FillRectangle(0,110,128,1,ORANGE);
		old_sMS = second_ms;
		HAL_Delay(50);
	}
	if(HAL_GPIO_ReadPin(BTN_4_GPIO_Port,BTN_4_Pin)){
		global_tick = 0;
		if(second_ms < 3){
		    second_ms++;
		}else{
		    second_ms = 0;
		}
	}
	if(HAL_GPIO_ReadPin(BTN_3_GPIO_Port,BTN_3_Pin)){
		global_tick = 0;
		if(second_ms > 0){
		   second_ms--;
		}else{
		   second_ms = 3;
		}
	}
	if(HAL_GPIO_ReadPin(BTN_2_GPIO_Port,BTN_2_Pin)){
		global_tick = 0;
		switch(second_ms){
			case 0: menu_state = 3; break;
			case 1: menu_state = 2; break;
			case 2: menu_state = 1; break;
			case 3: menu_state = 8; break;
		}
	}
}
void xPub_display_screen(){
	if(menu_state != old_state){
		fillScreen(CREAM);
		ST7735_WriteString(20,2,"CARDWARE",Font_11x18,ORANGE,CREAM);
		ST7735_FillRectangle(0,22,128,1,ORANGE);
		old_state = menu_state;
		uint8_t read_len_bytes[2];
		read_from_se(optiga_util_instance,read_len_bytes,sizeof(read_len_bytes),0xF1D3);
		uint16_t read_len = (static_cast<uint16_t>(read_len_bytes[0]) << 8) | read_len_bytes[1];
		uint8_t read_data[read_len];
		read_from_se(optiga_util_instance,read_data,sizeof(read_data),0xF1D2);
		std::string xpub(reinterpret_cast<char*>(read_data), read_len);
		uint8_t addr_read_len_bytes[2];
		read_from_se(optiga_util_instance,addr_read_len_bytes,sizeof(addr_read_len_bytes),0xF1D5);
		uint16_t addr_read_len = (static_cast<uint16_t>(addr_read_len_bytes[0]) << 8) | addr_read_len_bytes[1];
		uint8_t addr_read_data[addr_read_len];
		read_from_se(optiga_util_instance,addr_read_data,sizeof(addr_read_data),0xF1D4);
		std::string addr_str(reinterpret_cast<char*>(addr_read_data), addr_read_len);
		xpub = "ZPUB: "+xpub;
		addr_str  = "ADDR: "+addr_str;
		ST7735_WriteString(0,28,xpub.c_str(),Font_7x10,BLACK,CREAM);
		ST7735_WriteString(0,115,addr_str.c_str(),Font_7x10,BLACK,CREAM);
		HAL_Delay(100);
  }
  if(HAL_GPIO_ReadPin(BTN_1_GPIO_Port,BTN_1_Pin)){
		global_tick = 0;
		menu_state = 0;
  }
}
void pairing_screen(){
	if(menu_state != old_state){
		fillScreen(CREAM);
		ST7735_WriteString(25,50,"LOADING",Font_11x18,ORANGE,CREAM);
		fillScreen(CREAM);
		old_state = menu_state;
		ST7735_WriteString(20,135,"SCAN TO PAIR",Font_7x10,BLACK,CREAM);
		HAL_Delay(100);
		tick_count = 0;
		qr_state = 0;
		draw_pairing_qr();
	}
	if(tick_count > 25){
		tick_count = 0;
		qr_state ++;
		if(qr_state > 2){
			qr_state = 0;
		}
		draw_pairing_qr();
	}
	tick_count++;
	if(HAL_GPIO_ReadPin(BTN_1_GPIO_Port,BTN_1_Pin)){
		global_tick = 0;
		menu_state = 0;
		HAL_Delay(100);
		return;
	}
}
void scanning_screen(){
	if(menu_state != old_state){
		old_state = menu_state;
		unlock = false;
		total_collected = 0;
		reset_psbt_strings();
		fillScreen(CREAM);
		ST7735_WriteString(20,2,"SCANNING",Font_11x18,ORANGE,CREAM);
		ST7735_FillRectangle(0,22,128,1,ORANGE);
		HAL_Delay(100);
	}
	scan_for_qr(false, false);
	if(HAL_GPIO_ReadPin(BTN_1_GPIO_Port,BTN_1_Pin)){
		global_tick = 0;
		menu_state = 0;
		HAL_Delay(100);
		return;
	}
}
void confirm_screen(){
	if(menu_state != old_state){
		old_state = menu_state;
		fillScreen(CREAM);
		ST7735_WriteString(20,2,"CONFIRM?",Font_11x18,ORANGE,CREAM);
		ST7735_FillRectangle(0,22,128,1,ORANGE);
		tx_outputs = display_output(0);
		output_pos =0;
		HAL_Delay(100);
	}
	if(HAL_GPIO_ReadPin(BTN_2_GPIO_Port,BTN_2_Pin)){
		global_tick = 0;
		output_pos++;
		if((output_pos == tx_outputs && tx_outputs != 1) ||(output_pos > tx_outputs && tx_outputs == 1)){
			menu_state = 17;
		}else{
			display_output(output_pos);
		}
		HAL_Delay(100);
	}
	if(HAL_GPIO_ReadPin(BTN_1_GPIO_Port,BTN_1_Pin)){
		global_tick = 0;
		menu_state = 0;
	}
}
void signing_screen(){
	if(menu_state != old_state){
		old_state = menu_state;
		fillScreen(CREAM);
		ST7735_WriteString(20,2,"SIGNING",Font_11x18,ORANGE,CREAM);
		tick_count = 0;
		qr_state = 0;
		combine_psbt();
		fillScreen(CREAM);
		ST7735_WriteString(15,135,"BROADCAST",Font_11x18,BLACK,CREAM);
		HAL_Delay(100);
	}
	if(HAL_GPIO_ReadPin(BTN_1_GPIO_Port,BTN_1_Pin)){
		global_tick = 0;
		menu_state = 0;
	}
	if(tick_count > 25){
		tick_count = 0;
		qr_state ++;
		if(qr_state > total_broadcast_qr){
			qr_state =0;
		}
		QRCode qrcode;
		uint8_t qrcodeData[qrcode_getBufferSize(3)];
		qrcode_initText(&qrcode, qrcodeData, 3, 0, broadcast_strings[qr_state]);
		draw_qr_code(qrcode);
	}
	tick_count++;
}
void video_entropy_screen(){
	if(menu_state != old_state){
		fillScreen(CREAM);
		ST7735_WriteString(20,2,"CARDWARE",Font_11x18,ORANGE,CREAM);
		ST7735_FillRectangle(0,22,128,1,ORANGE);
		old_state = menu_state;
		HAL_Delay(100);
	}
	if(entropy_trk != old_et){
		ST7735_FillRectangle(0,143,128,17,CREAM);
		int rectLength = (entropy_trk*128)/64;
		ST7735_FillRectangle(0,143,rectLength,17,GREEN);
		old_et = entropy_trk;
	}
	if(HAL_GPIO_ReadPin(BTN_1_GPIO_Port,BTN_1_Pin)){ //Down
		global_tick = 0;
		menu_state = 6;
		HAL_Delay(100);
	}
	scan_for_qr(true,false);
}
void settings_screen(){
	if(menu_state != old_state){
		fillScreen(CREAM);
		total_collected = 0;
		reset_psbt_strings();
		ST7735_WriteString(20,2,"SETTINGS",Font_11x18,ORANGE,CREAM);
		ST7735_FillRectangle(0,22,128,1,ORANGE);
		old_state = menu_state;
		second_ms = 0;
		old_sMS = 255;
		HAL_Delay(100);
	}
	if(second_ms != old_sMS){
		uint8_t i2c_read_data[2] ={0};
		read_from_se(optiga_util_instance,i2c_read_data,sizeof(i2c_read_data),0xF1D8);
		ST7735_FillRectangle(0,23,128,137,CREAM);
		std::string yesNo = "YES";
		if(i2c_read_data[0]==1){
			yesNo = "NO";
		}
		std::string lockTime = "5 min";
		switch(i2c_read_data[1]){
			case 0: lockTime = "None"; break;
			case 1: lockTime = "5 min"; break;
			case 2: lockTime = "10 min"; break;
		}
		switch(second_ms){
			case 0:
				ST7735_FillRectangle(0,22,128,22,ORANGE);
				ST7735_WriteString(5,29,"PIN ON START:",Font_7x10,CREAM,ORANGE);
				ST7735_WriteString(100,29,yesNo.c_str(),Font_7x10,CREAM,ORANGE);
				ST7735_WriteString(5,51,"LOCK TIME:",Font_7x10,ORANGE,CREAM);
				ST7735_WriteString(80,51,lockTime.c_str(),Font_7x10,ORANGE,CREAM);
				break;
			case 1:
				ST7735_FillRectangle(0,44,128,22,ORANGE);
				ST7735_WriteString(5,29,"PIN ON START:",Font_7x10,ORANGE,CREAM);
				ST7735_WriteString(100,29,yesNo.c_str(),Font_7x10,ORANGE,CREAM);
				ST7735_WriteString(5,51,"LOCK TIME:",Font_7x10,CREAM,ORANGE);
				ST7735_WriteString(80,51,lockTime.c_str(),Font_7x10,CREAM,ORANGE);
				break;
		}
		ST7735_FillRectangle(0,44,128,1,ORANGE);
		ST7735_FillRectangle(0,66,128,1,ORANGE);
		old_sMS = second_ms;
		HAL_Delay(50);
	}
	if(HAL_GPIO_ReadPin(BTN_4_GPIO_Port,BTN_4_Pin)){
		global_tick = 0;
		if(second_ms < 1){
			second_ms++;
		}else{
			second_ms = 0;
		}
	}
	if(HAL_GPIO_ReadPin(BTN_3_GPIO_Port,BTN_3_Pin)){
		global_tick = 0;
		if(second_ms > 0){
			second_ms--;
		}else{
			second_ms = 1;
		}
	}
	if(HAL_GPIO_ReadPin(BTN_2_GPIO_Port,BTN_2_Pin)){
		global_tick = 0;
		switch(second_ms){
			case 0: update_pin_on_start(); old_sMS = 255; break;
			case 1: update_lock_time(); old_sMS = 255; break;
		}
	}
	if(HAL_GPIO_ReadPin(BTN_1_GPIO_Port,BTN_1_Pin)){
		global_tick = 0;
		menu_state = 0;
	}
}
void seed_warning_screen(){
	if(menu_state != old_state){
		fillScreen(CREAM);
		ST7735_WriteString(20,2,"WARNING",Font_11x18,RED,CREAM);
		ST7735_FillRectangle(0,22,128,1,RED);
		ST7735_WriteString(5,25,"The device will",Font_7x10,BLACK,CREAM);
		ST7735_WriteString(5,35,"now display your",Font_7x10,BLACK,CREAM);
		ST7735_WriteString(5,45,"seed phrase.",Font_7x10,BLACK,CREAM);
		ST7735_WriteString(5,60,"Please write this",Font_7x10,BLACK,CREAM);
		ST7735_WriteString(5,70,"down and store",Font_7x10,BLACK,CREAM);
		ST7735_WriteString(5,80,"it safely. Your",Font_7x10,BLACK,CREAM);
		ST7735_WriteString(5,90,"seed phrase gives",Font_7x10,BLACK,CREAM);
		ST7735_WriteString(5,100,"access to all",Font_7x10,BLACK,CREAM);
		ST7735_WriteString(5,110,"your funds.",Font_7x10,BLACK,CREAM);
		old_state = menu_state;
		HAL_Delay(100);
	}
	if(HAL_GPIO_ReadPin(BTN_2_GPIO_Port,BTN_2_Pin)){
		global_tick = 0;
		fillScreen(CREAM);
		ST7735_WriteString(20,2,"CARDWARE",Font_11x18,ORANGE,CREAM);
		ST7735_FillRectangle(0,22,128,1,ORANGE);
		menu_state = 10;
	}
}
void seed_display_screen(){
	if(menu_state != old_state){
		ST7735_FillRectangle(0,23,128,137,CREAM);
		gen_seed_phrase();
		draw_seed_phrase(seed_phase);
		old_state = menu_state;
		if(seed_phase == 0 || seed_phase == 1){
			ST7735_WriteString(90,145,"NEXT",Font_7x10,BLACK,CREAM);
		}
		if(seed_phase == 2){
			ST7735_WriteString(90,145,"DONE",Font_7x10,BLACK,CREAM);
		}
		HAL_Delay(100);
	}
	if(HAL_GPIO_ReadPin(BTN_2_GPIO_Port,BTN_2_Pin)){
		global_tick = 0;
		if(seed_phase < 2){
			seed_phase = seed_phase + 1;
			old_state = 0;
		}else if(seed_phase == 2){
			srand(HAL_GetTick());
			menu_state = 11;
		}
	}
	if(HAL_GPIO_ReadPin(BTN_1_GPIO_Port,BTN_1_Pin)){
		global_tick = 0;
		if(seed_phase > 0){
			seed_phase = seed_phase - 1;
			old_state = 0;
		}
	}
}
void confirm_seed_screen(){
	if(menu_state != old_state){
		fillScreen(CREAM);
		ST7735_WriteString(20,2,"CARDWARE",Font_11x18,ORANGE,CREAM);
		ST7735_FillRectangle(0,22,128,1,ORANGE);
		check_mnem = "";
		option_state = 0;
		seed_pos = 0;
		gen_options();
		draw_option_state();
		old_state = menu_state;
		HAL_Delay(100);
	}
	if(HAL_GPIO_ReadPin(BTN_1_GPIO_Port,BTN_1_Pin)){
		global_tick = 0;
		if(seed_pos == 0){
		  menu_state = 10;
		}else{
		  seed_pos--;
		  gen_options();
		  draw_option_state();
		  check_mnem = remove_last_word(check_mnem);
		}
		HAL_Delay(100);
	}
	if(HAL_GPIO_ReadPin(BTN_2_GPIO_Port,BTN_2_Pin)){
		if(seed_pos == 0){
		  check_mnem =  mnem_options[option_state];
		}else{
		  check_mnem = check_mnem +" "+ mnem_options[option_state];
		}
		if(seed_pos == 23){
		  if(check_mnem == mnem){
			  fillScreen(CREAM);
			  ST7735_WriteString(25,2,"LOADING",Font_11x18,ORANGE,CREAM);
			  ST7735_FillRectangle(0,22,128,1,ORANGE);
			  save_info();
			  menu_state = 21;
		  }else{
			  menu_state = 12;
		  }
		}else{
		  seed_pos++;
		  gen_options();
		  draw_option_state();
		}
		HAL_Delay(100);
	}
	if(HAL_GPIO_ReadPin(BTN_3_GPIO_Port,BTN_3_Pin)){
		global_tick = 0;
		if(option_state >0){
		    option_state--;
		}else{
			option_state = 4;
		}
		draw_option_state();
		HAL_Delay(100);
	}
	if(HAL_GPIO_ReadPin(BTN_4_GPIO_Port,BTN_4_Pin)){
		global_tick = 0;
		if(option_state < 4){
			option_state++;
		}else{
			option_state = 0;
		}
		draw_option_state();
		HAL_Delay(100);
	}
}
void incorrect_seed_screen(){
	if(menu_state != old_state){
		fillScreen(CREAM);
		ST7735_WriteString(20,2,"WARNING",Font_11x18,RED,CREAM);
		ST7735_FillRectangle(0,22,128,1,RED);
		ST7735_WriteString(5,25,"Incorrect seed",Font_7x10,BLACK,CREAM);
		ST7735_WriteString(5,35,"phrase, please",Font_7x10,BLACK,CREAM);
		ST7735_WriteString(5,45,"try again.",Font_7x10,BLACK,CREAM);
		old_state = menu_state;
		HAL_Delay(100);
	}
	if(HAL_GPIO_ReadPin(BTN_2_GPIO_Port,BTN_2_Pin)){
		global_tick = 0;
		menu_state = 11;
	}
}
void choose_pin_screen(){
	if(menu_state != old_state){
		fillScreen(CREAM);
		ST7735_WriteString(20,2,"CARDWARE",Font_11x18,ORANGE,CREAM);
		ST7735_FillRectangle(0,22,128,1,ORANGE);
		ST7735_WriteString(25,25,"Choose pin:",Font_7x10,BLACK,CREAM);
		ST7735_FillRectangle(0,37,128,1,ORANGE);
		reset_pin_arr();
		pinpos = 0;
		old_state = menu_state;
		draw_pinselect();
		HAL_Delay(150);
	}
	if(HAL_GPIO_ReadPin(BTN_1_GPIO_Port,BTN_1_Pin)){
		global_tick = 0;
		if(pinpos > 0){
			pinpos--;
		}
		ST7735_FillRectangle(0,95,128,25,CREAM);
		draw_pinselect();
		HAL_Delay(150);
	}
	if(HAL_GPIO_ReadPin(BTN_2_GPIO_Port,BTN_2_Pin)){
		global_tick = 0;
		ST7735_FillRectangle(0,95,128,25,CREAM);
		if(pinpos < 7){
			pinpos++;
		}else if(pinpos == 7){
			if(pin_length_check()){
				menu_state = 14;
			}else{
				ST7735_WriteString(15,100,"Pin too short.",Font_7x10,BLACK,CREAM);
			}
		}
		draw_pinselect();
		HAL_Delay(150);
	}
	if(HAL_GPIO_ReadPin(BTN_3_GPIO_Port,BTN_3_Pin)){
		global_tick = 0;
		ST7735_FillRectangle(0,95,128,25,CREAM);
		if(pincode[pinpos] < 10){
			pincode[pinpos]++;
		}else{
			pincode[pinpos] = 0;
		}
		draw_pinselect();
		HAL_Delay(150);
	}
	if(HAL_GPIO_ReadPin(BTN_4_GPIO_Port,BTN_4_Pin)){
		global_tick = 0;
		ST7735_FillRectangle(0,95,128,25,CREAM);
		if(pincode[pinpos] > 0){
			pincode[pinpos]--;
		}else{
			pincode[pinpos] = 10;
		}
		draw_pinselect();
		HAL_Delay(150);
	}
}
void confirm_pin_screen(){
	if(menu_state != old_state){
		fillScreen(CREAM);
		ST7735_WriteString(20,2,"CARDWARE",Font_11x18,ORANGE,CREAM);
		ST7735_FillRectangle(0,22,128,1,ORANGE);
		ST7735_WriteString(25,25,"Confirm pin:",Font_7x10,BLACK,CREAM);
		ST7735_FillRectangle(0,37,128,1,ORANGE);
		reset_check_pin();
		old_state = menu_state;
		pinpos = 0;
		draw_checkpin();
		HAL_Delay(150);
	}
	if(HAL_GPIO_ReadPin(BTN_1_GPIO_Port,BTN_1_Pin)){
		global_tick = 0;
		if(pinpos > 0){
		  pinpos--;
		}
		draw_checkpin();
		HAL_Delay(150);
	}
	if(HAL_GPIO_ReadPin(BTN_2_GPIO_Port,BTN_2_Pin)){
		global_tick = 0;
		if(pinpos < 7){
		  pinpos++;
		}else if(pinpos == 7){
		  checkpin();
		}
		draw_checkpin();
		HAL_Delay(150);
	}
	if(HAL_GPIO_ReadPin(BTN_3_GPIO_Port,BTN_3_Pin)){
		global_tick = 0;
		if(checkcode[pinpos] < 10){
		  checkcode[pinpos]++;
		}else{
		  checkcode[pinpos] = 0;
		}
		draw_checkpin();
		HAL_Delay(150);
	}
	if(HAL_GPIO_ReadPin(BTN_4_GPIO_Port,BTN_4_Pin)){
		global_tick = 0;
		if(checkcode[pinpos] > 0){
		  checkcode[pinpos]--;
		}else{
		  checkcode[pinpos] = 10;
		}
		draw_checkpin();
		HAL_Delay(150);
	}
}
void incorrect_pin_screen(){
	if(menu_state != old_state){
		fillScreen(CREAM);
		ST7735_WriteString(20,2,"WARNING",Font_11x18,RED,CREAM);
		ST7735_FillRectangle(0,22,128,1,RED);
		ST7735_WriteString(5,25,"Incorrect pin",Font_7x10,BLACK,CREAM);
		ST7735_WriteString(5,35,"code, please try",Font_7x10,BLACK,CREAM);
		ST7735_WriteString(5,45,"again.",Font_7x10,BLACK,CREAM);
		old_state = menu_state;
		HAL_Delay(100);
	}
	if(HAL_GPIO_ReadPin(BTN_2_GPIO_Port,BTN_2_Pin)){
		global_tick = 0;
		menu_state = 13;
	}
}
void accept_video_screen(){
	if(menu_state != old_state){
		fillScreen(CREAM);
		ST7735_WriteString(20,2,"CARDWARE",Font_11x18,ORANGE,CREAM);
		ST7735_FillRectangle(0,22,128,1,ORANGE);
		ST7735_WriteString(10,55,"Press accept to",Font_7x10,BLACK,CREAM);
		ST7735_WriteString(10,65,"start video ",Font_7x10,BLACK,CREAM);
		ST7735_WriteString(10,75,"stream.",Font_7x10,BLACK,CREAM);
		old_state = menu_state;
		HAL_Delay(100);
	}
	if(HAL_GPIO_ReadPin(BTN_2_GPIO_Port,BTN_2_Pin)){
		global_tick = 0;
		menu_state = 7;
	}
	if(HAL_GPIO_ReadPin(BTN_1_GPIO_Port,BTN_1_Pin)){
		global_tick = 0;
		menu_state = 6;
	}
}
void setup_success_screen(){
	if(menu_state != old_state){
		fillScreen(CREAM);
		ST7735_WriteString(20,2,"CARDWARE",Font_11x18,ORANGE,CREAM);
		ST7735_FillRectangle(0,22,128,1,ORANGE);
		ST7735_WriteString(10,55,"Wallet setup ",Font_7x10,BLACK,CREAM);
		ST7735_WriteString(10,65,"successfully.",Font_7x10,BLACK,CREAM);
		ST7735_WriteString(10,75,"Please restart",Font_7x10,BLACK,CREAM);
		ST7735_WriteString(10,85,"your device by",Font_7x10,BLACK,CREAM);
		ST7735_WriteString(10,95,"unplugging.",Font_7x10,BLACK,CREAM);
		old_state = menu_state;
		HAL_Delay(100);
	}
}
void check_pin_screen(){
	 global_tick = 0;
	if(menu_state != old_state){
		fillScreen(CREAM);
		ST7735_WriteString(20,2,"CARDWARE",Font_11x18,ORANGE,CREAM);
		ST7735_FillRectangle(0,22,128,1,ORANGE);
		ST7735_WriteString(25,25,"Enter pin:",Font_7x10,BLACK,CREAM);
		ST7735_FillRectangle(0,37,128,1,ORANGE);
		reset_pin_arr();
		pinlock = false;
		pinpos = 0;
		old_state = menu_state;
		draw_pinselect();
		HAL_Delay(150);
	}
	if(HAL_GPIO_ReadPin(BTN_1_GPIO_Port,BTN_1_Pin)){
		global_tick = 0;
		if(pinpos > 0){
		  pinpos--;
		}
		draw_pinselect();
		HAL_Delay(150);
	}
	if(HAL_GPIO_ReadPin(BTN_2_GPIO_Port,BTN_2_Pin)){
		global_tick = 0;
		if(pinlock){
		}else{
			if(pinpos < 7){
			  pinpos++;
			}else if(pinpos ==7){
				pinlock = true;
				if(pin_hash_check()){
					uint8_t i2c_write_pincounts[1] ={5};
					write_to_se(optiga_util_instance,i2c_write_pincounts,sizeof(i2c_write_pincounts),0xF1D9);
					HAL_Delay(150);
					if(unlock){
					  menu_state = 0;
					}else{
					  menu_state = 5;
					}
					HAL_Delay(150);
				}else{
					menu_state = 18;
				}
			}
			draw_pinselect();
		}
		HAL_Delay(150);
	}
	if(HAL_GPIO_ReadPin(BTN_3_GPIO_Port,BTN_3_Pin)){
		global_tick = 0;
		if(pincode[pinpos] < 10){
		  pincode[pinpos]++;
		}else{
		  pincode[pinpos]=0;
		}
		draw_pinselect();
		HAL_Delay(150);
	}
	if(HAL_GPIO_ReadPin(BTN_4_GPIO_Port,BTN_4_Pin)){
		global_tick = 0;
		if(pincode[pinpos] > 0){
		  pincode[pinpos]--;
		}else{
		  pincode[pinpos] = 10;
		}
		draw_pinselect();
		HAL_Delay(150);
	}
}
void wrong_pin_screen(){
	if(menu_state != old_state){
		old_state = menu_state;
		uint8_t i2c_write_pincounts[1];
		read_from_se(optiga_util_instance,i2c_write_pincounts,sizeof(i2c_write_pincounts),0xF1D9);
		if(i2c_write_pincounts[0] == 1){
			erase_memory();
		}else{
			i2c_write_pincounts[0]--;
			write_to_se(optiga_util_instance,i2c_write_pincounts,sizeof(i2c_write_pincounts),0xF1D9);
			fillScreen(CREAM);
			ST7735_WriteString(20,2,"WARNING",Font_11x18,RED,CREAM);
			ST7735_FillRectangle(0,22,128,1,RED);
			ST7735_WriteString(5,25,"Incorrect pin.",Font_7x10,BLACK,CREAM);
			ST7735_WriteString(5,35,"The device will",Font_7x10,BLACK,CREAM);
			ST7735_WriteString(5,45,"erase all data",Font_7x10,BLACK,CREAM);
			ST7735_WriteString(5,55,"after 5 incorrect",Font_7x10,BLACK,CREAM);
			ST7735_WriteString(5,65,"attempts.",Font_7x10,BLACK,CREAM);
			ST7735_WriteString(60,90,std::to_string(i2c_write_pincounts[0]).c_str(),Font_11x18,BLACK,CREAM);
			ST7735_WriteString(35,120,"remaining",Font_7x10,BLACK,CREAM);
			HAL_Delay(100);
		}
	}
	if(HAL_GPIO_ReadPin(BTN_2_GPIO_Port,BTN_2_Pin)){
		menu_state = 17;
		unlock = true;
		HAL_Delay(150);
	}
}
void dice_entropy_screen(){
	if(menu_state != old_state){
		fillScreen(CREAM);
		ST7735_WriteString(20,2,"CARDWARE",Font_11x18,ORANGE,CREAM);
		ST7735_FillRectangle(0,22,128,1,ORANGE);
		ST7735_WriteString(10,25,"Enter dice roll:",Font_7x10,BLACK,CREAM);
		ST7735_FillRectangle(0,37,128,1,ORANGE);
		reset_pin_arr();
		dice_pos = 0;
		draw_diceselect();
		old_state = menu_state;
		HAL_Delay(150);
	}
	if(HAL_GPIO_ReadPin(BTN_1_GPIO_Port,BTN_1_Pin)){
		global_tick = 0;
		if(dice_pos > 0){
		  dice_pos--;
		}else{
		  menu_state = 6;
		}
		draw_diceselect();
		HAL_Delay(150);
	}
	if(HAL_GPIO_ReadPin(BTN_2_GPIO_Port,BTN_2_Pin)){
		global_tick = 0;
		if(dice_pos < max_dice){
		  dice_pos++;
		}else if(dice_pos == max_dice){
		  hash_dice_entropy();
		}
		draw_diceselect();
		HAL_Delay(150);
	}
	if(HAL_GPIO_ReadPin(BTN_3_GPIO_Port,BTN_3_Pin)){
		global_tick = 0;
		uint8_t max_pos = 5;
		if(max_dice == 64){ max_pos = 19; }
		if(dice_entropy[dice_pos] < max_pos){
		 dice_entropy[dice_pos]++;
		}else{
		 dice_entropy[dice_pos] = 0;
		}
		draw_diceselect();
		HAL_Delay(150);
	}
	if(HAL_GPIO_ReadPin(BTN_4_GPIO_Port,BTN_4_Pin)){
		global_tick = 0;
		uint8_t max_pos = 5;
		if(max_dice == 64){max_pos = 19;}
		if(dice_entropy[dice_pos] > 0){
		  dice_entropy[dice_pos]--;
		}else{
		  dice_entropy[dice_pos] = max_pos;
		}
		draw_diceselect();
		HAL_Delay(150);
	}
}
void restore_seed_screen(){
	if(menu_state != old_state){
		fillScreen(CREAM);
		ST7735_WriteString(20,2,"CARDWARE",Font_11x18,ORANGE,CREAM);
		ST7735_FillRectangle(0,22,128,1,ORANGE);
		ST7735_FillRectangle(0,23,128,137,CREAM);
		letter1_pos = 0;
		letter2_pos = 0;
		restore_word_pos = 0;
		restore_word_state = 0;
		restore = true;
		check_mnem = "";
		old_state = menu_state;
		draw_restore_word();
		HAL_Delay(100);
	}
	if(HAL_GPIO_ReadPin(BTN_1_GPIO_Port,BTN_1_Pin)){
		global_tick = 0;
		if(restore_word_state > 0){
		  restore_word_state--;
		}else{
		  if(restore_word_pos > 0){
			  restore_word_pos--;
			  check_mnem = remove_last_word(check_mnem);
		  }else{
			  menu_state = 6;
		  }
		}
		draw_restore_word();
		HAL_Delay(150);
	}
	if(HAL_GPIO_ReadPin(BTN_2_GPIO_Port,BTN_2_Pin)){
		global_tick = 0;
		if(restore_word_state < 2){
		  restore_word_state++;
		  if(restore_word_state == 2){
			  fetch_matching_words();
		  }
		  draw_restore_word();
		}else{
		  if(restore_word_pos == 0){
			  check_mnem = found_matches[found_word_pos];
		  }else{
			  check_mnem = check_mnem + " " +found_matches[found_word_pos];
		  }
		  my_printf("check mnem \n");
		  my_printf(check_mnem.c_str());
		  found_word_pos =0;
		  restore_word_state = 0;
		  if(restore_word_pos < 23){
			  restore_word_pos++;
			  draw_restore_word();
		  }else{
			  save_seed_hash();
		  }
		}

		HAL_Delay(150);
	}
	if(HAL_GPIO_ReadPin(BTN_3_GPIO_Port,BTN_3_Pin)){
		global_tick = 0;
		if(restore_word_state == 0){
		  if(letter1_pos > 0){
			  letter1_pos--;
		  }else{
			  letter1_pos = 25;
		  }
		}else if(restore_word_state == 1){
		  if(letter2_pos > 0){
			  letter2_pos--;
		  }else{
			  letter2_pos = 25;
		  }
		}else if(restore_word_state == 2){
		  if(found_word_pos > 0){
			  found_word_pos--;
		  }else{
			  found_word_pos = found_max-1;
		  }
		}
		draw_restore_word();
		HAL_Delay(150);
	}
	if(HAL_GPIO_ReadPin(BTN_4_GPIO_Port,BTN_4_Pin)){
		global_tick = 0;
		if(restore_word_state == 0){
		  if(letter1_pos < 25){
			  letter1_pos++;
		  }else{
			  letter1_pos = 0;
		  }
		}else if(restore_word_state == 1){
		  if(letter2_pos < 25){
			  letter2_pos++;
		  }else{
			  letter2_pos = 0;
		  }
		}else if(restore_word_state == 2){
		  if(found_word_pos < found_max-1){
			  found_word_pos++;
		  }else{
			  found_word_pos = 0;
		  }
		}
		draw_restore_word();
		HAL_Delay(150);
	}
}
void handle_global_tick(){
	global_tick++;
	if(global_tick > 30000*lock_mult && lock_mult != 0){
		menu_state = 17;
		unlock = true;
	}
}
void init_cardware_hardware(){
	ST7735_Init(0);
	fillScreen(CREAM);
	draw_logo();
	ST7735_WriteString(20,105,"CARDWARE",Font_11x18,ORANGE,CREAM);
	HAL_Delay(100);
	OV2640_Init(&hi2c1, &hdcmi);
	HAL_Delay(100);
	OV2640_ResolutionOptions(img_res);
	HAL_Delay(100);
	reset_psbt_strings();
	init_optiga();
	init_cardware();
}
void transaction_error_screen(){
	if(menu_state != old_state){
		old_state = menu_state;
		fillScreen(CREAM);
		ST7735_WriteString(20,2,"WARNING",Font_11x18,RED,CREAM);
		ST7735_FillRectangle(0,22,128,1,RED);
		ST7735_WriteString(5,25,"Incorrect tx",Font_7x10,BLACK,CREAM);
		ST7735_WriteString(5,35,"data. Please",Font_7x10,BLACK,CREAM);
		ST7735_WriteString(5,45,"try again.",Font_7x10,BLACK,CREAM);
		HAL_Delay(100);
	}
	if(HAL_GPIO_ReadPin(BTN_2_GPIO_Port,BTN_2_Pin)){
		menu_state = 0;
		HAL_Delay(150);
	}
}
void signing_error_screen(){
	if(menu_state != old_state){
		old_state = menu_state;
		fillScreen(CREAM);
		ST7735_WriteString(20,2,"WARNING",Font_11x18,RED,CREAM);
		ST7735_FillRectangle(0,22,128,1,RED);
		ST7735_WriteString(5,25,"Error signing",Font_7x10,BLACK,CREAM);
		ST7735_WriteString(5,35,"transaction.",Font_7x10,BLACK,CREAM);
		ST7735_WriteString(5,45,"Please try",Font_7x10,BLACK,CREAM);
		ST7735_WriteString(5,55,"again.",Font_7x10,BLACK,CREAM);
		HAL_Delay(100);
	}
	if(HAL_GPIO_ReadPin(BTN_2_GPIO_Port,BTN_2_Pin)){
		menu_state = 0;
		HAL_Delay(150);
	}
}
void manage_menu(){
	switch(menu_state){
		case 0: main_menu_screen(); break;
		case 1: xPub_display_screen(); break;
		case 2: pairing_screen(); break;
		case 3: scanning_screen();break;
		case 4: confirm_screen();break;
		case 5: signing_screen(); break;
		case 6: onboarding_screen(); break;
		case 7: video_entropy_screen(); break;
		case 8: settings_screen(); break;
		case 9: seed_warning_screen(); break;
		case 10: seed_display_screen(); break;
		case 11: confirm_seed_screen(); break;
		case 12: incorrect_seed_screen(); break;
		case 13: choose_pin_screen(); break;
		case 14: confirm_pin_screen();break;
		case 15: incorrect_pin_screen(); break;
		case 16: accept_video_screen(); break;
		case 17: check_pin_screen(); break;
		case 18: wrong_pin_screen(); break;
		case 19: dice_entropy_screen(); break;
		case 20: restore_seed_screen(); break;
		case 21: setup_success_screen();break;
		case 22: transaction_error_screen(); break;
		case 23: signing_error_screen(); break;
	}
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_MDMA_Init();
  MX_DCMI_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_TIM1_Init();
  MX_JPEG_Init();
  MX_DMA2D_Init();
  /* USER CODE BEGIN 2 */
  init_cardware_hardware();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	HAL_Delay(10);
	manage_menu();
	handle_global_tick();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 60;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 60;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV8;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSI, RCC_MCODIV_2);
}

/**
  * @brief DCMI Initialization Function
  * @param None
  * @retval None
  */
static void MX_DCMI_Init(void)
{

  /* USER CODE BEGIN DCMI_Init 0 */

  /* USER CODE END DCMI_Init 0 */

  /* USER CODE BEGIN DCMI_Init 1 */

  /* USER CODE END DCMI_Init 1 */
  hdcmi.Instance = DCMI;
  hdcmi.Init.SynchroMode = DCMI_SYNCHRO_HARDWARE;
  hdcmi.Init.PCKPolarity = DCMI_PCKPOLARITY_RISING;
  hdcmi.Init.VSPolarity = DCMI_VSPOLARITY_LOW;
  hdcmi.Init.HSPolarity = DCMI_HSPOLARITY_LOW;
  hdcmi.Init.CaptureRate = DCMI_CR_ALL_FRAME;
  hdcmi.Init.ExtendedDataMode = DCMI_EXTEND_DATA_8B;
  hdcmi.Init.JPEGMode = DCMI_JPEG_ENABLE;
  hdcmi.Init.ByteSelectMode = DCMI_BSM_ALL;
  hdcmi.Init.ByteSelectStart = DCMI_OEBS_ODD;
  hdcmi.Init.LineSelectMode = DCMI_LSM_ALL;
  hdcmi.Init.LineSelectStart = DCMI_OELS_ODD;
  if (HAL_DCMI_Init(&hdcmi) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DCMI_Init 2 */

  /* USER CODE END DCMI_Init 2 */

}

/**
  * @brief DMA2D Initialization Function
  * @param None
  * @retval None
  */
static void MX_DMA2D_Init(void)
{

  /* USER CODE BEGIN DMA2D_Init 0 */

  /* USER CODE END DMA2D_Init 0 */

  /* USER CODE BEGIN DMA2D_Init 1 */

  /* USER CODE END DMA2D_Init 1 */
  hdma2d.Instance = DMA2D;
  hdma2d.Init.Mode = DMA2D_M2M;
  hdma2d.Init.ColorMode = DMA2D_OUTPUT_RGB565;
  hdma2d.Init.OutputOffset = 0;
  hdma2d.LayerCfg[1].InputOffset = 0;
  hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_RGB565;
  hdma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
  hdma2d.LayerCfg[1].InputAlpha = 0;
  hdma2d.LayerCfg[1].AlphaInverted = DMA2D_REGULAR_ALPHA;
  hdma2d.LayerCfg[1].RedBlueSwap = DMA2D_RB_REGULAR;
  hdma2d.LayerCfg[1].ChromaSubSampling = DMA2D_NO_CSS;
  if (HAL_DMA2D_Init(&hdma2d) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DMA2D_ConfigLayer(&hdma2d, 1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DMA2D_Init 2 */

  /* USER CODE END DMA2D_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x0050174F;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }

  /** I2C Enable Fast Mode Plus
  */
  HAL_I2CEx_EnableFastModePlus(I2C_FASTMODEPLUS_I2C1);
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x307075B1;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief JPEG Initialization Function
  * @param None
  * @retval None
  */
static void MX_JPEG_Init(void)
{

  /* USER CODE BEGIN JPEG_Init 0 */

  /* USER CODE END JPEG_Init 0 */

  /* USER CODE BEGIN JPEG_Init 1 */

  /* USER CODE END JPEG_Init 1 */
  hjpeg.Instance = JPEG;
  if (HAL_JPEG_Init(&hjpeg) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN JPEG_Init 2 */

  /* USER CODE END JPEG_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_1LINE;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 0x0;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  hspi1.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi1.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi1.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi1.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi1.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi1.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi1.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi1.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 1000;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  * Configure DMA for memory to memory transfers
  *   hdma_memtomem_dma1_stream0
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* Configure DMA request hdma_memtomem_dma1_stream0 on DMA1_Stream0 */
  hdma_memtomem_dma1_stream0.Instance = DMA1_Stream0;
  hdma_memtomem_dma1_stream0.Init.Request = DMA_REQUEST_MEM2MEM;
  hdma_memtomem_dma1_stream0.Init.Direction = DMA_MEMORY_TO_MEMORY;
  hdma_memtomem_dma1_stream0.Init.PeriphInc = DMA_PINC_ENABLE;
  hdma_memtomem_dma1_stream0.Init.MemInc = DMA_MINC_ENABLE;
  hdma_memtomem_dma1_stream0.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_memtomem_dma1_stream0.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_memtomem_dma1_stream0.Init.Mode = DMA_NORMAL;
  hdma_memtomem_dma1_stream0.Init.Priority = DMA_PRIORITY_LOW;
  hdma_memtomem_dma1_stream0.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
  hdma_memtomem_dma1_stream0.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
  hdma_memtomem_dma1_stream0.Init.MemBurst = DMA_MBURST_SINGLE;
  hdma_memtomem_dma1_stream0.Init.PeriphBurst = DMA_PBURST_SINGLE;
  if (HAL_DMA_Init(&hdma_memtomem_dma1_stream0) != HAL_OK)
  {
    Error_Handler( );
  }

  /* DMA interrupt init */
  /* DMA2_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
  /* DMA2_Stream7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);

}

/**
  * Enable MDMA controller clock
  */
static void MX_MDMA_Init(void)
{

  /* MDMA controller clock enable */
  __HAL_RCC_MDMA_CLK_ENABLE();
  /* Local variables */

  /* MDMA interrupt initialization */
  /* MDMA_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(MDMA_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(MDMA_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, SE_RST_Pin|LCD_CS_Pin|LCD_RESET_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, BTN_1_Pin|BTN_2_Pin|BTN_3_Pin|BTN_4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, USER_BTN_Pin|CAMERA_PWDN_Pin|CAMERA_RESET_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : SE_RST_Pin LCD_CS_Pin LCD_RESET_Pin */
  GPIO_InitStruct.Pin = SE_RST_Pin|LCD_CS_Pin|LCD_RESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : BTN_1_Pin BTN_2_Pin BTN_3_Pin BTN_4_Pin */
  GPIO_InitStruct.Pin = BTN_1_Pin|BTN_2_Pin|BTN_3_Pin|BTN_4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_DC_Pin */
  GPIO_InitStruct.Pin = LCD_DC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LCD_DC_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF0_MCO;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : USER_BTN_Pin CAMERA_PWDN_Pin CAMERA_RESET_Pin */
  GPIO_InitStruct.Pin = USER_BTN_Pin|CAMERA_PWDN_Pin|CAMERA_RESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
