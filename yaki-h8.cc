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
#include <cmath>

#ifdef _MSC_VER
#if _MSC_VER >= 1600
// VC2010+ has nullptr
#define HAS_NATIVE_NULLPTR
#endif
#endif

#ifdef __GNUC__
#if (__GNUC__ >= 4) && (__GNUC_MINOR__>=6) && defined(__GXX_EXPERIMENTAL_CXX0X__)
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

class serial {
public:
	serial(const std::string &name, int speed = -1);
	~serial();
	void set_speed(int speed);
	bool readable();
	serial &read(void *buf, std::streamsize n);
	serial &write(const void *buf, std::streamsize n);
private:
	serial(const serial &);
	serial &operator =(const serial &);
	void *data;
};
void msleep(unsigned long msec);

#if defined(_WIN32)
#define IOS_BINMODE std::ios_base::binary
#include <windows.h>
std::map<serial *, int> serial_peek_buffer;
void msleep(unsigned long msec) { Sleep(msec); }
serial::serial(const std::string &name, int speed) {
	data = CreateFile(name.c_str(), GENERIC_READ|GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
	if (!data || data==INVALID_HANDLE_VALUE) throw std::runtime_error("cannot open port '" + name + "'");
	set_speed(speed);
	serial_peek_buffer[this] = EOF;
}
serial::~serial() {
	serial_peek_buffer.erase(this);
	CloseHandle(data);
}
void serial::set_speed(int speed) {
	if (speed<0) speed = 9600;
	DCB dcb;
	dcb.DCBlength = sizeof(dcb);
	dcb.BaudRate = speed;
	dcb.fBinary = TRUE;
	dcb.fParity = FALSE;
	dcb.fOutxCtsFlow = FALSE;
	dcb.fOutxDsrFlow = FALSE;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	dcb.fDsrSensitivity = FALSE;
	dcb.fTXContinueOnXoff = TRUE;
	dcb.fOutX = FALSE;
	dcb.fInX = FALSE;
	dcb.fErrorChar = FALSE;
	dcb.fNull = FALSE;
	dcb.fRtsControl = RTS_CONTROL_DISABLE;
	dcb.fAbortOnError = FALSE;
	dcb.wReserved = 0;
	dcb.XonLim = 0;
	dcb.XoffLim = 0;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;
	SetCommState(data, &dcb);
}
bool serial::readable() {
	if (serial_peek_buffer[this]!=EOF) return true;
	COMMTIMEOUTS ts;
	GetCommTimeouts(data, &ts);
	COMMTIMEOUTS to = { MAXDWORD, 0, 0, 0, 0 };
	SetCommTimeouts(data, &to);
	DWORD dw=0;
	char c;
	if (ReadFile(data, &c, 1, &dw, 0) && dw) serial_peek_buffer[this] = c;
	SetCommTimeouts(data, &ts);
	return serial_peek_buffer[this]!=EOF;
}
serial &serial::read(void *buf, std::streamsize n) {
	DWORD dw;
	unsigned char *p = static_cast<unsigned char *>(buf);
	{
		int &c = serial_peek_buffer[this];
		if (c!=EOF) {
			*p++ = c;
			--n;
			c = EOF;
		}
	}
	while (n) {
		ReadFile(data, p, n, &dw, 0);
		p += dw;
		n -= dw;
	}
	return *this;
}
serial &serial::write(const void *buf, std::streamsize n) {
	DWORD dw;
	const unsigned char *p = static_cast<const unsigned char *>(buf);
	while (n) {
		WriteFile(data, p, n, &dw, 0);
		p += dw;
		n -= dw;
	}
	return *this;
}
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <poll.h>
#include <termios.h>
#define IOS_BINMODE 0
void msleep(unsigned long msec) {
	timespec ts = { 0, msec*1000*1000 };
	nanosleep(&ts, 0);
}
serial::serial(const std::string &name, int speed) {
	const int fd = open(name.c_str(), O_RDWR);
	data = reinterpret_cast<void *>(fd);
	if (fd==-1) throw std::runtime_error("cannot open port '" + name + "'");
	set_speed(speed);
	struct termios st;
	tcgetattr(fd, &st);
	cfmakeraw(&st);
	tcsetattr(fd, TCSAFLUSH, &st);
}
serial::~serial() {
	close(static_cast<int>(reinterpret_cast<intptr_t>(data)));
}
void serial::set_speed(int speed) {
#define CASE_(x) case x: speed = B##x; break;
	switch (speed) {
		CASE_(0);
		CASE_(50);
		CASE_(75);
		CASE_(110);
		CASE_(134);
		CASE_(150);
		CASE_(200);
		CASE_(300);
		CASE_(600);
		CASE_(1200);
		CASE_(2400);
		CASE_(4800);
		CASE_(9600);
		CASE_(19200);
		CASE_(38400);
		CASE_(57600);
		CASE_(115200);
		CASE_(230400);
	default:
		return;
	}
#undef CASE_
	struct termios st;
	const int fd = static_cast<int>(reinterpret_cast<intptr_t>(data));
	tcgetattr(fd, &st);
	cfsetispeed(&st, speed);
	cfsetospeed(&st, speed);
	tcsetattr(fd, TCSAFLUSH, &st);
}
bool serial::readable() {
	pollfd pf = { static_cast<int>(reinterpret_cast<intptr_t>(data)), POLLIN, 0 };
	poll(&pf, 1, 0);
	return !!(pf.revents&POLLIN);
}
serial &serial::read(void *buf, std::streamsize n) {
	unsigned char *p = static_cast<unsigned char *>(buf);
	const int fd = static_cast<int>(reinterpret_cast<intptr_t>(data));
	while (n) {
		int r = ::read(fd, p, n);
		p += r;
		n -= r;
	}
	return *this;
}
serial &serial::write(const void *buf, std::streamsize n) {
	const unsigned char *p = static_cast<const unsigned char *>(buf);
	const int fd = static_cast<int>(reinterpret_cast<intptr_t>(data));
	while (n) {
		int r = ::write(fd, p, n);
		p += r;
		n -= r;
	}
	return *this;
}

#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) || defined(__CYGWIN__)
#define TINY_STUB_BEGIN binary_stub_tiny_bin_start
#define TINY_STUB_END binary_stub_tiny_bin_end
#else
#define TINY_STUB_BEGIN _binary_stub_tiny_bin_start
#define TINY_STUB_END _binary_stub_tiny_bin_end
#endif

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
int main(int argc, char **argv) try {
	string mot_name, port_name("COM1");
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
	ifstream mot_file(mot_name.c_str());
	if (!mot_file.is_open()) {
		cerr << "target file cannot open '" << mot_name << "'\n";
		return 1;
	}
	
	serial ser(port_name, init_speed);
	{
		cout << "synchronizing" << flush;
		const vector<unsigned char> zeroes(256, 0);
		unsigned n;
		const unsigned rep = static_cast<unsigned>(-1);
		for (n=0; n<rep; ++n) {
			if (n%16==0) cout << '.' << flush;
			ser.write(zeroes.data(), sizeof(zeroes));
			if (ser.readable()) break;
		}
		unsigned char c = 0x55;
		ser.read(&c, 1);
		if (c!=0) {
			cerr << "invalid response '0x" << hex << setw(2) << setprecision(2) << setfill('0') << uppercase << (unsigned)c << "' (should be 0x00)\n";
			return 3;
		}
		cout << " done." << endl;
	} {
		cout << "erasing..." << flush;
		ser.write("\x55", 1);
		char c = 0x55;
		ser.read(&c, 1);
		if (int r = check_error(c, 0xAA)) return r;
		cout << " done." << endl;
	} {
		extern unsigned char TINY_STUB_BEGIN[];
		extern unsigned char TINY_STUB_END[];
		cout << "sending stub file" << flush;
		const vector<unsigned char> stub(TINY_STUB_BEGIN, TINY_STUB_END);
		const unsigned len = stub.size();
		unsigned char lb[2] = { (unsigned char)(len>>8), (unsigned char)(len) };
		ser.write(lb, 2);
		cout << '.' << flush;
		ser.read(lb, 2);
		int n = 0;
		const auto end = stub.cend();
		for (auto ite = stub.cbegin(); ite!=end; ++ite) {
			unsigned char c = *ite;
			if (++n%16==0) cout << '.' << flush;
			ser.write(&c, 1);
			ser.read(&c, 1);
		};
		char c = 0x55;
		ser.read(&c, 1);
		if (int r = check_error(c, 0xAA)) return r;
		cout << "done." << endl;
	}
	
	vector<s_record_line> srec;
	size_t last_addr = 0;
	{
		while (mot_file) {
			s_record_line line;
			mot_file >> line;
			last_addr = max<size_t>(last_addr, line.load_addr+line.data.size());
			srec.push_back(move(line));
		}
	}
	last_addr = (last_addr+0xFF)&~0xFF;
	vector<unsigned char> romdata(last_addr, 0xFF);
	{
		const auto ptr = romdata.begin();
		auto ite = srec.cbegin();
		const auto end = srec.cend();
		for (; ite!=end; ++ite) {
			switch (ite->record_type) {
			case s_record_line::header_record:
				cout << "file: " << string(ite->data.cbegin(), ite->data.cend()) << " (" << romdata.size()/1024 << "KiB)"<< endl;
				continue;
			case s_record_line::data_16bit_addr:
			case s_record_line::data_24bit_addr:
			case s_record_line::data_32bit_addr:
				break;
			default:
				continue;
			}
			copy(ite->data.cbegin(), ite->data.cend(), ptr+ite->load_addr);
		}
	} {
		cout << "writing" << flush;
		unsigned char c = 0xFF;
		ser.read(&c, 1);
		if (int r = check_error(c, 'B')) return r;
		
		const int baud = alt_speed ? alt_speed : init_speed;
		c = static_cast<char>(round(target_clock / 32 / baud * 1000000 - 1));
		ser.write(&c, 1);
		ser.set_speed(baud);
		ser.read(&c, 1);
		if (int r = check_error(c, 'S')) return r;
		
		const unsigned m = romdata.size()/256;
		c = m&0xFF;
		ser.write(&c, 1);
		for (unsigned n=0; n<m; ++n) {
			cout << '.' << flush;
			ser.read(&c, 1);
			if (int r = check_error(c, 'A')) return r;
			ser.write(&romdata[n*256], 256);
		}
		ser.read(&c, 1);
		if (int r = check_error(c, 'F')) return r;
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

