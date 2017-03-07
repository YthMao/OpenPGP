#include "PGPCleartextSignature.h"

PGPCleartextSignature::PGPCleartextSignature()
    : hash_armor_header(),
      message(),
      sig()
{}

PGPCleartextSignature::PGPCleartextSignature(const PGPCleartextSignature & copy)
    : hash_armor_header(copy.hash_armor_header),
      message(copy.message),
      sig(copy.sig)
{
    sig.set_armored(true);
}

PGPCleartextSignature::PGPCleartextSignature(const std::string & data)
    : PGPCleartextSignature()
{
    read(data);
}

PGPCleartextSignature::PGPCleartextSignature(std::istream & stream)
    : PGPCleartextSignature()
{
    read(stream);
}

void PGPCleartextSignature::read(const std::string & data){
    std::stringstream s(data);
    read(s);
}

void PGPCleartextSignature::read(std::istream & stream){
    // find cleartext header
    //     - The cleartext header ’-----BEGIN PGP SIGNED MESSAGE-----’ on a
    //       single line,
    std::string line;
    while (std::getline(stream, line) && (line != "-----BEGIN PGP SIGNED MESSAGE-----"));

    if (!stream){
        throw std::runtime_error("Error: Data does not contain message section. Use PGP to parse this data.");
    }

    // read hash armor header(s)
    //     - One or more "Hash" Armor Headers,
    //     - Exactly one empty line not included into the message digest,
    while (std::getline(stream, line) && line.size()){
        std::stringstream s(line);
        std::string key, value;

        if (!(std::getline(s, key, ':') && std::getline(s, value))){
            std::cerr << "Warning: Discarding bad Armor Header: " << line << std::endl;
            continue;
        }

        if (key != "Hash"){
            std::cerr << "Warning: Hash Armor Header Key is not \"HASH\": \"" << key << "\"." << std::endl;
        }

        hash_armor_header.push_back(PGP::Armor_Key(key, value));
    }

    // read message
    //     - The dash-escaped cleartext that is included into the message
    //       digest,
    //
    message = "";
    while (std::getline(stream, line) && (line.substr(0, 29) != "-----BEGIN PGP SIGNATURE-----")){
        message += line + "\n";
    }
    message = reverse_dash_escape(message.substr(0, message.size() - 1));

    // read signature into string
    //     - The ASCII armored signature(s) including the ’-----BEGIN PGP
    //       SIGNATURE-----’ Armor Header and Armor Tail Lines.
    std::string ASCII_signature = line + "\n";
    while (std::getline(stream, line) && (line.substr(0, 27) != "-----END PGP SIGNATURE-----")){
        ASCII_signature += line + "\n";
    }
    ASCII_signature += line;

    // parse signature
    sig.read(ASCII_signature);
}

std::string PGPCleartextSignature::show(const uint8_t indents, const uint8_t indent_size) const{
    return std::string(indents * indent_size, ' ') +
           "Message:\n"                            +
           dash_escape(message)                    +
           "Signature:\n"                          +
           sig.show(indents + 1, indent_size);
}

std::string PGPCleartextSignature::write(uint8_t header) const{
    std::string out = "-----BEGIN PGP SIGNED MESSAGE-----\n";

    // write Armor Header
    for(std::pair <std::string, std::string> const & k : hash_armor_header){
        out += k.first + ": " + k.second + "\n";
    }

    // one empty line
    out += "\n";

    // only add "- " to front of message
    out += dash_escape(message);

    return out + "\n" + sig.write(header);
}

PGP::Armor_Keys PGPCleartextSignature::get_hash_armor_header() const{
    return hash_armor_header;
}

std::string PGPCleartextSignature::get_message() const{
    return message;
}

PGPDetachedSignature PGPCleartextSignature::get_sig() const{
    return sig;
}

void PGPCleartextSignature::set_hash_armor_header(const PGP::Armor_Keys & keys){
    hash_armor_header = keys;
}

void PGPCleartextSignature::set_message(const std::string & data){
    message = data;
}

void PGPCleartextSignature::set_sig(const PGPDetachedSignature & s){
    sig = s;
    sig.set_armored(true);
}

// 7.1. Dash-Escaped Text
//     The cleartext content of the message must also be dash-escaped.
//
//     Dash-escaped cleartext is the ordinary cleartext where every line
//     starting with a dash ’-’ (0x2D) is prefixed by the sequence dash ’-’
//     (0x2D) and space ’ ’ (0x20). This prevents the parser from
//     recognizing armor headers of the cleartext itself. An implementation
//     MAY dash-escape any line, SHOULD dash-escape lines commencing "From"
//     followed by a space, and MUST dash-escape any line commencing in a
//     dash. The message digest is computed using the cleartext itself, not
//     the dash-escaped form.
//
//     As with binary signatures on text documents, a cleartext signature is
//     calculated on the text using canonical <CR><LF> line endings. The
//     line ending (i.e., the <CR><LF>) before the ’-----BEGIN PGP
//     SIGNATURE-----’ line that terminates the signed text is not
//     considered part of the signed text.
//
//     When reversing dash-escaping, an implementation MUST strip the string
//     "- " if it occurs at the beginning of a line, and SHOULD warn on "-"
//     and any character other than a space at the beginning of a line.
//     Also, any trailing whitespace -- spaces (0x20) and tabs (0x09) -- at
//     the end of any line is removed when the cleartext signature is
//     generated.
std::string PGPCleartextSignature::dash_escape(const std::string & text){
    std::string out = "";

    std::stringstream s(text);
    std::string line;
    while (std::getline(s, line)){
        if (line.size() && line[0] == '-'){
            out += "- ";
        }
        out += line + "\n";
    }

    return out.substr(0, out.size() - 1);
}

std::string PGPCleartextSignature::reverse_dash_escape(const std::string & text){
    std::string out = "";

    std::stringstream s(text);
    std::string line;
    while (std::getline(s, line)){
        if (line.substr(0, 2) == "- "){
            out += line.substr(2, line.size() - 2);
        }
        else{
            out += line;
        }
        out += "\n";
    }

    return out.substr(0, out.size() - 1);
}

std::string PGPCleartextSignature::data_to_text() const{
    return data_to_text(message);
}

std::string PGPCleartextSignature::data_to_text(const std::string & text){
    std::string out = "";

    std::stringstream s(text);
    std::string line;
    while (std::getline(s, line)){
        // remove trailing whitespace
        std::string::size_type i = line.size();
        while (i && ((line[i - 1] == ' ') || (line[i - 1] == '\t'))){
            i--;
        }
        out += line.substr(0, i) + "\n";
    }

    return out.substr(0, out.size() - 1);
}

bool PGPCleartextSignature::meaningful(const std::string & error) const{
    return sig.meaningful(error);
}

bool PGPCleartextSignature::meaningful() const{
    return sig.meaningful();
}

PGPCleartextSignature & PGPCleartextSignature::operator=(const PGPCleartextSignature & copy){
    hash_armor_header = copy.hash_armor_header;
    message = copy.message;
    sig = copy.sig;
    return *this;
}

PGPCleartextSignature::Ptr PGPCleartextSignature::clone() const{
    PGPCleartextSignature::Ptr out = std::make_shared <PGPCleartextSignature> ();
    out -> hash_armor_header = hash_armor_header;
    out -> message = message;
    out -> sig = sig;
    return out;
}
