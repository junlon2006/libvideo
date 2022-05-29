
#define SWAP_Y(uyvy)  (((uyvy) & 0xFF00FF) + (((uyvy) >> 16) & 0xFF00) + (((uyvy) << 16) & 0xFF000000))

void grb565_mirror(unsigned char *src, int width, int height)
{
    int h;
    unsigned int *x, *y, tmp;
    for (h = 0; h < height; h++) {
        x = (unsigned int *)(src + (h * width * 2));
        y = x + ((width >> 1) - 1);
        while (x < y) {
            tmp = SWAP_Y(*x);
            *x = SWAP_Y(*y);
            *y = tmp;
            x++;
            y--;
        }
    }
}