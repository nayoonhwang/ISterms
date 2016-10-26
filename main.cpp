#include <stdio.h>
#include <stdlib.h>

//각종 string 관련 함수를 사용할 수 있게 해준다.
#include <string.h>

typedef unsigned char BYTE;//1바이트

//1바이트니깐 범위가 0~255이다.

typedef unsigned int WORD;//4바이트

/* 일단 과정을 생각해보자
 * Key size: 32 bits (4byte)
 * Block size: 48 bits (6byte)
 * Structure: SP-Network with 10 rounds
 * S-box is the same as AES
 * M: Linear Transition
 * M is a 3x3 matrix in GF(28)
 * GF(2^8)is the same field in AES
 *
 * key schedule(key expansion)
 * 5 circular shift
 *
 * Rotl(x,n)
 *
 */

BYTE S_box[256]=
        {
                0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
                0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
                0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
                0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
                0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
                0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
                0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
                0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
                0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
                0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
                0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
                0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
                0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
                0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
                0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
                0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
        };

BYTE new_key[68];
BYTE* text=(BYTE*)malloc(sizeof(BYTE)*6);
WORD Rotl(WORD key);
BYTE* Cipher(BYTE* in, BYTE *key);
void KeyExpansion(BYTE* key, BYTE* new_key);

/*
BYTE multi(BYTE in){
    BYTE num=in;
    num= num<<1;
    //2의 7승 초과
    if(num > 255){
        //num-=256;
        num^=27;
        return num;
    }
    return num;
}
 */
//multi 에 문제있음
BYTE multi(BYTE in)
{
    BYTE tmp=0x1B;
    if(in & 0x80){
        in = (in << 1);
        in = in ^ tmp;
        return in;
    }
    in = in << 1;
    return in;
}

BYTE* Cipher(BYTE* in, BYTE* key){
    int i,j,k,r;
    //6개의 byte를 저장하는 text 배열을 선언.
    for(i=0;i<6;i++){
        text[i]=in[i];
    }
    //plaintext in 배열 입력

    //key배열이 들어옴.(4바이트) key-> BYTE[4]

    //544 bit가 들어갈 new_key
    KeyExpansion(key, new_key);

    for(i=0;i<6;i++){
        text[i]= new_key[i] ^ text[i];
    }//add key

    //10번 반복한다.
    for(r=0;r<10;r++)
    {
        for(i=0;i<6;i++){
            text[i]=S_box[text[i]];//sub
        }
        /*tmp를 만들어서 후에 연산을 대비했다.이 과정이없었기에 계속 에러가 발생했다.
         * in[0]=in[0]^multi(in[1])^multi(in[3])
         * 이런식으로 하게되면,
         * in[3]=in[4]^multi(in[5])^multi(in[0]) 이런 과정에서 수정된 in[0]가 들어가기 떄문에
         * 행렬 곱셈의 결과가 달라질 수 있다. 즉 기존의 in[0],in[4],in[5]로 column vector를 생성한것이기 때문에 vector값이 달라지면 안된다.
         * 따라서, tmp라는 바이트 배열을 생성해서 기존의 in배열값을 모두 저장했다.
         * */

        BYTE tmp[6]={text[0],text[1],text[2],text[3],text[4],text[5]};
        //연산(곱셈)
        text[0]=tmp[1]^multi(tmp[2])^multi(tmp[3]);
        text[1]=multi(tmp[1])^multi(tmp[2])^tmp[3];
        text[2]=multi(tmp[1]) ^ tmp[2] ^ multi(tmp[3]);

        text[3]=tmp[4] ^ multi(tmp[5]) ^ multi(tmp[0]);
        text[4]=multi(tmp[4]) ^ multi(tmp[5]) ^ tmp[0];
        text[5]=multi(tmp[4]) ^ tmp[5]^ multi(tmp[0]);

        for(i=0;i<6;i++){
            text[i]^=new_key[r*6+i+6];
        }//add key
    /*
        for (i=0;i<6;i++)
            printf("ciphertext: %x",in[i]);
    */
        printf("\n");

    }
    return text;
}

void add(BYTE* in, BYTE* new_key){
    int i;
    for(i=0;i<6;i++){
        in[i]^=new_key[i];
    }//add key
}

//left circular shift by 5
WORD Rotl(WORD key){
    return ((key & 0xF8000000) >> 27| key << 5);
}

void KeyExpansion(BYTE* key, BYTE* new_key){
    //4바이트(32bit) 짜리 word 선언.
    //key배열은 4byte짜리.
    WORD word=(((WORD)key[0]<<24)|((WORD)key[1]<<16)| ((WORD)key[2]<<8)|((WORD)key[3]));

    //64바이트짜리 k 배열 선언
    WORD K[16];//k0~k15

    int i;
    int j=0;
    K[0]=Rotl(word);

    new_key[0]=key[0];
    new_key[1]=key[1];
    new_key[2]=key[2];
    new_key[3]=key[3];

    for(i=1;i<17;i++){
        //순환 쉬프트
        K[i]=Rotl(K[i-1]);

        //new_key에 입력. 1바이트씩.
        new_key[i * 4 + j++] = (BYTE)((K[i-1] & 0xFF000000) >> 24);
        new_key[i * 4 + j++] = (BYTE)((K[i-1] & 0x00FF0000) >> 16);
        new_key[i * 4 + j++] = (BYTE)((K[i-1] & 0x0000FF00) >> 8);
        new_key[i * 4 + j] = (BYTE)(K[i-1] & 0x000000FF);
        j=0;
    }
    //key 초기화 완료
}
WORD bytetoword(BYTE* key){
    WORD word=(((WORD)key[0]<<24)|((WORD)key[1]<<16)| ((WORD)key[2]<<8)|((WORD)key[3]));
    return word;
}
void wordtobyte(WORD word, BYTE* bytearr){
    bytearr[0]=(BYTE)(word & 0xFF000000 >> 24);
    bytearr[1] = (BYTE)(word & 0x00FF0000 >> 16);
    bytearr[2] = (BYTE)(word & 0x0000FF00 >> 8);
    bytearr[3] = (BYTE)(word & 0x000000FF);
}

int Decrypt_KEY(BYTE* plaintext, BYTE* ciphertext){
    BYTE BF_key[4]={0x3e,0x76,0xac,0x00};
    WORD BF_word=(((WORD)BF_key[0]<<24)|((WORD)BF_key[1]<<16)| ((WORD)BF_key[2]<<8)|((WORD)BF_key[3]));
    //4바이트로 변환.
    int i=0;
    BYTE* BF_ciphertext;//48bit

    for(;;BF_word++) {
        wordtobyte(BF_word, BF_key);
        BF_ciphertext=Cipher(plaintext, BF_key);

        for(i=0;i<6;) {
            if (BF_ciphertext[i] == ciphertext[i]){
                if(i==5)
                    goto final;
                else
                    i++;
            }
            else
                break;
        }
        printf("tried: %x",BF_word);
    }
    final:
        printf("key: %x",BF_word);
        return 0;
}

int main(){
    //sample plaintext(text)
    BYTE plain_sample[6]={0x00,0x11,0x22,0x33,0x44,0x55};
    //6바이트 짜리 plaintext
    BYTE plaintext1[6]={0x01,0x23,0x45,0x67,0x89,0xAB};
    BYTE plaintext2[6]={0x9A,0x6B,0xCC,0x10,0xE8,0x4A};
    //BYTE key[4]={0x3e,0x76,0xac,0x4b}; //sample key
    BYTE find_cipher[6]={0xFB,0xD0,0x40,0xD6,0xDB,0x9C};
    BYTE* ciphertext;
    //ciphertext = Cipher(plaintext1,key);
    int i;
    /*
    for (i=0;i<6;i++)
        printf("ciphertext: %x",ciphertext[i]);
    */
    Decrypt_KEY(plaintext1,find_cipher);

    return 0;
}
