#ifndef __BITCOIN_CURVE_H__
#define __BITCOIN_CURVE_H__

#include "uBitcoin_conf.h"
#include "BaseClasses.h"
#include "Conversion.h"
#include "utility/crypto/memzero.h"

class ECPoint : public Streamable{
protected:
    virtual size_t from_stream(ParseStream *s);
    virtual size_t to_stream(SerializeStream *s, size_t offset = 0) const;
public:
    uint8_t point[64];  // point on curve: (x,y)
    bool compressed;

    virtual void reset(){ bytes_parsed = 0; status=PARSING_DONE; memset(point, 0, 64); compressed = true; };
    virtual size_t length() const{ return 33 + 32*(1-compressed); };
    virtual size_t stringLength() const{ return 2*ECPoint::length(); };

    ECPoint(){ reset(); };
    ECPoint(const uint8_t pubkeyArr[64], bool use_compressed);
    ECPoint(const uint8_t * secArr);
    explicit ECPoint(const char * secHex);

    size_t sec(uint8_t * arr, size_t len) const;
    size_t fromSec(const uint8_t * arr, size_t len);
    /** \brief fills array with x coordinate of the point */
    size_t x(uint8_t * arr, size_t len) const{
        if(len < 32){
            return 0;
        }
        memcpy(arr, point, 32);
        return 32;
    };
    /** \brief parses x-only pubkey, from two possible points selects one with even y */
    size_t from_x(const uint8_t * arr, size_t len){
        if(len < 32){
            return 0;
        }
        uint8_t sec[33] = {0x02};
        memcpy(sec+1, arr, 32);
        return fromSec(sec, sizeof(sec));
    }
#if USE_ARDUINO_STRING
    String sec() const{
        char arr[65*2+1] = "";
        SerializeByteStream stream(arr, sizeof(arr));
        ECPoint::to_stream(&stream);
        if(compressed){
            arr[33*2+1] = 0;
        }
        String s(arr);
        return s;
    };
    String x() const{
        uint8_t arr[32];
        x(arr, sizeof(arr));
        return toHex(arr, sizeof(arr));
    };
#endif
#if USE_STD_STRING
    std::string sec() const{
        char arr[65*2+1] = "";
        SerializeByteStream stream(arr, sizeof(arr));
        ECPoint::to_stream(&stream);
        if(compressed){
            arr[33*2+1] = 0;
        }
        std::string s(arr);
        return s;
    };
    std::string x() const{
        uint8_t arr[32];
        x(arr, sizeof(arr));
        return toHex(arr, sizeof(arr));
    };
#endif
    // bool verify(const Signature sig, const uint8_t hash[32]) const;
    virtual bool isValid() const;
    /** \brief checks if pubkey has even Y coordinate */
    bool isEven() const;
    explicit operator bool() const { return isValid(); };
    bool operator==(const ECPoint& other) const{ return (memcmp(point, other.point, 64) == 0); };
    bool operator!=(const ECPoint& other) const{ return !operator==(other); };

    ECPoint operator+(const ECPoint& other) const;
    ECPoint operator-() const;
    ECPoint operator-(const ECPoint& other) const;
    ECPoint operator+=(const ECPoint& other){ *this = *this+other; return *this; };
    ECPoint operator-=(const ECPoint& other){ *this = *this-other; return *this; };

    // sec-hex-comparison for multisig sorting
    bool operator<(const ECPoint& other) const;
    bool operator>(const ECPoint& other) const{ return (other<*this); };
    bool operator>=(const ECPoint& other) const{ return !(*this<other); };
    bool operator<=(const ECPoint& other) const{ return !(*this>other); };
};

extern const ECPoint InfinityPoint;
extern const ECPoint GeneratorPoint;

class ECScalar : public Streamable{
protected:
    virtual size_t from_stream(ParseStream *s);
    virtual size_t to_stream(SerializeStream *s, size_t offset = 0) const;
    uint8_t num[32];  // scalar mod secp526k1.order
    virtual void init(){ bytes_parsed = 0; status=PARSING_DONE; memzero(num, 32); };
public:
    virtual void reset(){ bytes_parsed = 0; status=PARSING_DONE; memzero(num, 32); };
    virtual size_t length() const{ return 32; };

    ECScalar(){ init(); };
    ECScalar(const uint8_t * arr, size_t len){ init(); parse(arr, len); };
    explicit ECScalar(const char * arr){ init(); parse(arr, strlen(arr)); };
    ECScalar(uint32_t i){ init(); intToBigEndian(i, num, 32); };
    ~ECScalar(){ memzero(num, 32); };

    /** \brief Populates array with the secret key */
    virtual void setSecret(const uint8_t secret_arr[32]){ memcpy(num, secret_arr, 32); };
    /** \brief Sets the secret key */
    void getSecret(uint8_t buffer[32]) const{ memcpy(buffer, num, 32); };

    ECScalar operator+(const ECScalar& other) const;
    ECScalar operator+(const uint32_t& i) const;
    ECScalar operator-() const;
    ECScalar operator-(const ECScalar& other) const;
    ECScalar operator-(const uint32_t& i) const;
    ECScalar operator+=(const ECScalar& other){ *this = *this+other; return *this; };
    ECScalar operator-=(const ECScalar& other){ *this = *this-other; return *this; };
    ECScalar operator+=(const uint32_t& i){ *this = *this+i; return *this; };
    ECScalar operator-=(const uint32_t& i){ *this = *this-i; return *this; };

    ECScalar operator*(const ECScalar& other) const;
    ECScalar operator/(const ECScalar& other) const;
    ECScalar operator*=(const ECScalar& other){ *this = *this*other; return *this; };
    ECScalar operator/=(const ECScalar& other){ *this = *this/other; return *this; };
    
    virtual bool isValid() const{ uint8_t arr[32] = { 0 }; return (memcmp(num, arr, 32) != 0); };
    explicit operator bool() const { return isValid(); };
    bool operator==(const ECScalar& other) const{ return (memcmp(num, other.num, 32) == 0); };
    bool operator!=(const ECScalar& other) const{ return !operator==(other); };
    bool operator<(const ECScalar& other) const;
    bool operator>(const ECScalar& other) const{ return (other<*this); };
    bool operator>=(const ECScalar& other) const{ return !(*this<other); };
    bool operator<=(const ECScalar& other) const{ return !(*this>other); };
};

inline ECScalar operator/(uint32_t i, ECScalar& scalar){ return ECScalar(i) / scalar; };
inline ECScalar operator/(ECScalar& scalar, uint32_t i){ return scalar / ECScalar(i); };
inline ECScalar operator*(uint32_t i, ECScalar& scalar){ return ECScalar(i) * scalar; };
inline ECScalar operator*(ECScalar& scalar, uint32_t i){ return scalar * ECScalar(i); };
inline ECScalar operator+(uint32_t i, ECScalar& scalar){ return ECScalar(i) + scalar; };
inline ECScalar operator+(ECScalar& scalar, uint32_t i){ return scalar + ECScalar(i); };
inline ECScalar operator-(uint32_t i, ECScalar& scalar){ return ECScalar(i) - scalar; };
inline ECScalar operator-(ECScalar& scalar, uint32_t i){ return scalar - ECScalar(i); };

ECPoint operator*(const ECScalar& d, const ECPoint& p);
inline ECPoint operator*(const ECPoint& p, const ECScalar& d){ return d*p; };
inline ECPoint operator/(const ECPoint& p, const ECScalar& d){ return (ECScalar(1)/d)*p; };

#endif // __BITCOIN_CURVE_H__
