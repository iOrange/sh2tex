unsigned int gsmem[1024 * 1024];

int block32[32] =
{
     0,  1,  4,  5, 16, 17, 20, 21,
     2,  3,  6,  7, 18, 19, 22, 23,
     8,  9, 12, 13, 24, 25, 28, 29,
    10, 11, 14, 15, 26, 27, 30, 31
};


int columnWord32[16] =
{
     0,  1,  4,  5,  8,  9, 12, 13,
     2,  3,  6,  7, 10, 11, 14, 15
};

void writeTexPSMCT32(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void* data)
{
    unsigned int* src = (unsigned int*)data;
    int startBlockPos = dbp * 64;

    for (int y = dsay; y < dsay + rrh; y++)
    {
        for (int x = dsax; x < dsax + rrw; x++)
        {
            int pageX = x / 64;
            int pageY = y / 32;
            int page = pageX + pageY * dbw;

            int px = x - (pageX * 64);
            int py = y - (pageY * 32);

            int blockX = px / 8;
            int blockY = py / 8;
            int block = block32[blockX + blockY * 8];

            int bx = px - blockX * 8;
            int by = py - blockY * 8;

            int column = by / 2;

            int cx = bx;
            int cy = by - column * 2;
            int cw = columnWord32[cx + cy * 8];

            gsmem[startBlockPos + page * 2048 + block * 64 + column * 16 + cw] = *src;
            src++;
        }
    }
}

void readTexPSMCT32(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void* data)
{
    unsigned int* src = (unsigned int*)data;
    int startBlockPos = dbp * 64;

    for (int y = dsay; y < dsay + rrh; y++)
    {
        for (int x = dsax; x < dsax + rrw; x++)
        {
            int pageX = x / 64;
            int pageY = y / 32;
            int page = pageX + pageY * dbw;

            int px = x - (pageX * 64);
            int py = y - (pageY * 32);

            int blockX = px / 8;
            int blockY = py / 8;
            int block = block32[blockX + blockY * 8];

            int bx = px - blockX * 8;
            int by = py - blockY * 8;

            int column = by / 2;

            int cx = bx;
            int cy = by - column * 2;
            int cw = columnWord32[cx + cy * 8];

            *src = gsmem[startBlockPos + page * 2048 + block * 64 + column * 16 + cw];
            src++;
        }
    }
}

int blockZ32[32] =
{
     24, 25, 28, 29, 8, 9, 12, 13,
     26, 27, 30, 31,10, 11, 14, 15,
     16, 17, 20, 21, 0, 1, 4, 5,
     18, 19, 22, 23, 2, 3, 6, 7
};

int columnWordZ32[16] =
{
     0,  1,  4,  5,  8,  9, 12, 13,
     2,  3,  6,  7, 10, 11, 14, 15
};

void writeTexPSMZ32(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void* data)
{
    unsigned int* src = (unsigned int*)data;
    int startBlockPos = dbp * 64;

    for (int y = dsay; y < dsay + rrh; y++)
    {
        for (int x = dsax; x < dsax + rrw; x++)
        {
            int pageX = x / 64;
            int pageY = y / 32;
            int page = pageX + pageY * dbw;

            int px = x - (pageX * 64);
            int py = y - (pageY * 32);

            int blockX = px / 8;
            int blockY = py / 8;
            int block = blockZ32[blockX + blockY * 8];

            int bx = px - blockX * 8;
            int by = py - blockY * 8;

            int column = by / 2;

            int cx = bx;
            int cy = by - column * 2;
            int cw = columnWordZ32[cx + cy * 8];

            gsmem[startBlockPos + page * 2048 + block * 64 + column * 16 + cw] = *src;
            src++;
        }
    }
}

void readTexPSMZ32(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void* data)
{
    unsigned int* src = (unsigned int*)data;
    int startBlockPos = dbp * 64;

    for (int y = dsay; y < dsay + rrh; y++)
    {
        for (int x = dsax; x < dsax + rrw; x++)
        {
            int pageX = x / 64;
            int pageY = y / 32;
            int page = pageX + pageY * dbw;

            int px = x - (pageX * 64);
            int py = y - (pageY * 32);

            int blockX = px / 8;
            int blockY = py / 8;
            int block = blockZ32[blockX + blockY * 8];

            int bx = px - blockX * 8;
            int by = py - blockY * 8;

            int column = by / 2;

            int cx = bx;
            int cy = by - column * 2;
            int cw = columnWordZ32[cx + cy * 8];

            *src = gsmem[startBlockPos + page * 2048 + block * 64 + column * 16 + cw];
            src++;
        }
    }
}

int block16[32] =
{
     0,  2,  8, 10,
     1,  3,  9, 11,
     4,  6, 12, 14,
     5,  7, 13, 15,
    16, 18, 24, 26,
    17, 19, 25, 27,
    20, 22, 28, 30,
    21, 23, 29, 31
};

int columnWord16[32] =
{
     0,  1,  4,  5,  8,  9, 12, 13,   0,  1,  4,  5,  8,  9, 12, 13,
     2,  3,  6,  7, 10, 11, 14, 15,   2,  3,  6,  7, 10, 11, 14, 15
};

int columnHalf16[32] =
{
    0, 0, 0, 0, 0, 0, 0, 0,  1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0,  1, 1, 1, 1, 1, 1, 1, 1
};



void writeTexPSMCT16(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void* data)
{
    //dbw >>= 1;
    unsigned short* src = (unsigned short*)data;
    int startBlockPos = dbp * 64;

    for (int y = dsay; y < dsay + rrh; y++)
    {
        for (int x = dsax; x < dsax + rrw; x++)
        {
            int pageX = x / 64;
            int pageY = y / 64;
            int page = pageX + pageY * dbw;

            int px = x - (pageX * 64);
            int py = y - (pageY * 64);

            int blockX = px / 16;
            int blockY = py / 8;
            int block = block16[blockX + blockY * 4];

            int bx = px - blockX * 16;
            int by = py - blockY * 8;

            int column = by / 2;

            int cx = bx;
            int cy = by - column * 2;
            int cw = columnWord16[cx + cy * 16];
            int ch = columnHalf16[cx + cy * 16];

            unsigned short* dst = (unsigned short*)&gsmem[startBlockPos + page * 2048 + block * 64 + column * 16 + cw];
            dst[ch] = *src;
            src++;
        }
    }
}

void readTexPSMCT16(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void* data)
{
    //dbw >>= 1;
    unsigned short* src = (unsigned short*)data;
    int startBlockPos = dbp * 64;

    for (int y = dsay; y < dsay + rrh; y++)
    {
        for (int x = dsax; x < dsax + rrw; x++)
        {
            int pageX = x / 64;
            int pageY = y / 64;
            int page = pageX + pageY * dbw;

            int px = x - (pageX * 64);
            int py = y - (pageY * 64);

            int blockX = px / 16;
            int blockY = py / 8;
            int block = block16[blockX + blockY * 4];

            int bx = px - blockX * 16;
            int by = py - blockY * 8;

            int column = by / 2;

            int cx = bx;
            int cy = by - column * 2;
            int cw = columnWord16[cx + cy * 16];
            int ch = columnHalf16[cx + cy * 16];

            unsigned short* dst = (unsigned short*)&gsmem[startBlockPos + page * 2048 + block * 64 + column * 16 + cw];
            *src = dst[ch];
            src++;
        }
    }
}

int blockZ16[32] =
{
     24,  26,  16, 18,
     25,  27,  17, 19,
    28,  30, 20, 22,
     29,  31, 21, 23,
    8, 10, 0, 2,
    9, 11, 1, 3,
    12, 14, 4, 6,
    13, 15, 5, 7
};

int columnWordZ16[32] =
{
     0,  1,  4,  5,  8,  9, 12, 13,   0,  1,  4,  5,  8,  9, 12, 13,
     2,  3,  6,  7, 10, 11, 14, 15,   2,  3,  6,  7, 10, 11, 14, 15
};

int columnHalfZ16[32] =
{
    0, 0, 0, 0, 0, 0, 0, 0,  1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0,  1, 1, 1, 1, 1, 1, 1, 1
};

void writeTexPSMZ16(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void* data)
{
    //dbw >>= 1;
    unsigned short* src = (unsigned short*)data;
    int startBlockPos = dbp * 64;

    for (int y = dsay; y < dsay + rrh; y++)
    {
        for (int x = dsax; x < dsax + rrw; x++)
        {
            int pageX = x / 64;
            int pageY = y / 64;
            int page = pageX + pageY * dbw;

            int px = x - (pageX * 64);
            int py = y - (pageY * 64);

            int blockX = px / 16;
            int blockY = py / 8;
            int block = blockZ16[blockX + blockY * 4];

            int bx = px - blockX * 16;
            int by = py - blockY * 8;

            int column = by / 2;

            int cx = bx;
            int cy = by - column * 2;
            int cw = columnWordZ16[cx + cy * 16];
            int ch = columnHalfZ16[cx + cy * 16];

            unsigned short* dst = (unsigned short*)&gsmem[startBlockPos + page * 2048 + block * 64 + column * 16 + cw];
            dst[ch] = *src;
            src++;
        }
    }
}

void readTexPSMZ16(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void* data)
{
    //dbw >>= 1;
    unsigned short* src = (unsigned short*)data;
    int startBlockPos = dbp * 64;

    for (int y = dsay; y < dsay + rrh; y++)
    {
        for (int x = dsax; x < dsax + rrw; x++)
        {
            int pageX = x / 64;
            int pageY = y / 64;
            int page = pageX + pageY * dbw;

            int px = x - (pageX * 64);
            int py = y - (pageY * 64);

            int blockX = px / 16;
            int blockY = py / 8;
            int block = blockZ16[blockX + blockY * 4];

            int bx = px - blockX * 16;
            int by = py - blockY * 8;

            int column = by / 2;

            int cx = bx;
            int cy = by - column * 2;
            int cw = columnWordZ16[cx + cy * 16];
            int ch = columnHalfZ16[cx + cy * 16];

            unsigned short* dst = (unsigned short*)&gsmem[startBlockPos + page * 2048 + block * 64 + column * 16 + cw];
            *src = dst[ch];
            src++;
        }
    }
}

int blockZ16S[32] =
{
     24,  26,  8, 10,
     25,  27,  9, 11,
     16,  18, 0, 2,
     17,  19, 1, 3,
    28, 30, 12, 14,
    29, 31, 13, 15,
    20, 22, 4, 6,
    21, 23, 5, 7
};

int columnWordZ16S[32] =
{
     0,  1,  4,  5,  8,  9, 12, 13,   0,  1,  4,  5,  8,  9, 12, 13,
     2,  3,  6,  7, 10, 11, 14, 15,   2,  3,  6,  7, 10, 11, 14, 15
};

int columnHalfZ16S[32] =
{
    0, 0, 0, 0, 0, 0, 0, 0,  1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0,  1, 1, 1, 1, 1, 1, 1, 1
};

void writeTexPSMZ16S(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void* data)
{
    //dbw >>= 1;
    unsigned short* src = (unsigned short*)data;
    int startBlockPos = dbp * 64;

    for (int y = dsay; y < dsay + rrh; y++)
    {
        for (int x = dsax; x < dsax + rrw; x++)
        {
            int pageX = x / 64;
            int pageY = y / 64;
            int page = pageX + pageY * dbw;

            int px = x - (pageX * 64);
            int py = y - (pageY * 64);

            int blockX = px / 16;
            int blockY = py / 8;
            int block = blockZ16S[blockX + blockY * 4];

            int bx = px - blockX * 16;
            int by = py - blockY * 8;

            int column = by / 2;

            int cx = bx;
            int cy = by - column * 2;
            int cw = columnWordZ16S[cx + cy * 16];
            int ch = columnHalfZ16S[cx + cy * 16];

            unsigned short* dst = (unsigned short*)&gsmem[startBlockPos + page * 2048 + block * 64 + column * 16 + cw];
            dst[ch] = *src;
            src++;
        }
    }
}
void readTexPSMZ16S(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void* data)
{
    //dbw >>= 1;
    unsigned short* src = (unsigned short*)data;
    int startBlockPos = dbp * 64;

    for (int y = dsay; y < dsay + rrh; y++)
    {
        for (int x = dsax; x < dsax + rrw; x++)
        {
            int pageX = x / 64;
            int pageY = y / 64;
            int page = pageX + pageY * dbw;

            int px = x - (pageX * 64);
            int py = y - (pageY * 64);

            int blockX = px / 16;
            int blockY = py / 8;
            int block = blockZ16S[blockX + blockY * 4];

            int bx = px - blockX * 16;
            int by = py - blockY * 8;

            int column = by / 2;

            int cx = bx;
            int cy = by - column * 2;
            int cw = columnWordZ16S[cx + cy * 16];
            int ch = columnHalfZ16S[cx + cy * 16];

            unsigned short* dst = (unsigned short*)&gsmem[startBlockPos + page * 2048 + block * 64 + column * 16 + cw];
            *src = dst[ch];
            src++;
        }
    }
}

int block16S[32] =
{
     0,  2, 16, 18,
     1,  3, 17, 19,
     8, 10, 24, 26,
     9, 11, 25, 27,
     4,  6, 20, 22,
     5,  7, 21, 23,
    12, 14, 28, 30,
    13, 15, 29, 31
};

int columnWord16S[32] =
{
     0,  1,  4,  5,  8,  9, 12, 13,   0,  1,  4,  5,  8,  9, 12, 13,
     2,  3,  6,  7, 10, 11, 14, 15,   2,  3,  6,  7, 10, 11, 14, 15
};

int columnHalf16S[32] =
{
    0, 0, 0, 0, 0, 0, 0, 0,  1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0,  1, 1, 1, 1, 1, 1, 1, 1
};

void writeTexPSMCT16S(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void* data)
{
    //dbw >>= 1;
    unsigned short* src = (unsigned short*)data;
    int startBlockPos = dbp * 64;

    for (int y = dsay; y < dsay + rrh; y++)
    {
        for (int x = dsax; x < dsax + rrw; x++)
        {
            int pageX = x / 64;
            int pageY = y / 64;
            int page = pageX + pageY * dbw;

            int px = x - (pageX * 64);
            int py = y - (pageY * 64);

            int blockX = px / 16;
            int blockY = py / 8;
            int block = block16S[blockX + blockY * 4];

            int bx = px - blockX * 16;
            int by = py - blockY * 8;

            int column = by / 2;

            int cx = bx;
            int cy = by - column * 2;
            int cw = columnWord16S[cx + cy * 16];
            int ch = columnHalf16S[cx + cy * 16];

            unsigned short* dst = (unsigned short*)&gsmem[startBlockPos + page * 2048 + block * 64 + column * 16 + cw];
            dst[ch] = *src;
            src++;
        }
    }
}

void readTexPSMCT16S(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void* data)
{
    //dbw >>= 1;
    unsigned short* src = (unsigned short*)data;
    int startBlockPos = dbp * 64;

    for (int y = dsay; y < dsay + rrh; y++)
    {
        for (int x = dsax; x < dsax + rrw; x++)
        {
            int pageX = x / 64;
            int pageY = y / 64;
            int page = pageX + pageY * dbw;

            int px = x - (pageX * 64);
            int py = y - (pageY * 64);

            int blockX = px / 16;
            int blockY = py / 8;
            int block = block16S[blockX + blockY * 4];

            int bx = px - blockX * 16;
            int by = py - blockY * 8;

            int column = by / 2;

            int cx = bx;
            int cy = by - column * 2;
            int cw = columnWord16S[cx + cy * 16];
            int ch = columnHalf16S[cx + cy * 16];

            unsigned short* dst = (unsigned short*)&gsmem[startBlockPos + page * 2048 + block * 64 + column * 16 + cw];
            *src = dst[ch];
            src++;
        }
    }
}
int block8[32] =
{
     0,  1,  4,  5, 16, 17, 20, 21,
     2,  3,  6,  7, 18, 19, 22, 23,
     8,  9, 12, 13, 24, 25, 28, 29,
    10, 11, 14, 15, 26, 27, 30, 31
};

int columnWord8[2][64] =
{
    {
         0,  1,  4,  5,  8,  9, 12, 13,   0,  1,  4,  5,  8,  9, 12, 13,
         2,  3,  6,  7, 10, 11, 14, 15,   2,  3,  6,  7, 10, 11, 14, 15,

         8,  9, 12, 13,  0,  1,  4,  5,   8,  9, 12, 13,  0,  1,  4,  5,
        10, 11, 14, 15,  2,  3,  6,  7,  10, 11, 14, 15,  2,  3,  6,  7
    },
    {
         8,  9, 12, 13,  0,  1,  4,  5,   8,  9, 12, 13,  0,  1,  4,  5,
        10, 11, 14, 15,  2,  3,  6,  7,  10, 11, 14, 15,  2,  3,  6,  7,

         0,  1,  4,  5,  8,  9, 12, 13,   0,  1,  4,  5,  8,  9, 12, 13,
         2,  3,  6,  7, 10, 11, 14, 15,   2,  3,  6,  7, 10, 11, 14, 15
    }
};

int columnByte8[64] =
{
    0, 0, 0, 0, 0, 0, 0, 0,  2, 2, 2, 2, 2, 2, 2, 2,
    0, 0, 0, 0, 0, 0, 0, 0,  2, 2, 2, 2, 2, 2, 2, 2,

    1, 1, 1, 1, 1, 1, 1, 1,  3, 3, 3, 3, 3, 3, 3, 3,
    1, 1, 1, 1, 1, 1, 1, 1,  3, 3, 3, 3, 3, 3, 3, 3
};

void writeTexPSMT8(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void* data)
{
    dbw >>= 1;
    unsigned char* src = (unsigned char*)data;
    int startBlockPos = dbp * 64;

    for (int y = dsay; y < dsay + rrh; y++)
    {
        for (int x = dsax; x < dsax + rrw; x++)
        {
            int pageX = x / 128;
            int pageY = y / 64;
            int page = pageX + pageY * dbw;

            int px = x - (pageX * 128);
            int py = y - (pageY * 64);

            int blockX = px / 16;
            int blockY = py / 16;
            int block = block8[blockX + blockY * 8];

            int bx = px - (blockX * 16);
            int by = py - (blockY * 16);

            int column = by / 4;

            int cx = bx;
            int cy = by - column * 4;
            int cw = columnWord8[column & 1][cx + cy * 16];
            int cb = columnByte8[cx + cy * 16];

            unsigned char* dst = (unsigned char*)&gsmem[startBlockPos + page * 2048 + block * 64 + column * 16 + cw];
            dst[cb] = *src++;
        }
    }
}

void readTexPSMT8(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void* data)
{
    dbw >>= 1;
    unsigned char* src = (unsigned char*)data;
    int startBlockPos = dbp * 64;

    for (int y = dsay; y < dsay + rrh; y++)
    {
        for (int x = dsax; x < dsax + rrw; x++)
        {
            int pageX = x / 128;
            int pageY = y / 64;
            int page = pageX + pageY * dbw;

            int px = x - (pageX * 128);
            int py = y - (pageY * 64);

            int blockX = px / 16;
            int blockY = py / 16;
            int block = block8[blockX + blockY * 8];

            int bx = px - blockX * 16;
            int by = py - blockY * 16;

            int column = by / 4;

            int cx = bx;
            int cy = by - column * 4;
            int cw = columnWord8[column & 1][cx + cy * 16];
            int cb = columnByte8[cx + cy * 16];

            unsigned char* dst = (unsigned char*)&gsmem[startBlockPos + page * 2048 + block * 64 + column * 16 + cw];
            *src = dst[cb];
            src++;
        }
    }
}

int block4[32] =
{
     0,  2,  8, 10,
     1,  3,  9, 11,
     4,  6, 12, 14,
     5,  7, 13, 15,
    16, 18, 24, 26,
    17, 19, 25, 27,
    20, 22, 28, 30,
    21, 23, 29, 31
};

int columnWord4[2][128] =
{
    {
         0,  1,  4,  5,  8,  9, 12, 13,   0,  1,  4,  5,  8,  9, 12, 13,   0,  1,  4,  5,  8,  9, 12, 13,   0,  1,  4,  5,  8,  9, 12, 13,
         2,  3,  6,  7, 10, 11, 14, 15,   2,  3,  6,  7, 10, 11, 14, 15,   2,  3,  6,  7, 10, 11, 14, 15,   2,  3,  6,  7, 10, 11, 14, 15,

         8,  9, 12, 13,  0,  1,  4,  5,   8,  9, 12, 13,  0,  1,  4,  5,   8,  9, 12, 13,  0,  1,  4,  5,   8,  9, 12, 13,  0,  1,  4,  5,
        10, 11, 14, 15,  2,  3,  6,  7,  10, 11, 14, 15,  2,  3,  6,  7,  10, 11, 14, 15,  2,  3,  6,  7,  10, 11, 14, 15,  2,  3,  6,  7
    },
    {
         8,  9, 12, 13,  0,  1,  4,  5,   8,  9, 12, 13,  0,  1,  4,  5,   8,  9, 12, 13,  0,  1,  4,  5,   8,  9, 12, 13,  0,  1,  4,  5,
        10, 11, 14, 15,  2,  3,  6,  7,  10, 11, 14, 15,  2,  3,  6,  7,  10, 11, 14, 15,  2,  3,  6,  7,  10, 11, 14, 15,  2,  3,  6,  7,

         0,  1,  4,  5,  8,  9, 12, 13,   0,  1,  4,  5,  8,  9, 12, 13,   0,  1,  4,  5,  8,  9, 12, 13,   0,  1,  4,  5,  8,  9, 12, 13,
         2,  3,  6,  7, 10, 11, 14, 15,   2,  3,  6,  7, 10, 11, 14, 15,   2,  3,  6,  7, 10, 11, 14, 15,   2,  3,  6,  7, 10, 11, 14, 15
    }
};

int columnByte4[128] =
{
    0, 0, 0, 0, 0, 0, 0, 0,  2, 2, 2, 2, 2, 2, 2, 2,  4, 4, 4, 4, 4, 4, 4, 4,  6, 6, 6, 6, 6, 6, 6, 6,
    0, 0, 0, 0, 0, 0, 0, 0,  2, 2, 2, 2, 2, 2, 2, 2,  4, 4, 4, 4, 4, 4, 4, 4,  6, 6, 6, 6, 6, 6, 6, 6,

    1, 1, 1, 1, 1, 1, 1, 1,  3, 3, 3, 3, 3, 3, 3, 3,  5, 5, 5, 5, 5, 5, 5, 5,  7, 7, 7, 7, 7, 7, 7, 7,
    1, 1, 1, 1, 1, 1, 1, 1,  3, 3, 3, 3, 3, 3, 3, 3,  5, 5, 5, 5, 5, 5, 5, 5,  7, 7, 7, 7, 7, 7, 7, 7
};

void writeTexPSMT4(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void* data)
{
    dbw >>= 1;
    unsigned char* src = (unsigned char*)data;
    int startBlockPos = dbp * 64;

    bool odd = false;

    for (int y = dsay; y < dsay + rrh; y++)
    {
        for (int x = dsax; x < dsax + rrw; x++)
        {
            int pageX = x / 128;
            int pageY = y / 128;
            int page = pageX + pageY * dbw;

            int px = x - (pageX * 128);
            int py = y - (pageY * 128);

            int blockX = px / 32;
            int blockY = py / 16;
            int block = block4[blockX + blockY * 4];

            int bx = px - blockX * 32;
            int by = py - blockY * 16;

            int column = by / 4;

            int cx = bx;
            int cy = by - column * 4;
            int cw = columnWord4[column & 1][cx + cy * 32];
            int cb = columnByte4[cx + cy * 32];

            unsigned char* dst = (unsigned char*)&gsmem[startBlockPos + page * 2048 + block * 64 + column * 16 + cw];

            if (cb & 1)
            {
                if (odd)
                    dst[cb >> 1] = (dst[cb >> 1] & 0x0f) | ((*src) & 0xf0);
                else
                    dst[cb >> 1] = (dst[cb >> 1] & 0x0f) | (((*src) << 4) & 0xf0);
            } else
            {
                if (odd)
                    dst[cb >> 1] = (dst[cb >> 1] & 0xf0) | (((*src) >> 4) & 0x0f);
                else
                    dst[cb >> 1] = (dst[cb >> 1] & 0xf0) | ((*src) & 0x0f);
            }

            if (odd)
                src++;

            odd = !odd;
        }
    }
}

void readTexPSMT4(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void* data)
{
    dbw >>= 1;
    unsigned char* src = (unsigned char*)data;
    int startBlockPos = dbp * 64;

    bool odd = false;

    for (int y = dsay; y < dsay + rrh; y++)
    {
        for (int x = dsax; x < dsax + rrw; x++)
        {
            int pageX = x / 128;
            int pageY = y / 128;
            int page = pageX + pageY * dbw;

            int px = x - (pageX * 128);
            int py = y - (pageY * 128);

            int blockX = px / 32;
            int blockY = py / 16;
            int block = block4[blockX + blockY * 4];

            int bx = px - blockX * 32;
            int by = py - blockY * 16;

            int column = by / 4;

            int cx = bx;
            int cy = by - column * 4;
            int cw = columnWord4[column & 1][cx + cy * 32];
            int cb = columnByte4[cx + cy * 32];

            unsigned char* dst = (unsigned char*)&gsmem[startBlockPos + page * 2048 + block * 64 + column * 16 + cw];

            if (cb & 1)
            {
                if (odd)
                    *src = ((*src) & 0x0f) | (dst[cb >> 1] & 0xf0);
                else
                    *src = ((*src) & 0xf0) | ((dst[cb >> 1] >> 4) & 0x0f);
            } else
            {
                if (odd)
                    *src = ((*src) & 0x0f) | ((dst[cb >> 1] << 4) & 0xf0);
                else
                    *src = ((*src) & 0xf0) | (dst[cb >> 1] & 0x0f);
            }

            if (odd)
                src++;

            odd = !odd;
        }
    }
}
