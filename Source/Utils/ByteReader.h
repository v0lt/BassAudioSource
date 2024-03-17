// Copyright (c) 2022-2024 v0lt
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <cassert>

class ByteReader
{
	const uint8_t* m_start = nullptr;
	const uint8_t* m_end = nullptr;
	const uint8_t* m_pos = nullptr;

	bool m_error = false;

	template<typename T>
	T Read()
	{
		if (m_pos + sizeof(T) <= m_end) {
			T value = *(T*)m_pos;
			m_pos += sizeof(T);

			return value;
		}
		else {
			m_pos = m_end;
			m_error = true;

			return 0;
		}
	}

	template<typename T>
	T Look()
	{
		if (m_pos + sizeof(T) <= m_end) {
			T value = *(T*)m_pos;

			return value;
		}
		else {
			m_error = true;

			return 0;
		}
	}

public:
	ByteReader(const uint8_t* data)
		: m_start(data)
		, m_pos(data)
		, m_end(data)
	{
		assert(data);
	}

	void SetSize(const size_t size)
	{
		m_end = m_start + size;
	}

	size_t GetSize()
	{
		return m_end - m_start;
	}

	size_t GetPos()
	{
		return m_pos - m_start;
	}

	const uint8_t* GetPtr()
	{
		return m_pos;
	}

	size_t GetRemainder()
	{
		return m_end - m_pos;
	}

	bool Skip(size_t size)
	{
		m_pos += size;

		if (m_pos <= m_end) {
			return true;
		}

		m_pos = m_end;
		m_error = true;

		return false;
	}

	bool Seek(size_t pos)
	{
		m_pos = m_start + pos;

		if (m_pos <= m_end) {
			return true;
		}

		m_pos = m_end;
		m_error = true;

		return false;
	}

	bool GetError()
	{
		return m_error;
	}

	uint8_t ReadByte()
	{
		if (m_pos < m_end) {
			return *m_pos++;
		}
		else {
			m_error = true;
			return 0;
		}
	}

	uint8_t LookByte()
	{
		if (m_pos < m_end) {
			return *m_pos;
		}
		else {
			m_error = true;
			return 0;
		}
	}

	uint16_t Read16Le() { return Read<uint16_t>(); }
	uint32_t Read32Le() { return Read<uint32_t>(); }
	uint64_t Read64Le() { return Read<uint64_t>(); }

	uint16_t Read16Be() { return _byteswap_ushort(Read<uint16_t>()); }
	uint32_t Read32Be() { return _byteswap_ulong(Read<uint32_t>()); }
	uint64_t Read64Be() { return _byteswap_uint64(Read<uint64_t>()); }

	uint32_t Read24Le() {
		if (m_pos + 3 <= m_end) {
			uint32_t value =
				uint32_t(m_pos[0]) |
				(uint32_t(m_pos[0]) << 8) |
				(uint32_t(m_pos[0]) >> 16);
			m_pos += 3;

			return value;
		}
		else {
			m_pos = m_end;
			m_error = true;

			return 0;
		}
	}

	uint32_t Read24Be() {
		if (m_pos + 3 <= m_end) {
			uint32_t value =
				(uint32_t(m_pos[0]) << 16) |
				(uint32_t(m_pos[0]) << 8) |
				(uint32_t(m_pos[0]));
			m_pos += 3;

			return value;
		}
		else {
			m_pos = m_end;
			m_error = true;

			return 0;
		}
	}

	bool ReadBytes(void* dst, size_t size)
	{
		if (m_pos + size <= m_end) {
			memcpy(dst, m_pos, size);
			m_pos += size;

			return true;
		}
		else {
			m_error = true;

			return false;
		}
	}

	bool ReadString(std::string& str)
	{
		if (m_pos < m_end) {
			const uint8_t* p = m_pos;
			while (p < m_end && *p) {
				++p;
			}
			str.assign((const char*)m_pos, p - m_pos);
			m_pos = p;

			return true;
		}
		else {
			m_error = true;

			return false;
		}
	}
};
