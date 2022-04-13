#include "StdAfx.h"
#include "../Common/RegisterCodec.h"
#include "Lzma2Decoder.h"
#include "Lzma2Encoder.h"



namespace NCompress {
namespace NLzma2 {
REGISTER_CODEC_E(LZMA2,
    CDecoder(),
    CEncoder(),
    0x21,
    "LZMA2")
}}
