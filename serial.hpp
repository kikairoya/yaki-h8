#include <string>
#include <exception>
#include <stdexcept>
#include <algorithm>

class serial {
public:
	serial(const std::string &name, int speed = -1);
	~serial();
	void set_speed(int speed);
	bool readable();
	serial &read(void *buf, std::streamsize n);
	template <typename T>
	T read_value() {
		T v;
		read_value(v);
		return v;
	}
	template <typename T>
	serial &read_value(T &v) { return read(&v, sizeof(T)); }
	serial &write(const void *buf, std::streamsize n);
	template <typename T>
	serial &write_value(const T &v) {
		return write(&v, sizeof(T));
	}
	template <typename T>
	static T hton(volatile T v) {
		std::reverse((unsigned char *)(&v), (unsigned char *)(&v+1));
		return v;
	}
	template <typename T>
	static T ntoh(volatile T v) {
		std::reverse((unsigned char *)(&v), (unsigned char *)(&v+1));
		return v;
	}
private:
	serial(const serial &);
	serial &operator =(const serial &);
	void *data;
};
