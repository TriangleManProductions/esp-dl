#pragma once
#include "dl_detect_postprocessor.hpp"

namespace dl
{
    namespace detect
    {
        template <typename feature_t>
        class MSR01Postprocessor : public AnchorBoxDetectPostprocessor
        {
        private:
            void parse_stage(TensorBase *score, TensorBase *box, const int stage_index);
        public:
            void postprocess(std::map<std::string, TensorBase *>& model_outputs_map);
            using AnchorBoxDetectPostprocessor::AnchorBoxDetectPostprocessor;
        };
    }
}