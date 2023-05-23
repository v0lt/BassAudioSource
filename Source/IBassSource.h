/*
 *  Copyright (C) 2023 v0lt
 */

#pragma once

interface __declspec(uuid("153B5D50-39C6-4251-A135-C6070EC7A3B0"))
IBassSource : public IUnknown {
	STDMETHOD_(bool, GetActive()) PURE;
};
