#include "generatekey.h"

void generate_keys(PGP & public_key, PGP & private_key, const std::string & passphrase, const std::string & user, const std::string & comment, const std::string & email, const unsigned int DSA_bits, const unsigned int ElGamal_bits){
    BBS((mpz_class) (uint32_t) now()); // seed just in case not seeded

    std::vector <mpz_class> mpi;
    std::vector <Subpacket *> subpackets;

    // generate some pka values here
    if ((DSA_bits < 512) || (DSA_bits & 511)){
        std::cerr << "Error: DSA keysize should be at least 512 bits, and a multiple of 512, preferrably 1024, 2048, or 3072." << std::endl;
        throw(1);
    }
    if (ElGamal_bits < 512){
        std::cerr << "Error: ElGamal keysize is too small." << std::endl;
        throw(1);
    }

    // generate key values
    std::vector <mpz_class> dsa_pub = new_DSA_public(DSA_bits, (DSA_bits == 1024)?160:256);
    std::vector <mpz_class> dsa_pri = DSA_keygen(dsa_pub);
//    std::cout << "DSA x: " << dsa_pri[0].get_str(16) << std::endl;

    std::vector <mpz_class> elgamal_pub = ElGamal_keygen(ElGamal_bits);
    mpz_class elgamal_pri = elgamal_pub[3];
    elgamal_pub.pop_back();
//    std::cout << "ElGamal x: " << elgamal_pri.get_str(16) << std::endl;

    time_t time = now();

    // Secret Key Packet
    Tag5 sec;
    sec.set_version(4);
    sec.set_time(time);
    sec.set_pka(17);// DSA
    sec.set_mpi(dsa_pub);
    sec.set_s2k_con(254);
    sec.set_sym(9);// AES

    // Secret Key Packet S2K
    S2K3 sec_s2k3;
    sec_s2k3.set_hash(2);
    sec_s2k3.set_salt(unhexlify(bintohex(BBS().rand(64))));
    sec_s2k3.set_count(96);

    unsigned int sym_block_len = Symmetric_Algorithm_Block_Length.at(Symmetric_Algorithms.at(sec.get_sym()));

    // calculate the key from the passphrase
    std::string key = sec_s2k3.run(passphrase, sym_block_len >> 3);

    // encrypt private key value
    sec.set_s2k(&sec_s2k3);
    sec.set_IV(unhexlify(bintohex(BBS().rand(sym_block_len))));
    std::string secret = write_MPI(dsa_pri[0]);
    sec.set_secret(use_normal_CFB_encrypt(9, secret + use_hash(2, secret), key, sec.get_IV()));

    std::cout << "IV: " << hexlify(sec.get_IV()) << std::endl;

    std::string keyid = sec.get_keyid();

    // User ID packet
    Tag13 uid;
    uid.set_name(user);
    uid.set_comment(comment);
    uid.set_email(email);

    // Signature packet
//    Tag2 sig = *sign_primary_key(0x13, &sec, &uid, passphrase);
    // This is slightly faster, but I wanted to use the sign_ functions
    Tag2 sig;
    sig.set_version(4);
    sig.set_type(0x13);
    sig.set_pka(17);
    sig.set_hash(2);
    Tag2Sub2 tag2sub2; tag2sub2.set_time(time);
    sig.set_hashed_subpackets({&tag2sub2});
    Tag2Sub16 tag2sub16; tag2sub16.set_keyid(keyid);
    sig.set_unhashed_subpackets({&tag2sub16});
    std::string sig_hash = to_sign_13(&sec, &uid, &sig);
    sig.set_left16(sig_hash.substr(0, 2));
    sig.set_mpi(DSA_sign(sig_hash, dsa_pri, dsa_pub));

    // Secret Subkey Packet
    Tag7 ssb;
    ssb.set_version(4);
    ssb.set_time(time);
    ssb.set_pka(16);// ElGamal
    ssb.set_mpi(elgamal_pub);
    ssb.set_s2k_con(254);
    ssb.set_sym(9);// AES

    // Secret Subkey S2K
    S2K3 ssb_s2k3;
    ssb_s2k3.set_hash(2);
    ssb_s2k3.set_salt(unhexlify(bintohex(BBS().rand(64)))); // new salt value
    ssb_s2k3.set_count(96);

    sym_block_len = Symmetric_Algorithm_Block_Length.at(Symmetric_Algorithms.at(ssb.get_sym()));
    key = ssb_s2k3.run(passphrase, sym_block_len >> 3);

    ssb.set_s2k(&ssb_s2k3);
    ssb.set_IV(unhexlify(bintohex(BBS().rand(sym_block_len))));
    secret = write_MPI(elgamal_pri);
    ssb.set_secret(use_normal_CFB_encrypt(9, secret + use_hash(2, secret), key, ssb.get_IV()));

    // Subkey Binding Signature
//    Tag2 subsig = *sign_subkey(0x18, &sec, &ssb, passphrase);
    // This is slightly faster, but I wanted to use the sign_ functions
    Tag2 subsig;
    subsig.set_version(4);
    subsig.set_type(0x18);
    subsig.set_pka(17);
    subsig.set_hash(2);
    subsig.set_hashed_subpackets({&tag2sub2});
    subsig.set_unhashed_subpackets({&tag2sub16});
    sig_hash = to_sign_18(&sec, &ssb, &sig);
    subsig.set_left16(sig_hash.substr(0, 2));
    subsig.set_mpi(DSA_sign(sig_hash, dsa_pri, dsa_pub));

    // Convert private key data to public key data
    std::string data = sec.raw();
    Tag6 pub(data);

    data = ssb.raw();
    Tag14 sub(data);

    // Put key materials into keys
    private_key.set_ASCII_Armor(2);
    private_key.set_Armor_Header({std::pair <std::string, std::string> ("Version", "CC")});
    private_key.set_packets({&sec, &uid, &sig, &ssb, &subsig});

    public_key.set_ASCII_Armor(1);
    public_key.set_Armor_Header({std::pair <std::string, std::string> ("Version", "CC")});
    public_key.set_packets({&pub, &uid, &sig, &sub, &subsig});
}

void add_key_values(PGP & pub, PGP & pri, const std::string & passphrase, const bool new_keyid, const unsigned int pri_key_size, const unsigned int subkey_size){
    BBS((mpz_class) (uint32_t) now()); // seed just in case not seeded

    // at most only 1 of each pair is expected
    std::vector <mpz_class> pub_key;
    std::vector <mpz_class> pri_key;
    std::vector <mpz_class> pub_subkey;
    std::vector <mpz_class> pri_subkey;

    Tag5 * prikey = NULL;
    Tag7 * prisubkey = NULL;
    Tag13 * uid = new Tag13;
    Tag17 * attr = new Tag17;
    bool id = false;            // default UID came first
    bool key = false;           // default main key came first

    std::vector <Packet *> packets = pri.get_packets();
    for(Packet *& p : packets){
        std::string data = p -> raw();
        if (p -> get_tag() == 5){     // Secret Key Packet
            prikey = new Tag5(data);

            // Generate keypair
            std::vector <unsigned int> param;
            // RSA
            if ((prikey -> get_pka() == 1) || (prikey -> get_pka() == 2)/* || (prikey -> get_pka() == 3)*/){
                param = {pri_key_size};
            }
            // ElGamal
            else if (prikey -> get_pka() == 16){
                if (prikey -> get_version() == 3){
                    std::cerr << "Error: Only RSA is defined for version 3 key packets." << std::endl;
                    throw(1);
                }
                param = {pri_key_size};
            }
            // DSA
            else if (prikey -> get_pka() == 17){
                if (prikey -> get_version() == 3){
                    std::cerr << "Error: Only RSA is defined for version 3 key packets." << std::endl;
                    throw(1);
                }
                param = {pri_key_size};
                if (pri_key_size == 1024){
                    param.push_back(160);
                }
                else if (pri_key_size == 2048){
                    param.push_back(256);
                }
                else if (pri_key_size == 3072){
                    param.push_back(256);
                }
                else{
                    std::cerr << "Error: Undefined bit size for DSA: " << pri_key_size<< std::endl;
                    throw(1);
                }
            }
            else{
                std::cerr << "Error: Undefined or reserved PKA number: " << (int) prikey -> get_pka() << std::endl;
                throw(1);
            }

            generate_key_pair(prikey -> get_pka(), param, pub_key, pri_key);

            // put public key into packet
            prikey -> set_mpi(pub_key);

            // put private key into packet
            std::string secret = "";
            for(mpz_class & i : pri_key){
                secret += write_MPI(i);
            }

            std::string check;
            if (prikey -> get_s2k_con() == 254){
                check = use_hash(2, secret);
            }
            else{
                uint16_t sum = 0;
                for(char & c : secret){
                    sum += (uint8_t) c;
                }
                check = unhexlify(makehex(sum, 4));
            }
            std::string k = prikey -> get_s2k() -> run(passphrase, 16);
            prikey -> set_secret(use_normal_CFB_encrypt(prikey -> get_sym(), secret + check, k, prikey -> get_IV()));
            delete p;
            p = prikey;

            key = false;
        }
        else if (p -> get_tag() == 13){    // User ID packet
            uid -> read(data);
            id = false;
        }
        else if (p -> get_tag() == 17){    // User Attribute Packet
            attr -> read(data);
            id = true;
        }
        else if (p -> get_tag() == 2){     // Signature Packet
            Tag2 * sig = new Tag2(data);

            // check that there is a key to be signed
            if (!prikey){
                std::cerr << "Error: No primary key to be signed." << std::endl;
                throw(1);
            }

            // the correct key id
            std::string keyid = prikey -> get_keyid();

            // if fill in the new key id
            if (new_keyid){
                // find Key ID subpacket in the hashed subpackets
                std::vector <Subpacket *> subpackets = sig -> get_hashed_subpackets();
                for(Subpacket *& s : subpackets){
                    if (s -> get_type() == 16){
                        delete s;
                        Tag2Sub16 * t = new Tag2Sub16;
                        t -> set_keyid(keyid);
                        s = t;
                        break;
                    }
                }

                // find Key ID subpacket in the unhashed subpackets
                bool found = false;
                subpackets = sig -> get_unhashed_subpackets();
                for(Subpacket *& s : subpackets){
                    if (s -> get_type() == 16){
                        delete s;
                        Tag2Sub16 * t = new Tag2Sub16;
                        t -> set_keyid(keyid);
                        s = t;
                        found = true;
                        break;
                    }
                }

                // add a new unhashed subpacket
                if (!found){
                    Tag2Sub16 * t = new Tag2Sub16;
                    t -> set_keyid(keyid);
                    subpackets.push_back(t);
                }

                // put new subpackets back, since they are clone of the original
                sig -> set_unhashed_subpackets(subpackets);
            }

            std::string sig_hash;
            if (!key){  // if the key is a primary key
                // get the user id/attribute packet
                ID * i = uid;
                if (id){
                    i = attr;
                }
                if (!i){
                    std::cerr << "Error: No User ID or Attribute packet to be signed." << std::endl;
                    throw(1);
                }
                if (sig -> get_type() == 0x10){
                    sig_hash = to_sign_10(prikey, i, sig);
                }
                else if (sig -> get_type() == 0x11){
                    sig_hash = to_sign_11(prikey, i, sig);
                }
                else if (sig -> get_type() == 0x12){
                    sig_hash = to_sign_12(prikey, i, sig);
                }
                else if (sig -> get_type() == 0x13){
                    sig_hash = to_sign_13(prikey, i, sig);
                }
            }
            else{       // if the key is a subkey
                if (!prisubkey){
                    std::cerr << "Error: No primary key to be signed." << std::endl;
                    throw(1);
                }
                if (sig -> get_type() == 0x18){
                    sig_hash = to_sign_18(prikey, prisubkey, sig);
                }
                else if (sig -> get_type() == 0x19){
                    sig_hash = to_sign_19(prikey, prisubkey, sig);
                }
            }

            // fill in signature fields
            sig -> set_left16(sig_hash.substr(0, 2));
            sig -> set_mpi(pka_sign(sig_hash, sig -> get_pka(), (key?pub_subkey:pub_key), (key?pri_subkey:pri_key)));
            delete p;
            p = sig;
        }
        else if (p -> get_tag() == 7){     // Secret Subkey Packet
            prisubkey = new Tag7(data);

            // Generate keypair
            std::vector <unsigned int> param;
            // RSA
            if ((prisubkey -> get_pka() == 1) || (prisubkey -> get_pka() == 2) /*|| (prisubkey -> get_pka() == 3)*/){
                param = {subkey_size};
            }
            // ElGamal
            else if (prisubkey -> get_pka() == 16){
                if (prisubkey -> get_version() == 3){
                    std::cerr << "Error: Only RSA is defined for version 3 key packets." << std::endl;
                    throw(1);
                }
                param = {subkey_size};
            }
            // DSA
            else if (prisubkey -> get_pka() == 17){
                if (prisubkey -> get_version() == 3){
                    std::cerr << "Error: Only RSA is defined for version 3 key packets." << std::endl;
                    throw(1);
                }
                param = {subkey_size};
                if (subkey_size == 1024){
                    param.push_back(160);
                }
                else if (subkey_size == 2048){
                    param.push_back(256);
                }
                else if (subkey_size == 3072){
                    param.push_back(256);
                }
                else{
                    std::cerr << "Error: Undefined bit size for DSA: " << subkey_size << std::endl;
                    throw(1);
                }
            }
            else{
                std::cerr << "Error: Undefined or reserved PKA number: " << (int) prisubkey -> get_pka() << std::endl;
                throw(1);
            }

            generate_key_pair(prisubkey -> get_pka(), param, pub_subkey, pri_subkey);

            // put publc key into packet
            prisubkey -> set_mpi(pub_subkey);

            // put private key into packet
            std::string secret = "";
            for(mpz_class & i : pri_subkey){
                secret += write_MPI(i);
            }

            std::string check;
            if (prisubkey -> get_s2k_con() == 254){
                check = use_hash(2, secret);
            }
            else{
                uint16_t sum = 0;
                for(char & c : secret){
                    sum += (uint8_t) c;
                }
                check = unhexlify(makehex(sum, 4));
            }
            std::string k = prisubkey -> get_s2k() -> run(passphrase, 16);
            prisubkey -> set_secret(use_normal_CFB_encrypt(prisubkey -> get_sym(), secret + check, k, prisubkey -> get_IV()));
            delete p;
            p = prisubkey;

            key = true;
        }
        else{
            std::cerr << "Error: Packet Tag " << (int) p -> get_tag() << " does not belong in a private key."<< std::endl;
            throw(1);
            break;
        }
    }

    // write changes to public key
    std::vector <Packet *> pub_packets;
    for(Packet * p : packets){
        std::string data = p -> raw();
        if (p -> get_tag() == 5){ // Secret Key packet
            Tag6 * tag6 = new Tag6(data);
            pub_packets.push_back(tag6);
        }
        else if (p -> get_tag() == 7){ // Secret Subkey packet
            Tag14 * tag14 = new Tag14(data);
            pub_packets.push_back(tag14);
        }
        else if ((p -> get_tag() == 2) || (p -> get_tag() == 13) || (p -> get_tag() == 17)){
            pub_packets.push_back(p -> clone());
        }
        else{
            std::cerr << "Error: Packet Tag " << (int) p -> get_tag() << " doesnt belong here." << std::endl;
            throw(1);
            break;
        }
    }
    pub.set_packets(pub_packets);

    delete prikey;
    delete prisubkey;
    delete uid;
    delete attr;
}
