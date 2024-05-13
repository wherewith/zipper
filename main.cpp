#include <errno.h>
#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vector>

#include "Huffman.h"

void compressHuffman(char **buf, size_t *size) {
    std::map<char, unsigned> freq;
    for (size_t i = 0; i < *size; ++i) {
        freq[(*buf)[i]]++;
    }

    std::vector<char> data;
    std::vector<unsigned> frequencies;
    for (const auto &it : freq) {
        data.push_back(it.first);
        frequencies.push_back(it.second);
    }

    auto huffmanTree = HuffmanCodes(data.data(), frequencies.data(), data.size());

    std::string encodedString;
    for (size_t i = 0; i < *size; ++i) {
        encodedString += huffmanTree[(*buf)[i]];
    }

    std::vector<unsigned char> outBuf;
    size_t numUniqueChars = huffmanTree.size();
    size_t totalLength = encodedString.length();

    outBuf.insert(outBuf.end(), reinterpret_cast<char *>(&numUniqueChars), reinterpret_cast<char *>(&numUniqueChars) + sizeof(numUniqueChars));
    outBuf.insert(outBuf.end(), reinterpret_cast<char *>(&totalLength), reinterpret_cast<char *>(&totalLength) + sizeof(totalLength));

    for (auto &it : huffmanTree) {
        outBuf.push_back(it.first);
        size_t codeLength = it.second.size();
        outBuf.insert(outBuf.end(), reinterpret_cast<char *>(&codeLength), reinterpret_cast<char *>(&codeLength) + sizeof(codeLength));
        outBuf.insert(outBuf.end(), it.second.begin(), it.second.end());
    }

    for (size_t i = 0; i < encodedString.size(); i += 8) {
        unsigned char byte = 0;
        for (size_t j = 0; j < 8 && (i + j) < encodedString.size(); ++j) {
            byte |= (encodedString[i + j] == '1' ? 1 : 0) << (7 - j);
        }
        outBuf.push_back(byte);
    }

    free(*buf);
    *size = outBuf.size();
    *buf = (char *)malloc(*size);
    memcpy(*buf, outBuf.data(), *size);
}

void decompressHuffman(char **buf, size_t *size) {
    size_t i = 0;
    std::vector<unsigned char> outBuf;

    size_t numUniqueChars, totalLength;
    memcpy(&numUniqueChars, *buf + i, sizeof(numUniqueChars));
    i += sizeof(numUniqueChars);
    memcpy(&totalLength, *buf + i, sizeof(totalLength));
    i += sizeof(totalLength);

    std::map<std::string, char> codeMap;
    while (numUniqueChars--) {
        char c = (*buf)[i++];
        size_t codeLength;
        memcpy(&codeLength, *buf + i, sizeof(codeLength));
        i += sizeof(codeLength);
        std::string code(*buf + i, *buf + i + codeLength);
        i += codeLength;
        codeMap[code] = c;
    }

    std::string encoded;
    for (; i < *size; ++i) {
        for (int j = 7; j >= 0; --j) {
            encoded.push_back(((*buf)[i] & (1 << j)) ? '1' : '0');
            if (encoded.size() == totalLength) break;
        }
    }

    std::string currentCode;
    for (char bit : encoded) {
        currentCode.push_back(bit);
        if (codeMap.find(currentCode) != codeMap.end()) {
            outBuf.push_back(codeMap[currentCode]);
            currentCode.clear();
        }
    }

    free(*buf);
    *size = outBuf.size();
    *buf = (char *)malloc(*size);
    memcpy(*buf, outBuf.data(), *size);
}

void compressRLE(char **buf, size_t *size) {
    std::vector<unsigned char> outBuf;
    unsigned char p = (*buf)[0];
    size_t count = 1;

    for (size_t i = 1; i < *size; i++) {
        if (p == (*buf)[i]) {
            count++;
        } else {
            outBuf.insert(outBuf.end(), reinterpret_cast<char *>(&count), reinterpret_cast<char *>(&count) + sizeof(count));
            outBuf.push_back(p);
            p = (*buf)[i];
            count = 1;
        }
    }
    outBuf.insert(outBuf.end(), reinterpret_cast<char *>(&count), reinterpret_cast<char *>(&count) + sizeof(count));
    outBuf.push_back(p);

    free(*buf);
    *size = outBuf.size();
    *buf = (char *)malloc(*size);
    memcpy(*buf, outBuf.data(), *size);
}

void decompressRLE(char **buf, size_t *size) {
    std::vector<unsigned char> outBuf;
    size_t i = 0;

    while (i < *size) {
        size_t count;
        memcpy(&count, *buf + i, sizeof(count));
        i += sizeof(count);
        unsigned char c = (*buf)[i++];
        for (size_t j = 0; j < count; j++) {
            outBuf.push_back(c);
        }
    }

    free(*buf);
    *size = outBuf.size();
    *buf = (char *)malloc(*size);
    memcpy(*buf, outBuf.data(), *size);
}

void readFile(const char *filename, char **buf, size_t *size) {
    FILE *file;
    if ((file = fopen(filename, "rb")) == NULL) {
        fprintf(stderr, "Could not open %s with error %d\n", filename, errno);
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    rewind(file);

    *buf = (char *)malloc(*size);
    fread(*buf, 1, *size, file);
    fclose(file);
}

void writeFile(const char *filename, const char *buf, size_t size) {
    FILE *file;
    if ((file = fopen(filename, "wb")) == NULL) {
        fprintf(stderr, "Could not open %s with error %d\n", filename, errno);
        exit(EXIT_FAILURE);
    }
    fwrite(buf, 1, size, file);
    fclose(file);
}

void deleteFile(const char *filename) {
    if (remove(filename) != 0) {
        fprintf(stderr, "Could not delete %s with error %d\n", filename, errno);
    }
}

struct settings {
    enum mode { NONE_MODE,
                COMPRESS,
                DECOMPRESS } mode;
    enum method { NONE_METHOD,
                  RLE,
                  HUFFMAN } method;
};

int main(int argc, char **argv) {
    settings s;
    s.mode = s.NONE_MODE;
    s.method = s.NONE_METHOD;

    int opt;
    char *filename = nullptr;

    while ((opt = getopt(argc, argv, "cdrh")) != -1) {
        switch (opt) {
        case 'c':
            if (s.mode != s.NONE_MODE) {
                fprintf(stderr, "Usage: %s [ -c | -d ] [ -r | -h ] <filename>\n", argv[0]);
                exit(EXIT_FAILURE);
            }
            s.mode = s.COMPRESS;
            break;
        case 'd':
            if (s.mode != s.NONE_MODE) {
                fprintf(stderr, "Usage: %s [ -c | -d ] [ -r | -h ] <filename>\n", argv[0]);
                exit(EXIT_FAILURE);
            }
            s.mode = s.DECOMPRESS;
            break;
        case 'r':
            if (s.method != s.NONE_METHOD) {
                fprintf(stderr, "Usage: %s [ -c | -d ] [ -r | -h ] <filename>\n", argv[0]);
                exit(EXIT_FAILURE);
            }
            s.method = s.RLE;
            break;
        case 'h':
            if (s.method != s.NONE_METHOD) {
                fprintf(stderr, "Usage: %s [ -c | -d ] [ -r | -h ] <filename>\n", argv[0]);
                exit(EXIT_FAILURE);
            }
            s.method = s.HUFFMAN;
            break;
        default:
            fprintf(stderr, "Usage: %s [ -c | -d ] [ -r | -h ] <filename>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (s.mode == s.NONE_MODE || s.method == s.NONE_METHOD) {
        fprintf(stderr, "Both mode and method must be specified\n");
        exit(EXIT_FAILURE);
    }

    if (optind >= argc) {
        fprintf(stderr, "Expected filename after options\n");
        exit(EXIT_FAILURE);
    }

    filename = argv[optind];

    char *fileContent;
    size_t fileSize;
    char outFilename[256];

    readFile(filename, &fileContent, &fileSize);

    if (s.mode == s.COMPRESS) {
        if (s.method == s.RLE) {
            compressRLE(&fileContent, &fileSize);
            snprintf(outFilename, sizeof(outFilename), "%s.rle", filename);
        } else if (s.method == s.HUFFMAN) {
            compressHuffman(&fileContent, &fileSize);
            snprintf(outFilename, sizeof(outFilename), "%s.huf", filename);
        }
        writeFile(outFilename, fileContent, fileSize);
        deleteFile(filename);
    } else if (s.mode == s.DECOMPRESS) {
        if (s.method == s.RLE) {
            decompressRLE(&fileContent, &fileSize);
            snprintf(outFilename, sizeof(outFilename), "%.*s", (int)(strlen(filename) - 4), filename);
        } else if (s.method == s.HUFFMAN) {
            decompressHuffman(&fileContent, &fileSize);
            snprintf(outFilename, sizeof(outFilename), "%.*s", (int)(strlen(filename) - 4), filename);
        }
        writeFile(outFilename, fileContent, fileSize);
        deleteFile(filename);
    }


    free(fileContent);
    return 0;
}
