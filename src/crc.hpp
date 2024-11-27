#ifndef CRC_H
#define CRC_H

#include <QByteArray>

class Crc {
public:
	Crc();
	static QByteArray crc(uint32_t padlen, const QByteArray &data);

private:
	static uint32_t _crc32(const QByteArray &bytes, uint32_t initial_state);
};

#endif // CRC_H
