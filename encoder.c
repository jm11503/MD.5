#include "encoder.h"


int main(int argc, char *argv[]) {

    if (--argc < 1) {
        printf("Function takes a minimum of one argument!");
        exit(0);
    }

    else {
      if (!strcmp(argv[1], "-f")) {
        FILE *fp = fopen(argv[2], "rb");
        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        char *buffer = calloc(++size, 1);
        fread(buffer, size, 1, fp);
        fclose(fp);
        encode(buffer);
      }
      else {
        encode(argv[1]);
      }
    }

}

void encode(char *plaintext) {

    int len = strlen(plaintext);
    int bit_len = len * CHAR_IN_BITS;

    char *binaryString = stringToBinaryString(plaintext);
    int binaryLength = strlen(binaryString);
    // binaryString can't be appended so we need to copy it into a string buffer of proper length

    // must make sure string is of length 512 % n = 0; must have 64 bit padding, so pre-padded string must be n%512 = 448%512
    char *paddedBinaryString = binaryStringToPaddedBinaryString(binaryLength, binaryString);
    free(binaryString); // no need to waste memory

    //printf("%s\n", paddedBinaryString);

    int i;
    u_int32_t cA = A;
    u_int32_t cB = B;
    u_int32_t cC = C;
    u_int32_t cD = D;
    //printf("Initial Digest: %x%x%x%x\n", cA, cB, cC, cD);
    u_int32_t result;
    for (i = 0; i < strlen(paddedBinaryString); i += 512) { // main loop; digest occurs in 512-bit blocks
        char *block = calloc(512, CHAR_IN_BITS);
        strncat(block, &paddedBinaryString[i], 512);
        char strWords[16][33]; // create 16 words for operations
        u_int32_t words[16];
        int j;
        int counter = 0;
        for (j = 0; j < 16; j += 1) {
            strncpy(strWords[j], &block[j * 32], 32);
            strWords[j][32] = '\0';
            //words[j] = fromBinaryStringtoInt(strWords[j]); // converts binary strings to decimal equivalents
            int h;
            for (h = 0; h < 32; h++) {
              char c = strWords[j][h];
              int x = abs(atoi(&c));
              int mask = 1 << h;
              words[j] = ((words[j] & ~mask) | (x << h));
            }
        }
        for (j = 0; j < 4; j++) {
            int h;
            for (h = 0; h < 16; h++) {
              switch (j) {
                case 0:
                  result = fmod((cA + F(cB, cC, cD)), MODADD);
                  //printf("Round %d, %d pre-result: %ld\n", j, h, result);
                  result = fmod((words[round1sums[h]] + result), MODADD);
                  //printf("Round %d, %d post-result: %ld\n", j, h, result);
                  break;
                case 1:
                  result = fmod((cA + G(cB, cC, cD)), MODADD);
                  result = fmod((words[round2sums[h]] + result), MODADD);
                  break;
                case 2:
                  result = fmod((cA + H(cB, cC, cD)), MODADD);
                  result = fmod((words[round3sums[h]] + result), MODADD);
                  break;
                default:
                  result = fmod((cA + I(cB, cC, cD)), MODADD);
                  result = fmod((words[round4sums[h]] + result), MODADD);
                  break;
              }
              result = fmod((K[counter] + result), MODADD);
              counter++;
              switch (j) {
                case 0:
                  result = ROTATE(result, SHIFT1[h]);
                  break;
                case 1:
                  result = ROTATE(result, SHIFT2[h]);
                  break;
                case 2:
                  result = ROTATE(result, SHIFT3[h]);
                  break;
                default:
                  result = ROTATE(result, SHIFT4[h]);
                  break;
              }
              result = fmod((result + cB), MODADD);
              u_int32_t tmpA = cA;
              u_int32_t tmpB = cB;
              u_int32_t tmpC = cC;
              u_int32_t tmpD = cD;
              cA = cD;
              cB = result;
              cC = tmpB;
              cD = tmpC;
              //printf("result: %d \t A: %x\n", result, cA);
            }
        }
        cA = fmod((cA + A), MODADD);
        cB = fmod((cB + B), MODADD);
        cC = fmod((cC + C), MODADD);
        cD = fmod((cD + D), MODADD);
    }

    printf("Hash: %x%x%x%x\n", cA, cB, cC, cD);

}

int fromBinaryStringtoInt(const char *s) {
  return (int) strtol(s, NULL, 2);
}

const char *byte_to_binary(u_int32_t x) {
    static char b[9];
    b[0] = '\0';

    int z;
    for (z = 128; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}

char * stringToBinaryString(char *plaintext) {

    int len = strlen(plaintext);
    char *binaryString = calloc(len * CHAR_IN_BITS + 1, CHAR_IN_BITS);

    int i;
    char *ptr;
    char *start = binaryString;

    for (ptr = plaintext; *ptr != '\0'; ptr++) {
        for (i = 7; i >= 0; i--, binaryString++) {
            *binaryString = (*ptr & 1 << i) ? '1' : '0';
        }
    }

    *binaryString = '\0';
    binaryString = start;

    return binaryString;
}

char * binaryStringToPaddedBinaryString(int binaryLength, char *binaryString) {

    int binaryLengthCopy = binaryLength;

    char *paddedBinaryString = (binaryLength % 512 <= 448%512) ? calloc(binaryLength + 512 - (binaryLength % 512), CHAR_IN_BITS) : calloc(1024 + binaryLength - (binaryLength % 512), CHAR_IN_BITS);
    strncpy(paddedBinaryString, binaryString, strlen(binaryString));

    // pad until 64 bits left until next multiple of 512; 1 followed by zeroes
    if (binaryLength % 512 != 448 % 512) {
        strncat(paddedBinaryString, "1", strlen("1"));
        binaryLength++;
    }
    while (binaryLength % 512 != 448 % 512) {
        strncat(paddedBinaryString, "0", strlen("1"));
        binaryLength++;
    }

    // now we add least significant bits of binary representation of length until n%512 = 0, space before is zeroes
    char *reversedBinaryString = int2rebin(binaryLengthCopy);

    int zeroes = 64 - strlen(reversedBinaryString);

    int i;
    for (i = 0; i < zeroes; i++) {
        strncat(paddedBinaryString, "0", strlen("0"));
        binaryLength++;
    }
    for (i = 0; i < strlen(reversedBinaryString) && binaryLength % 512 != 0; i++, binaryLength++) {
        strncat(paddedBinaryString, &reversedBinaryString[i], 1);
    }

    return paddedBinaryString;

}

// convert integer to reversed binary string
char *int2rebin(int binaryLength) {

    int bits = sizeof(int) * CHAR_IN_BITS;
    char *string = calloc(bits + 1, CHAR_IN_BITS);

    int i;
    for (i = bits - 1; i >= 0; i--) {
        string[i] = (binaryLength & 1) + '0';
        binaryLength >>= 1;
    }

    int len_counter = 0;
    while (string[len_counter] != '1')
        len_counter++;

    char *string2 = &string[len_counter]; // get rid of unnecessary zeroes

    // reverse the string
    char *start = string2;
    char *end = start + strlen(string2) - 1;
    char temp;

    while (end > start) {
        temp = *start;
        *start = *end;
        *end = temp;

        ++start;
        --end;
    }

    return string2;

}
