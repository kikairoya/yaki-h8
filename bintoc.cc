#include <fstream>
#include <string>
#include <iomanip>

int main(int argc, char **argv) {
	if (argc < 4) return 1;
	const std::string inname = argv[1];
	const std::string outname = argv[2];
	const std::string symname = argv[3];
	std::ifstream fi(inname.c_str(), std::ios::binary);
	std::ofstream fo(outname.c_str());
	fo << "static const unsigned char " << symname << "[] = {\n";
	char c[16];
	while (fi.get(c, 16)) {
		fo << '\t';
		for (int n=0; n<16; ++n) fo << "0x" << std::hex << std::setfill('0') << std::setw(2) << (c[n]&0xFF) << ", ";
		fo << '\n';
	}
	fo << "};\n";
	return 0;
}
