
namespace evt
{

namespace
{

// 查表法
int y_table[256];
int v_r_table[256];
int v_g_table[256];
int u_g_table[256];
int u_b_table[256];

unsigned int limit_table[256 * 3];


void initColorTables();

bool onLoad = []()
{
    initColorTables();
    return true;
}();

inline unsigned int limitToUnsigned8Bits(int value)
{
    if (value > 255)
        return 255;
    else if (value < 0)
        return 0;
    else
        return value;
}

void initColorTables()
{
    for (int i = 0; i <= 255; i++)
    {
        y_table[i] = (i - 16) * 1192;
        v_r_table[i] = (i - 128) * 1634;
        v_g_table[i] = (i - 128) * 833;
        u_g_table[i] = (i - 128) * 401;
        u_b_table[i] = (i - 128) * 2065;
    }
    for (int i = 0; i < 256 * 3; i++)
    {
        limit_table[i] = limitToUnsigned8Bits(i - 256);
    }
}

} // namespace


void yuyv2bgr(unsigned char* bufferIn, unsigned char* bufferOut, int width, int height)
{
    unsigned char y1, u, y2, v;
    int size = width * height;
    int y1192;
    for (int pixel = 0; pixel < size ; pixel += 2)
    {
        y1 = *bufferIn++;
        u = *bufferIn++;
        y2 = *bufferIn++;
        v = *bufferIn++;

        y1192 = y_table[y1];

        *bufferOut++ = limit_table[((y1192 + u_b_table[u]) >> 10) + 256];
        *bufferOut++ = limit_table[((y1192 - v_g_table[v] - u_g_table[u]) >> 10) + 256];
        *bufferOut++ = limit_table[((y1192 + v_r_table[v]) >> 10) + 256]; //可能小于0，先取正

        y1192 = y_table[y2];
        *bufferOut++ = limit_table[((y1192 + u_b_table[u]) >> 10) + 256];
        *bufferOut++ = limit_table[((y1192 - v_g_table[v] - u_g_table[u]) >> 10) + 256];
        *bufferOut++ = limit_table[((y1192 + v_r_table[v]) >> 10) + 256]; //可能小于0，先取正

    }
}

} // namespace evt
