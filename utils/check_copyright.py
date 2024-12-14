#!/usr/bin/python

from debian import copyright

# Script that validates the copyright file.


def check_copyright() -> bool:
	with open('copyright', 'r', encoding='utf-8') as f:
		try:
			copyright.Copyright(f, strict=True)
		except copyright.Error as e:
			print(e)
			return False
		return True


if __name__ == '__main__':
	if not check_copyright():
		exit(1)
