#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <poll.h>
#include <termios.h>
#include <time.h>

void msleep(unsigned msec) {
	timespec t = { 0, (long)msec * 1000000 };
	nanosleep(&t, 0);
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
		throw std::runtime_error("invalid speed");
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

