#include <iostream>
#include <stdlib.h>
#include <string>

using namespace std;

int main() {
	
	string str;
	long int converted = 0;

	cout << "Input a string of binary data: ";
	cin >> str;

	cout << "\n\nString: " << str;

	converted = strtol(str.c_str(),NULL,2);

	cout << "\n\nBinary number: " << converted << endl;

	system("pause");
	return 0;
	}