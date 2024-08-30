#pragma once

#include "dl_base_pool2d.hpp"

namespace dl
{
    namespace base
    {
        /**
         * @brief 
         * 
         * @tparam feature_t 
         * @tparam filter_t 
         * @tparam buffer_t 
         * @param args_ptr 
         */
        template <typename feature_t, typename filter_t, typename buffer_t>
        void max_pool2d(void *args_ptr);
    } // namespace c
} // namespace dl