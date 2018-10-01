#include "ykcmp.h"
#include "util.h"

#include <iostream>

namespace yk {
    std::vector<char> compress(const std::vector<char>& data, bool naive) {
        unsigned int chunks = data.size() / 0x7F;
        unsigned int rest = data.size() % 0x7F;

        size_t maxZSize = chunks * 0x80;
        if(rest != 0) maxZSize += rest + 1;
        std::vector<char> result = { 'Y','K','C','M','P','_','V','1','\x04','\0','\0','\0' };
        result.resize(maxZSize + 0x14);
        writeU32(result, 0x10, data.size());

        unsigned int percentage = 0;
        if(naive) {
            auto dIter = data.begin();
            auto rIter = result.begin() + 0x14;
            for(int i = 0; i < chunks; i++) {
                *(rIter++) = 0x7F;
                std::copy(dIter, dIter + 0x7F, rIter);
                dIter += 0x7F; rIter += 0x7F;

                unsigned int newPercentage = std::fmin(100.f, static_cast<float>(rIter - result.begin()) / maxZSize * 100.f);
                for(; percentage < newPercentage; percentage++) {
                    std::cout << '#';
                }
            }
            if(rest != 0) {
                *(rIter++) = rest;
                std::copy(dIter, dIter + rest, rIter);
            }
            for (; percentage < 100; percentage++) {
                std::cout << '#';
            }

            writeU32(result, 0x0C, maxZSize + 0x14);
        } else {
            auto dIter = data.begin();
            auto rIter = result.begin() + 0x14;
            auto copyHead = result.end();

            while(dIter < data.end()) {
                unsigned int newPercentage = std::fmin(100.f, static_cast<float>(dIter - data.begin()) / data.size() * 100.f);
                for (; percentage < newPercentage; percentage++) {
                    std::cout << '#';
                }

                int maxOffset = 0x1000;
                const int maxSz[] = { 0x3, 0x1F, 0x1FF };
                int sz = 0, offset = 0, saved = 0;
                for (int i = 3; i > 0; i--) {
                    for (auto start = dIter - maxOffset; start < dIter - saved - i; start++) {
                        if (start < data.begin()) {
                            start = data.begin();
                        }
                        if (*start != *dIter) {
                            continue;
                        }

                        int s = -i;
                        auto leftIter = start, rightIter = dIter;
                        while (rightIter < data.end() && *leftIter == *rightIter && s < maxSz[i - 1] && leftIter < dIter) {
                            leftIter++;
                            rightIter++;
                            s++;
                        }

                        if (s > saved) {
                            sz = s + i;
                            offset = dIter - start;
                            saved = s;
                        }
                    }
                    maxOffset /= 0x10;
                    if (saved > maxOffset) {
                        continue;
                    }
                }

                if(copyHead != result.end()) {
                    if(saved <= 0) {
                        if(*copyHead >= 0x7F) {
                            copyHead = rIter;
                            *(rIter++) = 0;
                        }
                        *(rIter++) = *(dIter++);
                        (*copyHead)++;
                        continue;
                    } else {
                        copyHead = result.end();
                    }
                }

                if(saved <= 0) {
                    copyHead = rIter;
                    *(rIter++) = 1;
                    *(rIter++) = *(dIter++);
                    continue;
                }

                if(sz <= 0x4 && offset <= 0x10) {
                    *(rIter++) = (sz << 4) + 0x70 + (offset - 1);
                } else if(sz <= 0x21 && offset <= 0x100) {
                    *(rIter++) = sz + 0xC0 - 2;
                    *(rIter++) = offset - 1;
                } else {
                    int tmp = sz + 0xE00 - 3;
                    *(rIter++) = (tmp >> 4);
                    *(rIter++) = ((tmp & 0x0F) << 4) + ((offset - 1) >> 8);
                    *(rIter++) = ((offset - 1) & 0xFF);
                }

                dIter += sz;
            }
            for (; percentage < 100; percentage++) {
                std::cout << '#';
            }

            size_t zsize = rIter - result.begin();
            result.resize(zsize);
            writeU32(result, 0x0C, zsize);
        }

        std::cout << '\n';
        return result;
    }

    std::vector<char> decompress(const std::vector<char>& data) {
        size_t finalSize = readU32(data, 0x10);
        std::vector<char> result;
        result.resize(finalSize);

        auto rIter = result.begin();
        auto dIter = data.begin() + 0x14;
        unsigned int percentage = 0;
        for(; dIter < data.end() && rIter < result.end(); ) {
            unsigned char tmp = *(dIter++);

            if(tmp < 0x80) {
                std::copy(dIter, dIter + tmp, rIter);
                dIter += tmp; rIter += tmp;
            } else {
                size_t sz, offset;
                if(tmp < 0xC0) {
                    sz = (tmp >> 4) - 0x8 + 1;
                    offset = (tmp & 0x0F) + 1;
                } else if(tmp < 0xE0) {
                    sz = tmp - 0xC0 + 2;
                    offset = static_cast<unsigned char>(*(dIter++)) + 1;
                } else {
                    unsigned char tmp2 = *(dIter++);
                    unsigned char tmp3 = *(dIter++);
                    sz = (tmp << 4) + (tmp2 >> 4) - 0xE00 + 3;
                    offset = ((tmp2 & 0x0F) << 8) + tmp3 + 1;
                }

                std::copy(rIter - offset, rIter - offset + sz, rIter);
                rIter += sz;
            }

            unsigned int newPercentage = std::fmin(100.f, static_cast<float>(rIter - result.begin()) / finalSize * 100.f);
            for (; percentage < newPercentage; percentage++) {
                std::cout << '#';
            }
        }

        std::cout << '\n';
        return result;
    }
}
