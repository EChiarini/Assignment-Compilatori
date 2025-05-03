int verificoLICM(int a, int b) {
    int n=5,licm,licm2,li,d;

    for (int i=0;i<n;i++) {
        licm = a + b;
        for (int j=0;j<n;j++) {
            d = i + j;
            licm2 = b + a;
            li = a + i;
            if (j > 3) { int e = li + 2; break; }
        }
    }

    return n;
}
