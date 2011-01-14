#include <stdio.h>
#include <map>
#include <stdexcept>
#include <windows.h>

std::map<serial *, int> serial_peek_buffer;
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
	if (!SetCommState(data, &dcb)) throw std::runtime_error("cannot reset speed");
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
