#include "Encryptions.h"

SymAlg::Ptr setup_Sym(const uint8_t sym_alg, const std::string & key){
    SymAlg::Ptr alg;
    switch(sym_alg){
        case Sym::IDEA:
            alg = std::make_shared <IDEA> (key);
            break;
        case Sym::TRIPLEDES:
            alg = std::make_shared <TDES> (key.substr(0, 8), TDES_mode1, key.substr(8, 8), TDES_mode2, key.substr(16, 8), TDES_mode3);
            break;
        case Sym::CAST5:
            alg = std::make_shared <CAST128> (key);
            break;
        case Sym::BLOWFISH:
            alg = std::make_shared <Blowfish> (key);
            break;
        case Sym::AES128:
        case Sym::AES192:
        case Sym::AES256:
            alg = std::make_shared <AES> (key);
            break;
        case Sym::TWOFISH256:
            alg = std::make_shared <Twofish> (key);
            break;
        case Sym::CAMELLIA128:
        case Sym::CAMELLIA192:
        case Sym::CAMELLIA256:
            alg = std::make_shared <Camellia> (key);
            break;
        default:
            throw std::runtime_error("Error: Unknown Symmetric Key Algorithm value.");
            break;
    }
    return alg;
}