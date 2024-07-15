format:
	find . -type f -name "*.cpp" -o -name "*.h" | xargs clang-format -i