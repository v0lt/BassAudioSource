
#pragma once

#include <map>
#include "Utils/ByteReader.h"

// based on https://github.com/Aleksoid1978/MPC-BE/blob/master/src/DSUtil/ID3Tag.h

//
// ID3TagItem class
//

struct Chapters {
	std::wstring        name;
	REFERENCE_TIME rt = 0;

	Chapters(const std::wstring& _name, const REFERENCE_TIME _rt)
		: name(_name)
		, rt(_rt)
	{}
};

enum ID3Type {
	ID3_TYPE_STRING,
	ID3_TYPE_BINARY
};

class CID3TagItem
{
public:

	CID3TagItem(const UINT tag, const std::wstring& value)
		: m_type(ID3_TYPE_STRING)
		, m_tag(tag)
		, m_value(value) {}

	CID3TagItem(const UINT tag, const std::vector<BYTE>& data, const std::wstring& mime, const std::wstring& value)
		: m_type(ID3_TYPE_BINARY)
		, m_tag(tag)
		, m_Mime(mime)
		, m_Data(data)
		, m_value(value) {}

	UINT GetTag()        const { return m_tag; }
	std::wstring GetValue()    const { return m_value; }
	std::wstring GetMime()     const { return m_Mime; }
	const BYTE* GetData() const { return m_Data.data(); }
	size_t GetDataLen()   const { return m_Data.size(); }
	ID3Type GetType()     const { return m_type; }

protected:
	UINT             m_tag;

	// text value
	std::wstring      m_value;

	// binary value
	std::vector<BYTE> m_Data;
	std::wstring      m_Mime;

	ID3Type           m_type;
};

//
// CID3v2Tag
//

class CID3v2Tag
{
protected:
	BYTE m_major;
	BYTE m_flags;

	std::wstring ReadText(ByteReader& gb, UINT size, const BYTE encoding);
	std::wstring ReadField(ByteReader& gb, UINT &size, const BYTE encoding);

	void ReadTag(const UINT tag, ByteReader& gbData, UINT &size, CID3TagItem** item);
	void ReadChapter(ByteReader& gbData, UINT &size);

public:
	std::map<UINT, std::wstring> Tags;
	std::list<CID3TagItem*>  TagItems;
	std::list<Chapters>      ChaptersList;

	CID3v2Tag(BYTE major = 0, BYTE flags = 0);
	virtual ~CID3v2Tag();

	void Clear();

	// tag reading
	BOOL ReadTagsV2(const BYTE *buf, size_t len);
};

// ID3v2
static unsigned int hexdec2uint(unsigned int size)
{
	return (((size & 0x7F000000) >> 3) |
			((size & 0x007F0000) >> 2) |
			((size & 0x00007F00) >> 1) |
			((size & 0x0000007F)));
}

#define ID3v2_HEADER_SIZE 10
static int id3v2_match_len(const unsigned char *buf)
{
	if (buf[0] == 'I' && buf[1] == 'D' && buf[2] == '3'
			&& buf[3] != 0xff && buf[4] != 0xff
			&& (buf[6] & 0x80) == 0
			&& (buf[7] & 0x80) == 0
			&& (buf[8] & 0x80) == 0
			&& (buf[9] & 0x80) == 0) {
		int len = ((buf[6] & 0x7f) << 21) +
			((buf[7] & 0x7f) << 14) +
			((buf[8] & 0x7f) << 7) +
			(buf[9] & 0x7f) + ID3v2_HEADER_SIZE;

		if (buf[5] & 0x10) {
			len += ID3v2_HEADER_SIZE;
		}
		return len;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////
// experiments

struct ID3v2Frame
{
	uint32_t       id;
	uint32_t       flags;
	const uint8_t* data;
	uint32_t       size;
};

bool ParseID3v2Tag(const BYTE* buf, std::list<ID3v2Frame>& id3v2Frames); 

std::wstring GetID3v2FrameText(const ID3v2Frame& id3v2Frame);
