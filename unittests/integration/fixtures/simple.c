enum {
  MAGENTA = 0,
  RED,
  GREEN,
};

int tick(int hover) {
	if (hover) {
		return RED;
	} else {
		return GREEN;
	}
}