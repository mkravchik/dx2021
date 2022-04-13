#include "StdAfx.h"
#include "../../../C/Bra.h"
#include "../Common/RegisterCodec.h"
#include "BranchMisc.h"

namespace NCompress {
namespace NBranch {




REGISTER_FILTER_CREATE(CreateBra_Decoder_IA64, CCoder(IA64_Convert, false)) REGISTER_FILTER_CREATE(CreateBra_Encoder_IA64, CCoder(IA64_Convert, true)) CREATE_BRA(PPC)
REGISTER_FILTER_CREATE(CreateBra_Decoder_ARM, CCoder(ARM_Convert, false)) REGISTER_FILTER_CREATE(CreateBra_Encoder_ARM, CCoder(ARM_Convert, true)) CREATE_BRA(PPC)
REGISTER_FILTER_CREATE(CreateBra_Decoder_ARMT, CCoder(ARMT_Convert, false)) REGISTER_FILTER_CREATE(CreateBra_Encoder_ARMT, CCoder(ARMT_Convert, true)) CREATE_BRA(PPC)
REGISTER_FILTER_CREATE(CreateBra_Decoder_SPARC, CCoder(SPARC_Convert, false)) REGISTER_FILTER_CREATE(CreateBra_Encoder_SPARC, CCoder(SPARC_Convert, true)) CREATE_BRA(PPC)





REGISTER_CODECS_VAR {
  REGISTER_FILTER_ITEM( CreateBra_Decoder_PPC, CreateBra_Encoder_PPC, 0x3030000 + 0x205, "PPC"),
  REGISTER_FILTER_ITEM( CreateBra_Decoder_IA64, CreateBra_Encoder_IA64, 0x3030000 + 0x401, "IA64"),
  REGISTER_FILTER_ITEM( CreateBra_Decoder_ARM, CreateBra_Encoder_ARM, 0x3030000 + 0x501, "ARM"),
  REGISTER_FILTER_ITEM( CreateBra_Decoder_ARMT, CreateBra_Encoder_ARMT, 0x3030000 + 0x701, "ARMT"),
  REGISTER_FILTER_ITEM( CreateBra_Decoder_SPARC, CreateBra_Encoder_SPARC, 0x3030000 + 0x805, "SPARC")
};
REGISTER_CODECS(Branch)
}}
