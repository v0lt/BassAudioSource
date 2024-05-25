/*
 *  Copyright (C) 2023-2024 v0lt
 */

#pragma once

std::wstring GetFilterDirectory();

bool IsLikelyFilePath(const std::wstring_view str);

LPCWSTR GetNameAndVersion();
