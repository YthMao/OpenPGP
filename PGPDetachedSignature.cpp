#include "PGPDetachedSignature.h"

PGPDetachedSignature::PGPDetachedSignature()
    : PGP()
{}

PGPDetachedSignature::PGPDetachedSignature(const PGP & copy)
    : PGP(copy)
{}

PGPDetachedSignature::PGPDetachedSignature(const PGPDetachedSignature & copy)
    : PGP(copy)
{}

PGPDetachedSignature::PGPDetachedSignature(const std::string & data)
    : PGP(data)
{
    // warn if packet sequence is not meaningful
    std::string error;
    if (!meaningful(error)){
        std::cerr << error << std::endl;
    }
}

PGPDetachedSignature::PGPDetachedSignature(std::istream & stream)
    : PGP(stream)
{
    // warn if packet sequence is not meaningful
    std::string error;
    if (!meaningful(error)){
        std::cerr << error << std::endl;
    }
}

PGPDetachedSignature::~PGPDetachedSignature(){}

bool PGPDetachedSignature::meaningful(const PGP & pgp, std::string & error){
    if (pgp.get_type() != SIGNATURE){
        error = "Error: ASCII Armor type is not Signature.";
        return false;
    }

    if (pgp.get_packets().size() != 1){
        error = "Error: Wrong number of packets packets.";
        return false;
    }

    if (pgp.get_packets()[0] -> get_tag() != Packet::SIGNATURE){
        error = "Error: Packet is not a signature packet.";
        return false;
    }

    if (std::static_pointer_cast <Tag2> (pgp.get_packets()[0]) -> get_type() != Signature_Type::SIGNATURE_OF_A_BINARY_DOCUMENT){
        error = "Error: Signature type is not " + Signature_Type::NAME.at(Signature_Type::SIGNATURE_OF_A_BINARY_DOCUMENT);
        return false;
    }

    return true;
}

bool PGPDetachedSignature::meaningful(const PGP & pgp){
    std::string error;
    return meaningful(pgp, error);
}

bool PGPDetachedSignature::meaningful(std::string & error) const{
    return meaningful(*this, error);
}

bool PGPDetachedSignature::meaningful() const{
    std::string error;
    return meaningful(error);
}

PGP::Ptr PGPDetachedSignature::clone() const{
    return std::make_shared <PGPDetachedSignature> (*this);
}
