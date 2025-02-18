#include <spng.h>
#include <iostream>

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    unsigned char buf[30] = {0};
    spng_ctx *ctx = spng_ctx_new(0);

    spng_set_png_buffer(ctx, buf, 30);

    struct spng_plte plte;
    int e = spng_get_plte(ctx, &plte);
    std::cout << spng_strerror(e);

    spng_ctx_free(ctx);

    return 0;
}
