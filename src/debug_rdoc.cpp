// Debug only (not for merge): RenderDoc multi-frame captures and scripted
// key presses, driven by files so a tester (or a script) can use them without
// touching the window.
//
//   ABC_RDOC_TRIGGER=<file>  when the file appears: capture, then delete it.
//                            Its content (a number) = presents per capture
//                            (default 4). Needs the game started with
//                            librenderdoc.so preloaded and
//                            ENABLE_VULKAN_RENDERDOC_CAPTURE=1.
//   ABC_DEBUG_KEYS=<file>    when the file appears: run it, then delete it.
//                            One step per line: "<SDL scancode name> <ms>"
//                            (key held for ms, e.g. "Return 150"),
//                            "down <name>", "up <name>" or "sleep <ms>".
//
// Without the variables it does nothing.

#if !defined(_WIN32)
#include <elf.h>
#include <link.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

#include <SDL3/SDL.h>
#include <renderdoc_app.h>

// The loaded librenderdoc.so's entry point, or null. RenderDoc comes in as
// a Vulkan layer when Vulkan starts up, so this is retried until then.
// Looked up in the library's own dynamic symbol table: dlopen/dlsym can't be
// used, RenderDoc hooks dlopen and a system-wide preload hooks dlsym.
pRENDERDOC_GetAPI FindRenderDoc() {
  void* found = nullptr;
  dl_iterate_phdr(
      [](dl_phdr_info* info, size_t, void* out) -> int {
        if (!info->dlpi_name || !std::strstr(info->dlpi_name, "librenderdoc.so")) return 0;
        const ElfW(Dyn)* dyn = nullptr;
        for (int i = 0; i < info->dlpi_phnum; i++) {
          if (info->dlpi_phdr[i].p_type == PT_DYNAMIC) {
            dyn = reinterpret_cast<const ElfW(Dyn)*>(info->dlpi_addr + info->dlpi_phdr[i].p_vaddr);
          }
        }
        if (!dyn) return 0;
        const ElfW(Sym)* symtab = nullptr;
        const char* strtab = nullptr;
        const uint32_t* gnu_hash = nullptr;
        // Depending on the loader these are already absolute or still
        // relative to the load address.
        auto abs = [info](ElfW(Addr) p) { return p < info->dlpi_addr ? p + info->dlpi_addr : p; };
        for (; dyn->d_tag != DT_NULL; dyn++) {
          if (dyn->d_tag == DT_SYMTAB) symtab = reinterpret_cast<const ElfW(Sym)*>(abs(dyn->d_un.d_ptr));
          if (dyn->d_tag == DT_STRTAB) strtab = reinterpret_cast<const char*>(abs(dyn->d_un.d_ptr));
          if (dyn->d_tag == DT_GNU_HASH) gnu_hash = reinterpret_cast<const uint32_t*>(abs(dyn->d_un.d_ptr));
        }
        if (!symtab || !strtab || !gnu_hash) return 0;
        // Symbol count from the GNU hash table: highest bucket, then its chain.
        const uint32_t nbuckets = gnu_hash[0], symoffset = gnu_hash[1], bloom_size = gnu_hash[2];
        const uint32_t* buckets =
            gnu_hash + 4 + bloom_size * (sizeof(ElfW(Addr)) / sizeof(uint32_t));
        const uint32_t* chain = buckets + nbuckets;
        uint32_t last = 0;
        for (uint32_t b = 0; b < nbuckets; b++) last = buckets[b] > last ? buckets[b] : last;
        if (last < symoffset) return 0;
        while (!(chain[last - symoffset] & 1)) last++;
        for (uint32_t i = 0; i <= last; i++) {
          if (symtab[i].st_value && !std::strcmp(strtab + symtab[i].st_name, "RENDERDOC_GetAPI")) {
            *static_cast<void**>(out) = reinterpret_cast<void*>(info->dlpi_addr + symtab[i].st_value);
            return 1;
          }
        }
        return 0;
      },
      &found);
  return reinterpret_cast<pRENDERDOC_GetAPI>(found);
}

namespace {

void Sleep(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

// Reads and deletes `path` if it exists.
bool TakeFile(const std::string& path, std::string& content) {
  std::ifstream f(path);
  if (!f) return false;
  std::stringstream ss;
  ss << f.rdbuf();
  content = ss.str();
  f.close();
  std::remove(path.c_str());
  return true;
}

SDL_WindowID GameWindowId() {
  int count = 0;
  SDL_Window** windows = SDL_GetWindows(&count);
  SDL_WindowID id = (windows && count > 0) ? SDL_GetWindowID(windows[0]) : 0;
  SDL_free(windows);
  return id;
}

void PushKey(SDL_Scancode sc, bool down) {
  SDL_WindowID id = GameWindowId();
  // The keyboard driver ignores key-downs while it thinks the window is
  // unfocused, so claim focus first.
  if (down) {
    SDL_Event f{};
    f.type = SDL_EVENT_WINDOW_FOCUS_GAINED;
    f.window.windowID = id;
    SDL_PushEvent(&f);
  }
  SDL_Event e{};
  e.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
  e.key.windowID = id;
  e.key.scancode = sc;
  e.key.key = SDL_GetKeyFromScancode(sc, SDL_KMOD_NONE, false);
  e.key.down = down;
  SDL_PushEvent(&e);
}

void RunKeys(const std::string& script) {
  std::stringstream lines(script);
  for (std::string line; std::getline(lines, line);) {
    std::stringstream ls(line);
    std::string a, b;
    ls >> a >> b;
    if (a.empty()) continue;
    if (a == "sleep") {
      Sleep(std::atoi(b.c_str()));
    } else if (a == "down" || a == "up") {
      PushKey(SDL_GetScancodeFromName(b.c_str()), a == "down");
    } else {
      SDL_Scancode sc = SDL_GetScancodeFromName(a.c_str());
      if (sc == SDL_SCANCODE_UNKNOWN) {
        std::fprintf(stderr, "debug_keys: unknown key %s\n", a.c_str());
        continue;
      }
      PushKey(sc, true);
      Sleep(b.empty() ? 150 : std::atoi(b.c_str()));
      PushKey(sc, false);
      Sleep(100);
    }
  }
}

struct Starter {
  Starter() {
    const char* trig = std::getenv("ABC_RDOC_TRIGGER");
    const char* keys = std::getenv("ABC_DEBUG_KEYS");
    if ((!trig || !*trig) && (!keys || !*keys)) return;
    std::string trig_path = trig ? trig : "", keys_path = keys ? keys : "";
    std::thread([trig_path, keys_path] {
      RENDERDOC_API_1_4_0* api = nullptr;
      if (!trig_path.empty()) {
        pRENDERDOC_GetAPI get_api = nullptr;
        for (int i = 0; i < 120 && !(get_api = FindRenderDoc()); i++) Sleep(500);
        if (!get_api || !get_api(eRENDERDOC_API_Version_1_4_0, reinterpret_cast<void**>(&api))) {
          std::fprintf(stderr, "debug_rdoc: RenderDoc not loaded\n");
        } else {
          std::fprintf(stderr, "debug_rdoc: watching %s\n", trig_path.c_str());
        }
      }
      for (std::string content;;) {
        Sleep(100);
        if (api && TakeFile(trig_path, content)) {
          unsigned frames = unsigned(std::atoi(content.c_str()));
          if (frames == 0) frames = 4;
          api->TriggerMultiFrameCapture(frames);
          std::fprintf(stderr, "debug_rdoc: capture of %u frames\n", frames);
        }
        if (!keys_path.empty() && TakeFile(keys_path, content)) RunKeys(content);
      }
    }).detach();
  }
} starter;

}  // namespace
#endif
