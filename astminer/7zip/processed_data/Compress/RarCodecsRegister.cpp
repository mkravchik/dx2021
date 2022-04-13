#include "StdAfx.h"
#include "../Common/RegisterCodec.h"
#include "Rar1Decoder.h"
#include "Rar2Decoder.h"
#include "Rar3Decoder.h"
#include "Rar5Decoder.h"

namespace NCompress {

REGISTER_CODEC_CREATE(CreateCodec1, NRar1::CDecoder())
REGISTER_CODEC_CREATE(CreateCodec2, NRar2::CDecoder())
REGISTER_CODEC_CREATE(CreateCodec3, NRar3::CDecoder())
REGISTER_CODEC_CREATE(CreateCodec5, NRar5::CDecoder())

REGISTER_CODECS_VAR {
  { CreateCodec1, NULL, 0x40300 + 1, "Rar" "1", 1, false },
  { CreateCodec2, NULL, 0x40300 + 2, "Rar" "2", 1, false },
  { CreateCodec3, NULL, 0x40300 + 3, "Rar" "3", 1, false },
  { CreateCodec5, NULL, 0x40300 + 5, "Rar" "5", 1, false },
};
REGISTER_CODECS(Rar) }
