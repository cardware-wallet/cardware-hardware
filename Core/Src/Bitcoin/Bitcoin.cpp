#include "Bitcoin.h"
#include "Hash.h"
#include "Conversion.h"

#include <stdint.h>
#include <string.h>

#include "utility/crypto/bip39.h"
#include "utility/crypto/ecdsa.h"
#include "utility/crypto/memzero.h"
#include "utility/crypto/rfc6979.h"
#include "utility/crypto/secp256k1.h"
#include "utility/crypto/sha2.h"
#include "utility/segwit_addr.h"

#if USE_STD_STRING
using std::string;
#define String std::string
#endif

// error code when parsing fails
int ubtc_errno = 0;

const char * generateMnemonic(uint8_t numWords){
    if(numWords<12 || numWords > 24 || numWords % 3 != 0){
        return NULL;
    }
    int strength = numWords*32/3;
    return mnemonic_generate(strength);
}
const char * generateMnemonic(uint8_t numWords, const uint8_t * entropy_data, size_t dataLen){
    if(numWords<12 || numWords > 24 || numWords % 3 != 0){
        return NULL;
    }
    uint8_t hash[32];
    sha256(entropy_data, dataLen, hash);
    size_t len = numWords*4/3;
    return mnemonic_from_data(hash, len);
}

const char * mnemonicFromEntropy(const uint8_t * entropy_data, size_t dataLen){
    return mnemonic_from_data(entropy_data, dataLen);
}
size_t mnemonicToEntropy(const char * mnemonic, size_t mnemonicLen, uint8_t * output, size_t outputLen){
    int num_words = 1;
    for (size_t i = 0; i < strlen(mnemonic); i++){
        if(mnemonic[i] == ' '){
            num_words ++;
        }
    }
    size_t entropy_len = (num_words*4)/3;
    if(outputLen < entropy_len){
        return 0;
    }
    uint8_t res[33] = {0};
    int r = mnemonic_to_entropy(mnemonic, res);
    if(r == 0){
        return 0;
    }
    memcpy(output, res, entropy_len);
    return entropy_len;
}
#if USE_ARDUINO_STRING || USE_STD_STRING
size_t mnemonicToEntropy(String mnemonic, uint8_t * output, size_t outputLen){
    return mnemonicToEntropy(mnemonic.c_str(), mnemonic.length(), output, outputLen);
}
#else
size_t mnemonicToEntropy(char * mnemonic, uint8_t * output, size_t outputLen){
    return mnemonicToEntropy(mnemonic, strlen(mnemonic), output, outputLen);
}
#endif

const char * generateMnemonic(const uint8_t * entropy_data, size_t dataLen){
    return generateMnemonic(24, entropy_data, dataLen);
}
#if !(USE_ARDUINO_STRING || USE_STD_STRING)
const char * generateMnemonic(uint8_t numWords, const char * entropy_string){
    return generateMnemonic(numWords, (uint8_t*)entropy_string, strlen(entropy_string));
}
const char * generateMnemonic(const char * entropy_string){
    return generateMnemonic(24, entropy_string);
}
bool checkMnemonic(const char * mnemonic){
    return mnemonic_check(mnemonic);
}
#else
const char * generateMnemonic(uint8_t numWords, const String entropy_string){
    return generateMnemonic(numWords, (uint8_t*)entropy_string.c_str(), strlen(entropy_string.c_str()));
}
const char * generateMnemonic(const String entropy_string){
    return generateMnemonic(24, entropy_string);
}
bool checkMnemonic(const String mnemonic){
    return mnemonic_check(mnemonic.c_str());
}
#endif

// ---------------------------------------------------------------- Signature class

Signature::Signature(){
    memzero(tot, 3); index = 0;
    memzero(r, 32);
    memzero(s, 32);
}
Signature::Signature(const uint8_t r_arr[32], const uint8_t s_arr[32]){
    memzero(tot, 3); index = 0;
    memcpy(r, r_arr, 32);
    memcpy(s, s_arr, 32);
}
Signature::Signature(const uint8_t * der){
    memzero(tot, 3); index = 0; memzero(r, 32); memzero(s, 32);
    fromDer(der, der[1]+2);
}
Signature::Signature(const uint8_t * der, size_t derLen){
    memzero(tot, 3); index = 0; memzero(r, 32); memzero(s, 32);
    fromDer(der, derLen);
}
Signature::Signature(const char * der){
    memzero(tot, 3); index = 0; memzero(r, 32); memzero(s, 32);
    ParseByteStream s(der);
    Signature::from_stream(&s);
}
size_t Signature::from_stream(ParseStream *stream){
    // der encoding is probably the most uneffective way to encode signatures...
    // Format: 0x30 [total-length] 0x02 [R-length] [R] 0x02 [S-length] [S]
    // * total-length: 1-byte length descriptor of everything that follows
    // * R-length: 1-byte length descriptor of the R value that follows.
    // * R: arbitrary-length big-endian encoded R value. It must use the shortest
    //   possible encoding for a positive integers (which means no null bytes at
    //   the start, except a single one when the next byte has its highest bit set).
    // * S-length: 1-byte length descriptor of the S value that follows.
    // * S: arbitrary-length big-endian encoded S value. The same rules apply.
    if(status == PARSING_FAILED){
        return 0;
    }
    if(status == PARSING_DONE){
        bytes_parsed = 0;
        memzero(tot, 3);
        memzero(r, 32);
        memzero(s, 32);
    }
    status = PARSING_INCOMPLETE;
    size_t bytes_read = 0;
    uint8_t c = 0;
    if(stream->available() && bytes_parsed+bytes_read < 1){
        c = stream->read();
        bytes_read++;
        if(c!=0x30){
            status = PARSING_FAILED;
            return bytes_read;
        }
    }
    if(stream->available() && bytes_parsed+bytes_read < 2){
        tot[0] = stream->read();
        bytes_read++;
        if(tot[0] > 70){ status = PARSING_FAILED; return bytes_read; }
    }
    // r
    if(stream->available() && bytes_parsed+bytes_read < 3){
        c = stream->read();
        bytes_read++;
        if(c != 0x02){ status = PARSING_FAILED; return bytes_read; }
    }
    if(stream->available() && bytes_parsed+bytes_read < 4){
        tot[1] = stream->read();
        bytes_read++;
        if(tot[1] > 33){ status = PARSING_FAILED; return bytes_read; }
    }
    if(stream->available() && tot[1]==33 && bytes_parsed+bytes_read < 5){
        c = stream->read();
        bytes_read++;
        if(c != 0){ status = PARSING_FAILED; return bytes_read; }
    }
    while(stream->available() && bytes_parsed+bytes_read < (size_t)4+tot[1]){
        r[bytes_parsed+bytes_read-4+32-tot[1]] = stream->read();
        bytes_read++;
    }
    if(rlen() != tot[1]){ status = PARSING_FAILED; return bytes_read; }
    // s
    if(stream->available() && bytes_parsed+bytes_read < (size_t)4+tot[1]+1){
        c = stream->read();
        bytes_read++;
        if(c != 0x02){ status = PARSING_FAILED; return bytes_read; }
    }
    if(stream->available() && bytes_parsed+bytes_read < (size_t)4+tot[1]+2){
        tot[2] = stream->read();
        bytes_read++;
        if(tot[2] > 33){ status = PARSING_FAILED; return bytes_read; }
    }
    if(stream->available() && tot[2]==33 && bytes_parsed+bytes_read < (size_t)4+tot[1]+3){
        c = stream->read();
        bytes_read++;
        if(c != 0){ status = PARSING_FAILED; return bytes_read; }
    }
    while(stream->available() && bytes_parsed+bytes_read < (size_t)4+tot[1]+2+tot[2]){
        s[bytes_parsed+bytes_read-4-tot[1]-2-tot[2]+32] = stream->read();
        bytes_read++;
    }
    if(slen() != tot[2]){ status = PARSING_FAILED; return bytes_read; }
    if(bytes_parsed+bytes_read == (size_t)4+tot[1]+2+tot[2]){
        status = PARSING_DONE;
    }
    bytes_parsed+=bytes_read;
    return bytes_read;
}

size_t Signature::to_stream(SerializeStream *stream, size_t offset) const{
    uint8_t arr[72];
    der(arr, sizeof(arr));
    size_t l = Signature::length();
    size_t bytes_written = 0;
    while(stream->available() && offset+bytes_written < l){
        stream->write(arr[offset+bytes_written]);
        bytes_written++;
    }
    return bytes_written;
}

size_t Signature::rlen() const{
    uint8_t len = 33;
    for(int i=0; i<32; i++){
        if(r[i] > 0){
            if(r[i] < 0x80){
                len --;
            }
            break;
        }else{
            len--;
        }
    }
    return len;
}
size_t Signature::slen() const{
    uint8_t len = 33;
    for(int i=0; i<32; i++){
        if(s[i] > 0){
            if(s[i] < 0x80){
                len --;
            }
            break;
        }else{
            len--;
        }
    }
    return len;
}
size_t Signature::length() const{
    return rlen()+slen()+6;
}

size_t Signature::fromDer(const uint8_t * raw, size_t rawLen){
    ParseByteStream stream(raw, rawLen);
    return Signature::from_stream(&stream);
}
size_t Signature::der(uint8_t * bytes, size_t len) const{
    memzero(bytes, len);
    uint8_t _rlen = rlen();
    uint8_t _slen = slen();
    bytes[0] = 0x30;
    bytes[1] = 4+_rlen+2+_slen-2;
    bytes[2] = 0x02;
    bytes[3] = _rlen;
    if(_rlen == 33){
        memcpy(bytes+5, r, 32);
    }else{
        memcpy(bytes+4, r+32-_rlen, _rlen);
    }

    bytes[4+_rlen] = 0x02;
    bytes[4+_rlen+1] = _slen;
    if(_slen == 33){
        memcpy(bytes+4+_rlen+3, s, 32);
    }else{
        memcpy(bytes+4+_rlen+2, s+32-_slen, _slen);
    }
    return 4+_rlen+2+_slen;
}
void Signature::bin(uint8_t * arr, size_t len) const{
    size_t l = len;
    if(l > 32){
        l = 32;
    }
    memcpy(arr, r, l);
    if(len > 32){
        l = len-32;
        if(l > 32){
            l = 32;
        }
        memcpy(arr+32, s, l);
    }
    if(len > 64){
        arr[64] = index;
    }
}
void Signature::fromBin(const uint8_t * arr, size_t len){
    size_t l = len;
    if(l > 32){
        l = 32;
    }
    memcpy(r, arr, l);
    if(len > 32){
        l = len-32;
        if(l > 32){
            l = 32;
        }
        memcpy(s, arr+32, l);
    }
    if(len > 64){
        index = arr[64];
    }
}

// ---------------------------------------------------------------- SchnorrSignature

SchnorrSignature::SchnorrSignature(){
    memzero(r, 32);
    memzero(s, 32);
}
SchnorrSignature::SchnorrSignature(const uint8_t r_arr[32], const uint8_t s_arr[32]){
    memcpy(r, r_arr, 32);
    memcpy(s, s_arr, 32);
}
SchnorrSignature::SchnorrSignature(const uint8_t rs_arr[64]){
    memcpy(r, rs_arr, 32);
    memcpy(s, rs_arr+32, 32);
}
SchnorrSignature::SchnorrSignature(const char * rs){
    memzero(r, 32); memzero(s, 32);
    ParseByteStream s(rs);
    SchnorrSignature::from_stream(&s);
}
size_t SchnorrSignature::from_stream(ParseStream *stream){
    if(status == PARSING_FAILED){
        return 0;
    }
    if(status == PARSING_DONE){
        bytes_parsed = 0;
        memzero(r, 32);
        memzero(s, 32);
    }
    status = PARSING_INCOMPLETE;
    size_t bytes_read = 0;
    while(stream->available() && bytes_parsed+bytes_read < 32){
        r[bytes_read+bytes_parsed] = stream->read();
        bytes_read++;
    }
    while(stream->available() && bytes_parsed+bytes_read < 64){
        s[bytes_read+bytes_parsed-32] = stream->read();
        bytes_read++;
    }
    if(bytes_parsed+bytes_read == 64){
        status = PARSING_DONE;
    }
    bytes_parsed+=bytes_read;
    return bytes_read;
}

size_t SchnorrSignature::to_stream(SerializeStream *stream, size_t offset) const{
    uint8_t arr[64];
    memcpy(arr, r, 32);
    memcpy(arr+32, s, 32);
    size_t l = sizeof(arr);
    size_t bytes_written = 0;
    while(stream->available() && offset+bytes_written < l){
        stream->write(arr[offset+bytes_written]);
        bytes_written++;
    }
    return bytes_written;
}

// ---------------------------------------------------------------- PublicKey class

int PublicKey::legacyAddress(char * address, size_t len, const Network * network) const{
    memzero(address, len);

    uint8_t buffer[20];
    uint8_t sec_arr[65] = { 0 };
    int l = sec(sec_arr, sizeof(sec_arr));
    hash160(sec_arr, l, buffer);

    uint8_t addr[21];
    addr[0] = network->p2pkh;
    memcpy(addr+1, buffer, 20);

    return toBase58Check(addr, 21, address, len);
}
#if USE_ARDUINO_STRING || USE_STD_STRING
String PublicKey::legacyAddress(const Network * network) const{
    char addr[40] = { 0 };
    legacyAddress(addr, sizeof(addr), network);
    return String(addr);
}
#endif
int PublicKey::segwitAddress(char address[], size_t len, const Network * network) const{
    memzero(address, len);
    if(len < 76){ // TODO: 76 is too much for native segwit
        return 0;
    }
    uint8_t hash[20];
    uint8_t sec_arr[65] = { 0 };
    int l = sec(sec_arr, sizeof(sec_arr));
    hash160(sec_arr, l, hash);
    segwit_addr_encode(address, network->bech32, 0, hash, 20);
    return 76;
}
#if USE_ARDUINO_STRING || USE_STD_STRING
String PublicKey::segwitAddress(const Network * network) const{
    char addr[76] = { 0 };
    segwitAddress(addr, sizeof(addr), network);
    return String(addr);
}
#endif
int PublicKey::nestedSegwitAddress(char address[], size_t len, const Network * network) const{
    memzero(address, len);
    uint8_t script[22] = { 0 };
    // script[0] = 0x00; // no need to set - already zero
    script[1] = 0x14;
    uint8_t sec_arr[65] = { 0 };
    int l = sec(sec_arr, sizeof(sec_arr));
    hash160(sec_arr, l, script+2);

    uint8_t addr[21];
    addr[0] = network->p2sh;
    hash160(script, 22, addr+1);

    return toBase58Check(addr, 21, address, len);
}
#if USE_ARDUINO_STRING || USE_STD_STRING
String PublicKey::nestedSegwitAddress(const Network * network) const{
    char addr[40] = { 0 };
    nestedSegwitAddress(addr, sizeof(addr), network);
    return String(addr);
}
#endif
Script PublicKey::script(ScriptType type) const{
    return Script(*this, type);
}
bool PublicKey::verify(const Signature sig, const uint8_t hash[32]) const{
    uint8_t signature[64] = {0};
    sig.bin(signature, 64);
    uint8_t pub[65];
    serialize(pub, 65);
    return (ecdsa_verify_digest(&secp256k1, pub, signature, hash)==0);
}
bool PublicKey::schnorr_verify(const SchnorrSignature sig, const uint8_t hash[32]) const{
    PublicKey pub = *this;
    if(!pub.isEven()){
        pub = -pub;
    }
    uint8_t rs[64];
    sig.serialize(rs, sizeof(rs));
    PublicKey R;
    R.from_x(rs, 32);
    // calculate hash using tagged hash with "BIP0340/challenge" prefix
    uint8_t e[32];
    uint8_t tmp[32];
    TaggedHash tch("BIP0340/challenge");
    // write R
    tch.write(rs, 32);
    // write xonly pubkey
    pub.x(tmp, sizeof(tmp));
    tch.write(tmp, sizeof(tmp));
    // write message
    tch.write(hash, 32);
    tch.end(e);
    PrivateKey challenge(e);
    PublicKey S = challenge*pub + R;
    S.x(tmp, sizeof(tmp));
    PrivateKey s(rs+32);
    s.publicKey().x(e, 32);
    return memcmp(tmp, e, 32) == 0;
};

// ---------------------------------------------------------------- PrivateKey class

size_t PrivateKey::from_stream(ParseStream *s){
    if(status == PARSING_FAILED){
        return 0;
    }
    if(status == PARSING_DONE){
        bytes_parsed = 0;
    }
    status = PARSING_INCOMPLETE;
    size_t bytes_read = 0;
    while(s->available() > 0 && bytes_parsed+bytes_read < 32){
        num[bytes_parsed+bytes_read] = s->read();
        bytes_read++;
    }
    if(bytes_parsed+bytes_read == 32){
        status = PARSING_DONE;
        uint8_t zero[32] = { 0 };
        if(memcmp(num, zero, 32)==0){ // should we add something else here?
            status = PARSING_FAILED;
        }
        bignum256 n;
        bn_read_be(num, &n);
        bn_mod(&n, &secp256k1.order);
        bn_write_be(&n, num);
        pubKey = *this * GeneratorPoint;
    }
    bytes_parsed += bytes_read;
    return bytes_read;
}
PrivateKey::PrivateKey(void){
    reset();
    memzero(num, 32); // empty key
    network = &DEFAULT_NETWORK;
}
PrivateKey::PrivateKey(const uint8_t * secret_arr, bool use_compressed, const Network * net){
    reset();
    memcpy(num, secret_arr, 32);
    network = net;
    pubKey = *this * GeneratorPoint;
    pubKey.compressed = use_compressed;
}
PrivateKey::PrivateKey(const ECScalar other){
    reset();
    other.getSecret(num);
    pubKey = *this * GeneratorPoint;
    pubKey.compressed = true;
    network = &DEFAULT_NETWORK;
}
/*PrivateKey &PrivateKey::operator=(const PrivateKey &other){
    if (this == &other){ return *this; } // self-assignment
    reset();
    other.getSecret(num);
    network = other.network;
    pubKey = *this * GeneratorPoint;
    pubKey.compressed = other.pubKey.compressed;
    return *this;
};*/
PrivateKey::~PrivateKey(void) {
    reset();
    // erase secret key from memory
    memzero(num, 32);
}
int PrivateKey::wif(char * wifArr, size_t wifSize) const{
    memzero(wifArr, wifSize);

    uint8_t wifHex[34] = { 0 }; // prefix + 32 bytes secret (+ compressed )
    size_t len = 33;
    wifHex[0] = network->wif;
    memcpy(wifHex+1, num, 32);
    if(pubKey.compressed){
        wifHex[33] = 0x01;
        len++;
    }
    size_t l = toBase58Check(wifHex, len, wifArr, wifSize);

    memzero(wifHex, sizeof(wifHex)); // secret should not stay in RAM
    return l;
}
#if USE_ARDUINO_STRING || USE_STD_STRING
String PrivateKey::wif() const{
    char wifString[53] = { 0 };
    wif(wifString, sizeof(wifString));
    return String(wifString);
}
#endif
int PrivateKey::fromWIF(const char * wifArr, size_t wifSize){
    uint8_t arr[40] = { 0 };
    size_t l = fromBase58Check(wifArr, wifSize, arr, sizeof(arr));
    if( (l < 33) || (l > 34) ){
        memzero(num, 32);
        return 0;
    }
    bool compressed;
    network = &DEFAULT_NETWORK;
    bool found = false;
    for(int i=0; i<networks_len; i++){
        if(arr[0] == networks[i]->wif){
            network = networks[i];
            found = true;
            break;
        }
    }
    if(!found){
        return 0;
    }

    if(l == 34){
        compressed = (arr[33] > 0);
    }
    if(l == 33){
        compressed = false;
    }
    memcpy(num, arr+1, 32);
    memzero(arr, 40); // clear memory

    pubKey = *this * GeneratorPoint;
    pubKey.compressed = compressed;
    return 1;
}
int PrivateKey::fromWIF(const char * wifArr){
    return fromWIF(wifArr, strlen(wifArr));
}

PublicKey PrivateKey::publicKey() const{
    return pubKey;
}

int PrivateKey::address(char * address, size_t len) const{
    return pubKey.address(address, len, network);
}
int PrivateKey::legacyAddress(char * address, size_t len) const{
    return pubKey.legacyAddress(address, len, network);
}
int PrivateKey::segwitAddress(char * address, size_t len) const{
    return pubKey.segwitAddress(address, len, network);
}
int PrivateKey::nestedSegwitAddress(char * address, size_t len) const{
    return pubKey.nestedSegwitAddress(address, len, network);
}
#if USE_ARDUINO_STRING || USE_STD_STRING
String PrivateKey::address() const{
    return pubKey.address(network);
}
String PrivateKey::legacyAddress() const{
    return pubKey.legacyAddress(network);
}
String PrivateKey::segwitAddress() const{
    return pubKey.segwitAddress(network);
}
String PrivateKey::nestedSegwitAddress() const{
    return pubKey.nestedSegwitAddress(network);
}
#endif

int PrivateKey::ecdh(const PublicKey pub, uint8_t shared_secret[32], bool use_hash){
    // calculate pk * pub, serialize as uncompressed point and hash <x><y>
    ECPoint mult = *this * pub;
    mult.compressed = false;
    uint8_t sec[65];
    mult.sec(sec, sizeof(sec));
    if(use_hash){
        sha256(sec+1, 64, shared_secret);
    }else{
        // just copy x
        memcpy(shared_secret, sec+1, 32);
    }
    return 1;
}

static int is_canonical(uint8_t by, uint8_t sig[64]){
  return 1;
}

Signature PrivateKey::sign(const uint8_t hash[32]) const{
    uint8_t signature[65] = {0};
    uint8_t i = 0;
    ecdsa_sign_digest(&secp256k1, num, hash, signature, &i, &is_canonical);
    Signature sig(signature, signature+32);
    sig.index = i;
    return sig;
}

SchnorrSignature PrivateKey::schnorr_sign(const uint8_t hash[32]) const{
    PrivateKey prv = *this;
    PublicKey pub = prv.publicKey();
    // check if pubkey is even, if not - negate
    if(!pub.isEven()){
        prv = -prv;
        pub = -pub;
    }
    uint8_t r[32];
    uint8_t s[32];
    uint8_t tmp[32];
    // generate k using tagged hash with "BIP0340/nonce" prefix
    uint8_t nonce[32];
    TaggedHash tnonce("BIP0340/nonce");
    prv.getSecret(tmp);
    tnonce.write(tmp, sizeof(tmp));
    pub.x(tmp, sizeof(tmp));
    tnonce.write(tmp, sizeof(tmp));
    tnonce.write(hash, 32);
    tnonce.end(nonce);
    PrivateKey k(nonce);
    PublicKey R = k.publicKey();
    // flip k if r is not even
    if(!R.isEven()){
        k = -k;
        R = -R;
    }

    // calculate hash using tagged hash with "BIP0340/challenge" prefix
    uint8_t e[32];
    TaggedHash tch("BIP0340/challenge");
    R.x(r, sizeof(r));
    tch.write(r, sizeof(r));
    pub.x(tmp, sizeof(tmp));
    tch.write(tmp, sizeof(tmp));
    tch.write(hash, 32);
    tch.end(e);
    PrivateKey challenge(e);
    // calculate s
    PrivateKey S = k + challenge*prv;
    S.getSecret(s);

    SchnorrSignature sig(r, s);
    return sig;
}
#if USE_ARDUINO_STRING || USE_STD_STRING
PrivateKey::PrivateKey(const String wifString){
    fromWIF(wifString.c_str());
}
#else
PrivateKey::PrivateKey(const char * wifArr){
    fromWIF(wifArr);
}
#endif
