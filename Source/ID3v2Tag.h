/*
 *  Copyright (C) 2022 v0lt
 */

#pragma once


struct ID3v2Frame
{
	uint32_t       id;
	uint32_t       flags;
	const uint8_t* data;
	uint32_t       size;
};

bool ParseID3v2Tag(const BYTE* buf, std::list<ID3v2Frame>& id3v2Frames); 

std::wstring GetID3v2FrameText(const ID3v2Frame& id3v2Frame);
