int tick(int n) {
  int i = 0;
  int j = 1;

  for (int c = 0; c < n; c++) {
    int tmp = i;
    i = j;
    j = i + tmp;
  }

  return i;
}