#include "Body.h"
#pragma optimize("", off)

// ***************       Please cover this file with file generated by key_obf.py      *************** //
// ***************    This is an example of password Dt!5OoxWyIHsh#p~s6LKOmEeRJI44!AW  *************** //
// ***************      DO NOT USE THIS PASSWORD & CODE IN PRODUCTION ENVIRONMENTS     *************** //

namespace EncryptionKey {
void f00(unsigned char* part) {
    unsigned char v09 = static_cast<unsigned char>(144 ^ 69 ^ 112 ^ 236);
    part[0] = v09;
    unsigned char v18 = static_cast<unsigned char>(188 ^ 174 ^ 64);
    part[1] = v18;
    unsigned char v0c = static_cast<unsigned char>(191 ^ 0 ^ 21 ^ 202 ^ 8);
    part[2] = v0c;
    unsigned char v0a = static_cast<unsigned char>(173 ^ 119 ^ 146);
    part[3] = v0a;
}

void f01(unsigned char* part) {
    unsigned char v06 = static_cast<unsigned char>(82 ^ 11 ^ 33);
    part[0] = v06;
    unsigned char v1f = static_cast<unsigned char>(43 ^ 19 ^ 205 ^ 28 ^ 190);
    part[1] = v1f;
    unsigned char v17 = static_cast<unsigned char>(105 ^ 118 ^ 26 ^ 64 ^ 32);
    part[2] = v17;
    unsigned char v0d = static_cast<unsigned char>(185 ^ 219 ^ 219 ^ 154);
    part[3] = v0d;
}

void f02(unsigned char* part) {
    unsigned char v02 = static_cast<unsigned char>(23 ^ 34 ^ 151 ^ 89 ^ 76 ^ 150);
    part[0] = v02;
    unsigned char v13 = static_cast<unsigned char>(215 ^ 77 ^ 175 ^ 126);
    part[1] = v13;
    unsigned char v1e = static_cast<unsigned char>(21 ^ 15 ^ 217 ^ 130);
    part[2] = v1e;
    unsigned char v16 = static_cast<unsigned char>(100 ^ 90 ^ 200 ^ 179);
    part[3] = v16;
}

void f03(unsigned char* part) {
    unsigned char v19 = static_cast<unsigned char>(179 ^ 253 ^ 204 ^ 124 ^ 36 ^ 144);
    part[0] = v19;
    unsigned char v14 = static_cast<unsigned char>(28 ^ 143 ^ 220);
    part[1] = v14;
    unsigned char v04 = static_cast<unsigned char>(34 ^ 25 ^ 84 ^ 32);
    part[2] = v04;
    unsigned char v01 = static_cast<unsigned char>(176 ^ 239 ^ 43);
    part[3] = v01;
}

void f04(unsigned char* part) {
    unsigned char v1c = static_cast<unsigned char>(5 ^ 84 ^ 208 ^ 88 ^ 237);
    part[0] = v1c;
    unsigned char v05 = static_cast<unsigned char>(49 ^ 33 ^ 127);
    part[1] = v05;
    unsigned char v10 = static_cast<unsigned char>(215 ^ 93 ^ 86 ^ 89 ^ 231 ^ 17);
    part[2] = v10;
    unsigned char v0e = static_cast<unsigned char>(253 ^ 215 ^ 189 ^ 217 ^ 188 ^ 130);
    part[3] = v0e;
}

void f05(unsigned char* part) {
    unsigned char v12 = static_cast<unsigned char>(93 ^ 206 ^ 5 ^ 21 ^ 207);
    part[0] = v12;
    unsigned char v07 = static_cast<unsigned char>(60 ^ 252 ^ 61 ^ 216 ^ 114);
    part[1] = v07;
    unsigned char v1a = static_cast<unsigned char>(233 ^ 249 ^ 89);
    part[2] = v1a;
    unsigned char v0f = static_cast<unsigned char>(197 ^ 30 ^ 77 ^ 232);
    part[3] = v0f;
}

void f06(unsigned char* part) {
    unsigned char v1d = static_cast<unsigned char>(40 ^ 12 ^ 195 ^ 3 ^ 9 ^ 204);
    part[0] = v1d;
    unsigned char v11 = static_cast<unsigned char>(205 ^ 71 ^ 68 ^ 248);
    part[1] = v11;
    unsigned char v1b = static_cast<unsigned char>(38 ^ 91 ^ 187 ^ 175 ^ 95 ^ 2);
    part[2] = v1b;
    unsigned char v0b = static_cast<unsigned char>(33 ^ 37 ^ 119);
    part[3] = v0b;
}

void f07(unsigned char* part) {
    unsigned char v00 = static_cast<unsigned char>(49 ^ 184 ^ 205);
    part[0] = v00;
    unsigned char v08 = static_cast<unsigned char>(222 ^ 92 ^ 251);
    part[1] = v08;
    unsigned char v15 = static_cast<unsigned char>(148 ^ 128 ^ 241 ^ 92 ^ 212);
    part[2] = v15;
    unsigned char v03 = static_cast<unsigned char>(181 ^ 129 ^ 148 ^ 188 ^ 133 ^ 172);
    part[3] = v03;
}
}

std::array<uint8_t, 32> ResourcePack::get_aes_key() {
    std::array<uint8_t, 32> key = {};
    unsigned char part[4];
    EncryptionKey::f00(part);
    key[9] = part[0];
    key[24] = part[1];
    key[12] = part[2];
    key[10] = part[3];
    EncryptionKey::f01(part);
    key[6] = part[0];
    key[31] = part[1];
    key[23] = part[2];
    key[13] = part[3];
    EncryptionKey::f02(part);
    key[2] = part[0];
    key[19] = part[1];
    key[30] = part[2];
    key[22] = part[3];
    EncryptionKey::f03(part);
    key[25] = part[0];
    key[20] = part[1];
    key[4] = part[2];
    key[1] = part[3];
    EncryptionKey::f04(part);
    key[28] = part[0];
    key[5] = part[1];
    key[16] = part[2];
    key[14] = part[3];
    EncryptionKey::f05(part);
    key[18] = part[0];
    key[7] = part[1];
    key[26] = part[2];
    key[15] = part[3];
    EncryptionKey::f06(part);
    key[29] = part[0];
    key[17] = part[1];
    key[27] = part[2];
    key[11] = part[3];
    EncryptionKey::f07(part);
    key[0] = part[0];
    key[8] = part[1];
    key[21] = part[2];
    key[3] = part[3];
    return key;
}
#pragma optimize("", on)
