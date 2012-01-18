#include <vector>
#include <boost/asio.hpp>
#include "srec-parser.hpp"

namespace asio = boost::asio;


#include <string>
#include <fstream>
#include <iterator>
#include <vector>
#include <deque>
#include <boost/exception/all.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

using namespace std;
using boost::system::error_code;
using boost::format;

inline string get_option_arg(int &argc, char **&argv) {
	if (strlen(*argv)==2) {
		if (--argc) return string(*++argv);
		++argv;
		return string();
	}
	return string((*argv)+2);
}

inline void require(unsigned char val, unsigned char ex) {
	if (val == ex) return;
	cerr << (format("invalid response '0x%02X'(should be 0x%02X\n") % (unsigned)val % (unsigned)ex);
	throw runtime_error("unexpected respoonse");
}

void help() {
	cerr << "usage: yaki-h8 [opts...] {file}\n";
	cerr << "\n";
	cerr << "  file:        S-record file to transfer\n";
	cerr << " options:\n";
	cerr << "  -p PORT      specify serial port\n";
	cerr << "  -s BAUDRATE  specify initial hand-shake speed\n";
	cerr << "  -c CLOCK     specify target system clock freq\n";
	cerr << "  -t BAUDRATE  specify data transfer speed\n";
	cerr << "  -l TYPE      specify target type\n";
	cerr << "  -h           print this message\n";
}

#include "stub-h8tiny.dat"

inline vector<unsigned char> load_loader(const string &loader_file) {
	if (loader_file == "tiny") return vector<unsigned char>(stub_h8tiny, stub_h8tiny+sizeof(stub_h8tiny));
	throw runtime_error("cant find stub <" + loader_file + ">");
}

inline std::vector<unsigned char> read_file(const string &file) {
	std::ifstream f(file);
	f.seekg(0, std::ios_base::end);
	std::vector<unsigned char> r(f.tellg());
	f.seekg(0, std::ios_base::beg);
	f.read((char *)&r[0], r.size());
	return r;
}

int main(int argc, char **argv) {
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
				case 's': init_speed = boost::lexical_cast<int>(get_option_arg(argc, argv)); continue;
				case 'c': target_clock = boost::lexical_cast<double>(get_option_arg(argc, argv)); continue;
				case 't': alt_speed = boost::lexical_cast<int>(get_option_arg(argc, argv)); continue;
				case 'l': loader_file = get_option_arg(argc, argv); continue;
				}
			}
			cerr << "unknown option: '" << arg << "'\n";
			continue;
		}
		if (mot_name.empty()) mot_name = std::move(arg);
		else cerr << "multiple target files. ignored.\n";
	}
	if (mot_name.empty()) {
		cerr << "target file must be set\n";
		return 1;
	}

	asio::io_service serv;
	asio::serial_port ser(serv, port_name);
	ser.set_option(asio::serial_port::baud_rate(init_speed));
	ser.set_option(asio::serial_port::flow_control());
	ser.set_option(asio::serial_port::parity());
	ser.set_option(asio::serial_port::stop_bits());
	ser.set_option(asio::serial_port::character_size());

	const auto sendb = [&ser](unsigned char b) {
		asio::write(ser, asio::buffer(&b, 1));
	};
	const auto sendw = [&ser](uint16_t w) {
		const unsigned char a[] = { (unsigned char)(w>>8), (unsigned char)w };
		asio::write(ser, asio::buffer(a));
	};
//	const auto sendl = [&ser](uint32_t l) {
//		const unsigned char a[] = { (unsigned char)(l>>24), (unsigned char)(l>>16), (unsigned char)(l>>8), (unsigned char)(l) };
//		asio::write(ser, asio::buffer(a));
//	};
	const auto recvb = [&ser]() -> unsigned char {
		unsigned char c;
		asio::read(ser, asio::buffer(&c, 1));
		return c;
	};

	auto romdata(parse_srec<deque<unsigned char>>(read_file(mot_name)));
	romdata.resize(((romdata.size()-1)|0x000000FF)+1, 0xFF);

	{
		cout << "synchronizing" << flush;
		unsigned char c;
		const array<unsigned char, 256> zeroes = {{}};
		bool synced = false;
		asio::async_read(ser, asio::buffer(&c, 1), [&synced](error_code ec, size_t) { synced = true; });
		for (unsigned n = 0; n < static_cast<unsigned>(-1) && !synced; ++n) {
			cout << '.' << flush;
			asio::async_write(ser, asio::buffer(zeroes), [](error_code ec, size_t) { });
			serv.run_one();
		}
		ser.cancel();
		if (!synced) {
			cerr << "no responce from target.\n";
			return 2;
		}
		require(c, 0x00);
		cout << " done." << endl;
	} {
		cout << "erasing..." << flush;
		sendb(0x55);
		unsigned char c;
		while ((c=recvb()) == 0x00);
		require(c, 0xAA);
		cout << " done." << endl;
	} {
		const vector<unsigned char> stub(load_loader(loader_file));
		cout << "sending stub (" << stub.size() << "B) " << flush;
		sendw(stub.size()&0xFFFF);
		cout << '.' << flush;
		recvb(); recvb();
		for (unsigned n=0; n<stub.size(); ++n) {
			if (n%16==0) cout << '.' << flush;
			sendb(stub[n]);
			recvb();
		}
		require(recvb(), 0xAA);
		cout << "done." << endl;
	} {
		cout << "writing " << mot_name << " (" << (romdata.size()/1024) << "KiB) "<< flush;
		require(recvb(), 'B');
		cout << '.' << flush;
		const int baud = alt_speed ? alt_speed : init_speed;
		const unsigned char spdchr = static_cast<unsigned char>(round(target_clock / 32 / baud * 1000000 - 1));
		sendb(spdchr);
		require(recvb(), spdchr);
		ser.set_option(asio::serial_port::baud_rate(baud));
		while (recvb() != 'S');
		do {
			sendb('s');
		} while (recvb() != 'T') ;
		sendb('t');
		cout << '.' << flush;
		const uint16_t m = static_cast<uint16_t>(romdata.size()>>8);
		sendw(m);
		for (uint32_t n = 0; n < m; ++n) {
			require(recvb(), 'A');
			cout << '.' << flush;
			asio::write(ser, asio::buffer(&romdata[n*256], 128));
			require(recvb(), 'A');
			asio::write(ser, asio::buffer(&romdata[n*256+128], 128));
		}
		require(recvb(), 'F');
		cout << "done." << endl;
	}
	cout << "finish." << endl;
	return 0;
}
