// SPDX-License-Identifier: Apache-2.0 OR MIT
#include "common.hh"
#include "util.hh"
#include <stdio.h>
#include <filesystem>
namespace fs = std::filesystem;
#if defined(WIN32) && (defined(RSGAME_BUNDLE) || !defined(RSGAME_PORTABLE))
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <shlobj.h>
#undef near
#undef far
#undef min
#undef max
#undef NEAR
#undef FAR
#endif
namespace rsgame {
static void *load_file_raw(const char *filename, size_t *size) {
	if (verbose)
		fprintf(stderr, "trying file: %s\n", filename);
	return SDL_LoadFile(filename, size);
}
#if defined(WIN32)
static void *load_file_raw(const wchar_t *filename, size_t *size) {
	if (verbose)
		fprintf(stderr, "trying file: %S\n", filename);
	FILE *fp = _wfopen(filename, L"rb");
	return fp ? SDL_LoadFile_RW(SDL_RWFromFP(fp, SDL_TRUE), size, 1) : nullptr;
}
#endif
static void *load_file_raw(const fs::path &pa, size_t *size) {
	return load_file_raw(pa.c_str(), size);
}
#if defined(WIN32) && defined(RSGAME_BUNDLE)
static bool bundle_loaded = false;
static bool bundle_failed = true;
struct BundleEntry {
	void *buf;
	size_t len;
	char name[17];
};
static bool operator<(const BundleEntry &lhs, const BundleEntry &rhs) {
	return strcmp(lhs.name, rhs.name) < 0;
}
static bool operator<(const BundleEntry &lhs, const char *rhs) {
	return strcmp(lhs.name, rhs) < 0;
}
static bool operator<(const char *lhs, const BundleEntry &rhs) {
	return strcmp(lhs, rhs.name) < 0;
}
static std::set<BundleEntry, std::less<>> bundle_set;
static void load_bundle() {
	bundle_loaded = true;
	HRSRC hResInfo = FindResourceW(nullptr, MAKEINTRESOURCEW(102), MAKEINTRESOURCEW(256));
	if (!hResInfo)
		return;
	HGLOBAL hResData = LoadResource(nullptr, hResInfo);
	if (!hResData)
		return;
	char *buf = (char *)LockResource(hResData);
	DWORD len = SizeofResource(nullptr, hResInfo);
	if (!buf || len < 12 || memcmp(buf, "asse", 4))
		return;
	uint32_t count = *(uint32_t*)(buf+4);
	uint32_t ofs = *(uint32_t*)(buf+8);
	for (uint32_t i = 0; i < count; i++) {
		if (len < ofs+24)
			return;
		BundleEntry ent;
		ent.buf = buf + *(uint32_t*)(buf+ofs);
		ent.len = *(uint32_t*)(buf+ofs+4);
		strncpy(ent.name, buf+ofs+8, 16);
		ent.name[16] = 0;
		if (len < (char *)ent.buf + ent.len - buf)
			return;
		bundle_set.insert(ent);
		ofs += 24;
	}
	bundle_failed = false;
}
static void *load_file_bundle(int type, const char *filename, size_t *size) {
	if (type == FILE_DATA) {
		if (!bundle_loaded)
			load_bundle();
		if (!bundle_failed) {
			auto it = bundle_set.find(filename);
			if (it != bundle_set.end()) {
				void *res = SDL_malloc(it->len + 1);
				if (res) {
					memcpy(res, it->buf, it->len);
					((char*)res)[it->len] = 0;
					if (size)
						*size = it->len;
					return res;
				}
			}
		}
	}
	return 0;
}
#endif
#define APPDIRNAME "rsgame"
#if !defined(WIN32) && !defined(RSGAME_PORTABLE)
static fs::path get_file_path_xdghome(int type) {
	static const char *homeenvs[] = { "XDG_DATA_HOME", "XDG_CONFIG_HOME", "XDG_STATE_HOME" };
	static const char *homedefault[] = { ".local/share", ".config", ".local/state" };
	fs::path pa;
	const char *p;

	p = SDL_getenv(homeenvs[type]);
	if (!p || !*p) {
		p = SDL_getenv("HOME");
		if (!p || !*p)
			p = "/";
		return fs::path(p) / homedefault[type] / APPDIRNAME;
	} else {
		return fs::path(p) / APPDIRNAME;
	}
}
static void *load_file_xdgdirs(int type, const char *filename, size_t *size) {
	static const char *dirsenvs[] = { "XDG_DATA_DIRS", "XDG_CONFIG_DIRS", nullptr };
	static const char *dirsdefault[] = { "/usr/local/share:/usr/share", "/etc/xdg", nullptr };
	const char *p, *q;
	void *res;

	if (dirsenvs[type]) {
		p = SDL_getenv(dirsenvs[type]);
		if (!p || !*p)
			p = dirsdefault[type];
		while ((q = strchr(p, ':'))) {
			if ((res = load_file_raw(fs::path(p, q) / APPDIRNAME / filename, size)))
				return res;
			p = q+1;
		}
		if ((res = load_file_raw(fs::path(p) / APPDIRNAME / filename, size)))
			return res;
	}
	return 0;
}
#endif
#if defined(WIN32) && !defined(RSGAME_PORTABLE)
static fs::path get_file_path_appdata(int type) {
	wchar_t *p = nullptr;
	if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &p))) {
		static const char *suffixes[] = { "share", "config", "state" };
		fs::path pa = fs::path(p) / APPDIRNAME / suffixes[type];
		CoTaskMemFree(p);
		return pa;
	} else {
		CoTaskMemFree(p);
		return fs::path();
	}
}
#endif
static void *load_file_workdir(int type, const char *filename, size_t *size) {
	if (type == FILE_DATA)
		return load_file_raw(fs::path("assets") / filename, size);
	else
		return load_file_raw(filename, size);
}
#if defined(WIN32)
static void *load_file_exedir(int type, const char *filename, size_t *size) {
	wchar_t p[MAX_PATH];
	if (GetModuleFileNameW(nullptr, p, MAX_PATH) == MAX_PATH)
		return nullptr;
	if (type == FILE_DATA)
		return load_file_raw(fs::path(p).parent_path() / "assets" / filename, size);
	else
		return load_file_raw(fs::path(p).parent_path() / filename, size);
}
#endif
static void *load_file_impl(int type, const char *filename, size_t *size) {
	/* Search for the file in the following order:
	 * 1) ~/.local/share/rsgame, ~/.config/rsgame, ~/.local/state/rsgame (Unix)
	 * 2) %APPDATA%/rsgame/share, %APPDATA%/rsgame/config, %APPDATA%/rsgame/state (Win32)
	 * 3) Current working directory
	 * 4) Application (.exe) directory (Win32)
	 * 5) /usr/share/rsgame, /etc/xdg/rsgame (Unix)
	 * 6) Bundled resources (Win32, only FILE_DATA)
	 *
	 * 3 and 4 look for FILE_DATA in the assets subdirectory.
	 */
	void *res;
#if !defined(WIN32) && !defined(RSGAME_PORTABLE)
	if ((res = load_file_raw(get_file_path_xdghome(type) / filename, size)))
		return res;
#endif
#if defined(WIN32) && !defined(RSGAME_PORTABLE)
	fs::path pa = get_file_path_appdata(type);
	if (!pa.empty())
		if ((res = load_file_raw(pa / filename, size)))
			return res;
#endif
	if ((res = load_file_workdir(type, filename, size)))
		return res;
#if defined(WIN32)
	if ((res = load_file_exedir(type, filename, size)))
		return res;
#endif
#if !defined(WIN32) && !defined(RSGAME_PORTABLE)
	if ((res = load_file_xdgdirs(type, filename, size)))
		return res;
#endif
#if defined(WIN32) && defined(RSGAME_BUNDLE)
	if ((res = load_file_bundle(type, filename, size)))
		return res;
#endif
	return nullptr;
}
void SDLDeleter::operator()(void *p) {
	SDL_free(p);
}
std::unique_ptr<char[], SDLDeleter> load_file(int type, const char *filename, size_t &size) {
	return std::unique_ptr<char[], SDLDeleter>((char *)load_file_impl(type, filename, &size));
}
std::unique_ptr<char[], SDLDeleter> load_file(int type, const char *filename) {
	return std::unique_ptr<char[], SDLDeleter>((char *)load_file_impl(type, filename, 0));
}
}
