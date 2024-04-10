int a_func();
int b_func();

int cond(int n) {
    if (n > 5) {
        return a_func();
    }
    return b_func();
}
