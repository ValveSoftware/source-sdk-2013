/* This will only be available for GCC with glibc for the foreseeable future */

 __attribute__((target_clones("default,avx2"))) int f(int x)
{
    return x + 3;
}

int main(int argc, char **argv)
{
    int y = f(39);
    return 0;
}