/**
 * @file hal_http.hpp
 * @brief 簡易HTTP GET。URLの内容をPSRAMバッファへ取得する（アルバムアート等）。
 */
#pragma once
#include <cstdint>
#include <cstddef>

namespace HAL
{
    /**
     * url を GET し、確保したバッファ(*out_buf)へ格納する。
     * 成功時 true。呼び出し側は使用後 free(*out_buf) すること。
     * @param max_len 受信上限（既定 200KB）
     */
    bool http_get(const char* url, uint8_t** out_buf, size_t* out_len, size_t max_len = 200 * 1024);
}
