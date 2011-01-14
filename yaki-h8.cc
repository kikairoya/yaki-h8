#include <vector>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <memory>
#include <sstream>
#include <iterator>
#include <cstring>
#include <iomanip>
#include <map>
#include <array>
#include <cmath>

#define GNUC_VER ((__GNUC__*10000L) + (__GNUC_MINOR__*100L) + (__GNUC_PATCHLEVEL__*1L))

#ifdef _MSC_VER
#if _MSC_VER >= 1600
// VC2010+ has nullptr
#define HAS_NATIVE_NULLPTR
#endif
#endif

#ifdef __GNUC__
#if (GNUC_VER >= 40600) && defined(__GXX_EXPERIMENTAL_CXX0X__)
// GCC 4.6+ with -std=c++0x has nullptr
#define HAS_NATIVE_NULLPTR
#endif
#endif


#ifndef HAS_NATIVE_NULLPTR
const class nullptr_t {
public:
	template<class T>
	operator T *() const { return 0; }
	template<class C, class T>
	operator T C::*() const { return 0; }
private:
	void operator &() const;
} nullptr = {};
#endif

template <typename charT, typename T, class Traits = std::char_traits<charT>, class Alloc = std::allocator<charT> >
inline std::basic_string<charT, Traits, Alloc> to_string(const T &val) {
	std::basic_stringstream<charT, Traits, Alloc> ss;
	ss << val;
	return ss.str();
}


struct s_record_line {
	enum record_type_t {
		header_record=0,
		data_16bit_addr=1,
		data_24bit_addr=2,
		data_32bit_addr=3,
		//invalid_record_4=4,
		data_left_count=5,
		//invalid_record_6=6,
		entry_32bit_addr=7,
		entry_24bit_addr=8,
		entry_16bit_addr=9
	} record_type;
	unsigned char byte_count;
	unsigned long load_addr;
	std::vector<unsigned char> data;
	unsigned char checksum;
};
class stream_is_not_srec: public std::runtime_error {
public:
	stream_is_not_srec(): std::runtime_error("stream is not SREC (not found 'S')") { }
};
class unknown_record_type: public std::runtime_error {
public:
	unknown_record_type(int c): std::runtime_error("unknown record type "+to_string<char>(c)) { }
};
template <typename charT>
unsigned long basic_strhextoul(charT *s);
template <>
unsigned long basic_strhextoul<char>(char *s) { return strtoul(s, 0, 16); }
template <>
unsigned long basic_strhextoul<wchar_t>(wchar_t *s) { return wcstoul(s, 0, 16); }
template <unsigned Bytes, typename retT, typename charT, class Traits>
retT get_hex_Nbyte(std::basic_istream<charT, Traits> &is) {
	charT r[Bytes*2+1];
	is.read(r, Bytes*2);
	r[Bytes*2] = 0;
	return static_cast<retT>(basic_strhextoul(r));
}
template <typename charT, class Traits>
std::basic_istream<charT, Traits> &operator >>(std::basic_istream<charT, Traits> &is, s_record_line &line) {
	std::ios_base::iostate err = std::ios_base::goodbit;
	try {
		typename std::basic_istream<charT, Traits>::sentry ipfx(is);
		if (ipfx) {
			s_record_line l;
			if (is.get()!=is.widen('S')) throw stream_is_not_srec();
			switch (int c = (is.get()-is.widen('0'))) {
			case 0: l.record_type = s_record_line::header_record; break;
			case 1: l.record_type = s_record_line::data_16bit_addr; break;
			case 2: l.record_type = s_record_line::data_24bit_addr; break;
			case 3: l.record_type = s_record_line::data_32bit_addr; break;
			case 4: throw unknown_record_type(c+is.widen('0'));
			case 5: l.record_type = s_record_line::data_left_count; break;
			case 6: throw unknown_record_type(c+is.widen('0'));
			case 7: l.record_type = s_record_line::entry_32bit_addr; break;
			case 8: l.record_type = s_record_line::entry_24bit_addr; break;
			case 9: l.record_type = s_record_line::entry_16bit_addr; break;
			default: throw unknown_record_type(c+is.widen('0'));
			}
			int bc = l.byte_count = get_hex_Nbyte<1, unsigned>(is);
			switch (l.record_type) {
			case s_record_line::header_record:
			case s_record_line::data_16bit_addr:
			case s_record_line::data_left_count:
			case s_record_line::entry_16bit_addr:
				bc -= 2;
				l.load_addr = get_hex_Nbyte<2, unsigned long>(is);
				break;
			case s_record_line::data_24bit_addr:
			case s_record_line::entry_24bit_addr:
				bc -= 3;
				l.load_addr = get_hex_Nbyte<3, unsigned long>(is);
				break;
			case s_record_line::data_32bit_addr:
			case s_record_line::entry_32bit_addr:
				bc -= 4;
				l.load_addr = get_hex_Nbyte<4, unsigned long>(is);
				break;
			}
			for (int n=0; n<bc-1; ++n) l.data.push_back(get_hex_Nbyte<1, unsigned>(is));
			l.checksum = get_hex_Nbyte<1, unsigned>(is);
			line = l;
		}
	} catch (...) {
		bool f = false;
		try { is.setstate(std::ios_base::failbit); }
		catch (std::ios_base::failure) { f = true; }
		if (f) throw;
	}
	if (err!=std::ios_base::goodbit) is.setstate(err);
	return is;
}

#include "serial.hpp"

using namespace std;

void help() {
	cerr << "usage: yaki-h8 [opts...] {file}\n";
	cerr << "\n";
	cerr << "  file:        S-record file to transfer\n";
	cerr << " options:\n";
	cerr << "  -p PORT      specify serial port\n";
	cerr << "  -s BAUDRATE  specify initial hand-shake speed\n";
	cerr << "  -c CLOCK     specify target system clock freq\n";
	cerr << "  -t BAUDRATE  specify data transfer speed\n";
	cerr << "  -h           print this message\n";
}
string get_option_arg(int &argc, char **&argv) {
	if (strlen(*argv)==2) {
		if (--argc) return string(*++argv);
		++argv;
		return string();
	}
	return string((*argv)+2);
}
int check_error(unsigned char c, unsigned char expect) {
	if (c!=expect) {
		stringstream ss;
		ss << "invalid response '0x";
		ss << hex << setw(2) << setprecision(2) << setfill('0') << uppercase << (c&0xFF);
		ss << "' (should be ";
		ss << hex << setw(2) << setprecision(2) << setfill('0') << uppercase << (expect&0xFF);
		ss << ")\n";
		cerr << ss.str();
		return 1;
	}
	return 0;
}

vector<unsigned char> load_loader(const string &loader_file) {
	ifstream f(("stub-"+loader_file+".bin").c_str(), ios::binary);
	if (!f.is_open()) throw runtime_error("can't open stub file <" + loader_file + ">");
	f.seekg(0, ios::end);
	const auto len = f.tellg();
	vector<unsigned char> r(len, 0);
	f.seekg(0, ios::beg);
	f.read(reinterpret_cast<char *>(&r[0]), len);
	cout << " stub-" << loader_file << ".bin(" << r.size() << "B)" << flush;
	return r;
}

uint32_t load_srec(const string &name, vector<s_record_line> &srec) {
	ifstream ifs(name.c_str());
	if (!ifs.is_open()) throw runtime_error("can't open rom file <" + name + ">");
	uint32_t last = 0;
	while (ifs) {
		s_record_line line;
		ifs >> line;
		last = max<uint32_t>(last, line.load_addr+line.data.size());
		srec.push_back(move(line));
	}
	last += 0xFF;
	last &=~0xFF;
	return last;
}

struct copy_srec_line {
	copy_srec_line(vector<unsigned char> &romdata): romdata(romdata) { }
	void operator ()(const s_record_line &line) {
		switch (line.record_type) {
		case s_record_line::header_record:
			cout << "file: " << string(line.data.cbegin(), line.data.cend()) << " (" << romdata.size()/1024 << "KiB)"<< endl;
			break;
		case s_record_line::data_16bit_addr:
		case s_record_line::data_24bit_addr:
		case s_record_line::data_32bit_addr:
			copy(line.data.cbegin(), line.data.cend(), romdata.begin()+line.load_addr);
			break;
		default:
			break;
		}
	}
	vector<unsigned char> &romdata;
};

vector<unsigned char> load_romimage(const string &mot_name) {
	vector<s_record_line> srec;
	const auto last_addr = load_srec(mot_name, srec);
	vector<unsigned char> romdata(last_addr, 0xFF);
	for_each(srec.cbegin(), srec.cend(), copy_srec_line(romdata));
	return romdata;
}

int main(int argc, char **argv) try {
	string mot_name;
	string port_name("COM1");
	string loader_file;
	int init_speed = 9600;
	int alt_speed = 0;
	double target_clock = 20.0;
	while (--argc) {
		string arg(*++argv);
		if (arg.empty()) continue;
		if (arg[0]=='-') {
			if (arg.length()==1) {
				if (--argc) mot_name = *++argv;
				else ++argc;
				continue;
			} else {
				switch (arg[1]) {
				case 'h': help(); continue;
				case 'p': port_name = get_option_arg(argc, argv); continue;
				case 's': init_speed = strtoul(get_option_arg(argc, argv).c_str(), nullptr, 10); continue;
				case 'c': target_clock = strtod(get_option_arg(argc, argv).c_str(), nullptr); continue;
				case 't': alt_speed = strtoul(get_option_arg(argc, argv).c_str(), nullptr, 10); continue;
				case 'l': loader_file = get_option_arg(argc, argv); continue;
				}
			}
			cerr << "unknown option: '" << arg << "'\n";
			continue;
		}
		if (mot_name.empty()) mot_name = move(arg);
		else cerr << "multiple target files. ignored.\n";
	}
	if (mot_name.empty()) {
		cerr << "target file must be set\n";
		return 1;
	}


	serial ser(port_name, init_speed);
	const vector<unsigned char> romdata(load_romimage(mot_name.c_str()));
	{
		cout << "synchronizing" << flush;
		const array<unsigned char, 256> zeroes = {{}};
		unsigned n;
		const unsigned rep = static_cast<unsigned>(-1);
		for (n=0; n<rep; ++n) {
			cout << '.' << flush;
			ser.write(zeroes.data(), sizeof(zeroes));
			if (ser.readable()) break;
		}
		if (n == rep) {
			cerr << "not responce from target.\n";
			return 2;
		}
		if (const auto c = ser.read_value<char>()) {
			cerr << "invalid response '0x" << hex << setw(2) << setprecision(2) << setfill('0') << uppercase << (unsigned)c << "' (should be 0x00)\n";
			return 3;
		}
		cout << " done." << endl;
	} {
		cout << "erasing..." << flush;
		ser.write_value<char>(0x55);
		if (const int r = check_error(ser.read_value<char>(), 0xAA)) return r;
		cout << " done." << endl;
	} {
		cout << "sending stub file" << flush;
		const vector<unsigned char> stub(load_loader(loader_file));
		ser.write_value(ser.hton(static_cast<uint16_t>(stub.size())));
		cout << '.' << flush;
		ser.read_value<uint16_t>();
		unsigned n = 0;
		for_each(stub.cbegin(), stub.cend(), [&n, &ser](const unsigned char c) {
			if (++n%16==0) cout << '.' << flush;
			ser.write_value(c);
			ser.read_value<unsigned char>();
		});
		if (const int r = check_error(ser.read_value<char>(), 0xAA)) return r;
		cout << "done." << endl;
	} {
		cout << "writing" << flush;
		if (const int r = check_error(ser.read_value<char>(), 'B')) return r;
		cout << '.' << flush;

		const int baud = alt_speed ? alt_speed : init_speed;
		ser.write_value(static_cast<unsigned char>(round(target_clock / 32 / baud * 1000000 - 1)));
		ser.set_speed(baud);
		if (const int r = check_error(ser.read_value<char>(), 'S')) return r;
		cout << '.' << flush;
		
		const uint32_t m = romdata.size()>>8;
		ser.write_value(ser.hton(m));
		for (uint32_t n=0; n<m; ++n) {
			if (int r = check_error(ser.read_value<char>(), 'A')) return r;
			cout << '.' << flush;
			ser.write(&romdata[n*256], 128);
			if (int r = check_error(ser.read_value<char>(), 'A')) return r;
			ser.write(&romdata[n*256+128], 128);
		}
		if (int r = check_error(ser.read_value<char>(), 'F')) return r;
		cout << "done." << endl;
	}
	cout << "finish." << endl;
	return 0;
} catch (const exception &e) {
	cerr << "fatal: " << e.what() << endl;
	return 1;
} catch (...) {
	cerr << "fatal: unknown error" << endl;
	return 1;
}

